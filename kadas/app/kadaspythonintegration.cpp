/***************************************************************************
    qgspythonutils.cpp - routines for interfacing Python
    ---------------------
    begin                : October 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Python.h>

#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#include <qgis/qgis.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmessagelog.h>
#include <qgis/qgsmessageoutput.h>
#include <qgis/qgssettings.h>

#include <kadas/core/kadas.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadaspythonintegration.h>

PyThreadState *_mainState = nullptr;


KadasPythonIntegration::KadasPythonIntegration( QObject *parent )
  : QObject( parent )
{
}

KadasPythonIntegration::~KadasPythonIntegration()
{
#if SIP_VERSION >= 0x40e06
  exitPython();
#endif
}

bool KadasPythonIntegration::checkSystemImports()
{
  runString( QStringLiteral( "import sys" ) );   // import sys module (for display / exception hooks)
  runString( QStringLiteral( "import os" ) );   // import os module (for user paths)

#ifdef Q_OS_WIN
  runString( "oldhome=None" );
  runString( "if 'HOME' in os.environ: oldhome=os.environ['HOME']\n" );
  runString( "os.environ['HOME']=os.environ['USERPROFILE']\n" );
#endif

  // construct a list of plugin paths
  // locally installed plugins have priority over the system plugins
  QStringList pluginpaths;
  pluginpaths << '"' + homePluginsPath() + '"';
  pluginpaths << '"' + kadasPluginsPath() + '"';
  pluginpaths << '"' + qgisPluginsPath() + '"';

  // expect that bindings are installed locally, so add the path to modules
  // also add path to plugins
  QStringList newpaths;
  newpaths << '"' + qgisPythonPath() + '"';
  newpaths << '"' + kadasPythonPath() + '"';
  newpaths << '"' + homePythonPath() + '"';
  newpaths << pluginpaths;
  runString( "sys.path = [" + newpaths.join( QStringLiteral( "," ) ) + "] + sys.path" );

  // import SIP
  if ( !runString( QStringLiteral( "from qgis.PyQt import sip" ),
                   QObject::tr( "Couldn't load SIP module." ) + '\n' + QObject::tr( "Python support will be disabled." ) ) )
  {
    return false;
  }

  // set PyQt api versions
  QStringList apiV2classes;
  apiV2classes << QStringLiteral( "QDate" ) << QStringLiteral( "QDateTime" ) << QStringLiteral( "QString" ) << QStringLiteral( "QTextStream" ) << QStringLiteral( "QTime" ) << QStringLiteral( "QUrl" ) << QStringLiteral( "QVariant" );
  Q_FOREACH ( const QString &clsName, apiV2classes )
  {
    if ( !runString( QStringLiteral( "sip.setapi('%1', 2)" ).arg( clsName ),
                     QObject::tr( "Couldn't set SIP API versions." ) + '\n' + QObject::tr( "Python support will be disabled." ) ) )
    {
      return false;
    }
  }
  // import Qt bindings
  if ( !runString( QStringLiteral( "from PyQt5 import QtCore, QtGui" ),
                   QObject::tr( "Couldn't load PyQt." ) + '\n' + QObject::tr( "Python support will be disabled." ) ) )
  {
    return false;
  }

  // import QGIS bindings
  QString error_msg = QObject::tr( "Couldn't load PyQGIS." ) + '\n' + QObject::tr( "Python support will be disabled." );
  if ( !runString( QStringLiteral( "from qgis.core import *" ), error_msg ) || !runString( QStringLiteral( "from qgis.gui import *" ), error_msg ) )
  {
    return false;
  }

  // import QGIS utils
  error_msg = QObject::tr( "Couldn't load QGIS utils." ) + '\n' + QObject::tr( "Python support will be disabled." );
  if ( !runString( QStringLiteral( "import qgis.utils" ), error_msg ) )
  {
    return false;
  }

  // tell the utils script where to look for the plugins
  runString( QStringLiteral( "qgis.utils.plugin_paths = [%1]" ).arg( pluginpaths.join( ',' ) ) );
  runString( QStringLiteral( "qgis.utils.sys_plugin_path = \"%1\"" ).arg( qgisPluginsPath() ) );
  runString( QStringLiteral( "qgis.utils.home_plugin_path = \"%1\"" ).arg( homePluginsPath() ) );    // note - homePluginsPath() returns a python expression, not a string literal

#ifdef Q_OS_WIN
  runString( "if oldhome: os.environ['HOME']=oldhome\n" );
#endif

  return true;
}

void KadasPythonIntegration::init()
{
  // initialize python
  Py_Initialize();
  // initialize threading AND acquire GIL
  PyEval_InitThreads();

  mPythonEnabled = true;

  mMainModule = PyImport_AddModule( "__main__" );  // borrowed reference
  mMainDict = PyModule_GetDict( mMainModule );  // borrowed reference
}

void KadasPythonIntegration::finish()
{
  // release GIL!
  // Later on, we acquire GIL just before doing some Python calls and
  // release GIL again when the work with Python API is done.
  // (i.e. there must be PyGILState_Ensure + PyGILState_Release pair
  // around any calls to Python API, otherwise we may segfault!)
  _mainState = PyEval_SaveThread();
}

bool KadasPythonIntegration::checkQgisUser()
{
  // import QGIS user
  QString error_msg = QObject::tr( "Couldn't load qgis.user." ) + '\n' + QObject::tr( "Python support will be disabled." );
  if ( !runString( QStringLiteral( "import qgis.user" ), error_msg ) )
  {
    // Should we really bail because of this?!
    return false;
  }
  return true;
}

void KadasPythonIntegration::initPython( KadasPluginInterface *interface, const bool installErrorHook )
{
  init();
  if ( !checkSystemImports() )
  {
    exitPython();
    return;
  }

  if ( interface )
  {
    // initialize 'iface' object
    runString( QStringLiteral( "qgis.utils.initInterface(%1)" ).arg( reinterpret_cast< quint64 >( interface ) ) );
  }

  if ( !checkQgisUser() )
  {
    exitPython();
    return;
  }
  if ( installErrorHook )
  {
    KadasPythonIntegration::installErrorHook();
  }
  finish();
}

void KadasPythonIntegration::exitPython()
{
  if ( mErrorHookInstalled )
  {
    uninstallErrorHook();
  }
  // causes segfault!
  //Py_Finalize();
  mMainModule = nullptr;
  mMainDict = nullptr;
  mPythonEnabled = false;
}


bool KadasPythonIntegration::isEnabled()
{
  return mPythonEnabled;
}

void KadasPythonIntegration::installErrorHook()
{
  runString( QStringLiteral( "qgis.utils.installErrorHook()" ) );
  mErrorHookInstalled = true;
}

void KadasPythonIntegration::uninstallErrorHook()
{
  runString( QStringLiteral( "qgis.utils.uninstallErrorHook()" ) );
}

bool KadasPythonIntegration::runStringUnsafe( const QString &command, bool single )
{
  // acquire global interpreter lock to ensure we are in a consistent state
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  // TODO: convert special characters from unicode strings u"…" to \uXXXX
  // so that they're not mangled to utf-8
  // (non-unicode strings can be mangled)
  PyObject *obj = PyRun_String( command.toUtf8().constData(), single ? Py_single_input : Py_file_input, mMainDict, mMainDict );
  bool res = nullptr == PyErr_Occurred();
  Py_XDECREF( obj );

  // we are done calling python API, release global interpreter lock
  PyGILState_Release( gstate );

  return res;
}

bool KadasPythonIntegration::runString( const QString &command, QString msgOnError, bool single )
{
  bool res = runStringUnsafe( command, single );
  if ( res )
  {
    return true;
  }

  if ( msgOnError.isEmpty() )
  {
    // use some default message if custom hasn't been specified
    msgOnError = QObject::tr( "An error occurred during execution of following code:" ) + "\n<tt>" + command + "</tt>";
  }

  // TODO: use python implementation

  QString traceback = getTraceback();
  QString path, version;
  evalString( QStringLiteral( "str(sys.path)" ), path );
  evalString( QStringLiteral( "sys.version" ), version );

  QString str = "<font color=\"red\">" + msgOnError + "</font><br><pre>\n" + traceback + "\n</pre>"
                + QObject::tr( "Python version:" ) + "<br>" + version + "<br><br>"
                + QObject::tr( "KADAS version:" ) + "<br>" + QStringLiteral( "%1 '%2', %3" ).arg( Kadas::KADAS_VERSION, Kadas::KADAS_RELEASE_NAME, Kadas::KADAS_DEV_VERSION ) + "<br><br>"
                + QObject::tr( "Python path:" ) + "<br>" + path;
  str.replace( '\n', QLatin1String( "<br>" ) ).replace( QLatin1String( "  " ), QLatin1String( "&nbsp; " ) );

  qDebug() << str;
  QgsMessageOutput *msg = QgsMessageOutput::createMessageOutput();
  msg->setTitle( QObject::tr( "Python error" ) );
  msg->setMessage( str, QgsMessageOutput::MessageHtml );
  msg->showMessage();

  return res;
}


QString KadasPythonIntegration::getTraceback()
{
#define TRACEBACK_FETCH_ERROR(what) {errMsg = what; goto done;}

  // acquire global interpreter lock to ensure we are in a consistent state
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  QString errMsg;
  QString result;

  PyObject *modStringIO = nullptr;
  PyObject *modTB = nullptr;
  PyObject *obStringIO = nullptr;
  PyObject *obResult = nullptr;

  PyObject *type, *value, *traceback;

  PyErr_Fetch( &type, &value, &traceback );
  PyErr_NormalizeException( &type, &value, &traceback );

  const char *iomod = "io";

  modStringIO = PyImport_ImportModule( iomod );
  if ( !modStringIO )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "can't import %1" ).arg( iomod ) );
  }

  obStringIO = PyObject_CallMethod( modStringIO, reinterpret_cast< const char * >( "StringIO" ), nullptr );

  /* Construct a cStringIO object */
  if ( !obStringIO )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "cStringIO.StringIO() failed" ) );
  }

  modTB = PyImport_ImportModule( "traceback" );
  if ( !modTB )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "can't import traceback" ) );
  }

  obResult = PyObject_CallMethod( modTB,  reinterpret_cast< const char * >( "print_exception" ),
                                  reinterpret_cast< const char * >( "OOOOO" ),
                                  type, value ? value : Py_None,
                                  traceback ? traceback : Py_None,
                                  Py_None,
                                  obStringIO );

  if ( !obResult )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "traceback.print_exception() failed" ) );
  }

  Py_DECREF( obResult );

  obResult = PyObject_CallMethod( obStringIO,  reinterpret_cast< const char * >( "getvalue" ), nullptr );
  if ( !obResult )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "getvalue() failed." ) );
  }

  /* And it should be a string all ready to go - duplicate it. */
  if ( !PyUnicode_Check( obResult ) )
  {
    TRACEBACK_FETCH_ERROR( QStringLiteral( "getvalue() did not return a string" ) );
  }

  result = QString::fromUtf8( PyUnicode_AsUTF8( obResult ) );

done:

  // All finished - first see if we encountered an error
  if ( result.isEmpty() && !errMsg.isEmpty() )
  {
    result = errMsg;
  }

  Py_XDECREF( modStringIO );
  Py_XDECREF( modTB );
  Py_XDECREF( obStringIO );
  Py_XDECREF( obResult );
  Py_XDECREF( value );
  Py_XDECREF( traceback );
  Py_XDECREF( type );

  // we are done calling python API, release global interpreter lock
  PyGILState_Release( gstate );

  return result;
}

QString KadasPythonIntegration::getTypeAsString( PyObject *obj )
{
  if ( !obj )
  {
    return QString();
  }

  if ( PyType_Check( obj ) )
  {
    QgsDebugMsg( QStringLiteral( "got type" ) );
    return QString( ( ( PyTypeObject * ) obj )->tp_name );
  }
  else
  {
    QgsDebugMsg( QStringLiteral( "got object" ) );
    return PyObjectToQString( obj );
  }
}

bool KadasPythonIntegration::getError( QString &errorClassName, QString &errorText )
{
  // acquire global interpreter lock to ensure we are in a consistent state
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  if ( !PyErr_Occurred() )
  {
    PyGILState_Release( gstate );
    return false;
  }

  PyObject *err_type = nullptr;
  PyObject *err_value = nullptr;
  PyObject *err_tb = nullptr;

  // get the exception information and clear error
  PyErr_Fetch( &err_type, &err_value, &err_tb );

  // get exception's class name
  errorClassName = getTypeAsString( err_type );

  // get exception's text
  if ( nullptr != err_value && err_value != Py_None )
  {
    errorText = PyObjectToQString( err_value );
  }
  else
  {
    errorText.clear();
  }

  // cleanup
  Py_XDECREF( err_type );
  Py_XDECREF( err_value );
  Py_XDECREF( err_tb );

  // we are done calling python API, release global interpreter lock
  PyGILState_Release( gstate );

  return true;
}


QString KadasPythonIntegration::PyObjectToQString( PyObject *obj ) const
{
  QString result;

  // is it None?
  if ( obj == Py_None )
  {
    return QString();
  }

  // check whether the object is already a unicode string
  if ( PyUnicode_Check( obj ) )
  {
    result = QString::fromUtf8( PyUnicode_AsUTF8( obj ) );
    return result;
  }

  // if conversion to Unicode failed, try to convert it to classic string, i.e. str(obj)
  PyObject *obj_str = PyObject_Str( obj );  // new reference
  if ( obj_str )
  {
    result = QString::fromUtf8( PyUnicode_AsUTF8( obj_str ) );
    Py_XDECREF( obj_str );
    return result;
  }

  // some problem with conversion to Unicode string
  QgsDebugMsg( QStringLiteral( "unable to convert PyObject to a QString!" ) );
  return QStringLiteral( "(qgis error)" );
}


bool KadasPythonIntegration::evalString( const QString &command, QString &result ) const
{
  // acquire global interpreter lock to ensure we are in a consistent state
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  PyObject *res = PyRun_String( command.toUtf8().constData(), Py_eval_input, mMainDict, mMainDict );
  bool success = nullptr != res;

  // TODO: error handling

  if ( success )
  {
    result = PyObjectToQString( res );
  }

  Py_XDECREF( res );

  // we are done calling python API, release global interpreter lock
  PyGILState_Release( gstate );

  return success;
}

QString KadasPythonIntegration::qgisPythonPath() const
{
  if ( QgsApplication::isRunningFromBuildDir() )
  {
    return QgsApplication::buildOutputPath() + QStringLiteral( "/python" );
  }
  else
  {
    return QgsApplication::pkgDataPath() + QStringLiteral( "/python" );
  }
}

QString KadasPythonIntegration::qgisPluginsPath() const
{
  return qgisPythonPath() + QStringLiteral( "/plugins" );
}

QString KadasPythonIntegration::kadasPythonPath() const
{
  if ( KadasApplication::isRunningFromBuildDir() )
  {
    return KadasApplication::applicationDirPath() + QStringLiteral( "/../python" );
  }
  else
  {
    return QgsApplication::pkgDataPath() + QStringLiteral( "/python" );
  }
}

QString KadasPythonIntegration::kadasPluginsPath() const
{
  return kadasPythonPath() + QStringLiteral( "/plugins" );
}

QString KadasPythonIntegration::homePythonPath() const
{
  QString settingsDir = QgsApplication::qgisSettingsDirPath();
  return QDir( settingsDir ).absoluteFilePath( "python" );
}

QString KadasPythonIntegration::homePluginsPath() const
{
  return QDir( homePythonPath() ).absoluteFilePath( "plugins" );
}

void KadasPythonIntegration::restorePlugins()
{
  QgsSettings mySettings;

  for ( const QString &packageName : pluginList() )
  {
    // check if the plugin was active on last session
    if ( mySettings.value( "/PythonPlugins/" + packageName ).toBool() )
    {
      loadPlugin( packageName );
    }
  }
}

bool KadasPythonIntegration::loadPlugin( const QString &packageName )
{
  if ( !mPythonEnabled )
  {
    return false;
  }

  bool success = false;
  QString output;

  if ( isPluginLoaded( packageName ) )
  {
    return true;
  }

  QString pluginName  = getPluginMetadata( packageName, QStringLiteral( "name" ) );
  QString description = getPluginMetadata( packageName, QStringLiteral( "description" ) );
  QString version     = getPluginMetadata( packageName, QStringLiteral( "version" ) );
  if ( pluginName == QLatin1String( "__error__" ) || description == QLatin1String( "__error__" ) || version == QLatin1String( "__error__" ) )
  {
    QgsMessageLog::logMessage( QObject::tr( "Error when reading metadata of plugin %1" ).arg( packageName ), QObject::tr( "Plugins" ) );
    success = false;
  }
  else
  {
    success = true;
  }

  if ( success )
  {
    success = isPythonPluginCompatible( packageName );
  }

  if ( success )
  {
    evalString( QStringLiteral( "qgis.utils.loadPlugin('%1')" ).arg( packageName ), output );
    success = ( output == QLatin1String( "True" ) );
  }

  if ( success )
  {
    evalString( QStringLiteral( "qgis.utils.startPlugin('%1')" ).arg( packageName ), output );
    success = ( output == QLatin1String( "True" ) );
  }

  QgsSettings().setValue( "/PythonPlugins/" + packageName, success );
  return success;
}

QString KadasPythonIntegration::getPluginMetadata( const QString &pluginName, const QString &function ) const
{
  if ( !mPythonEnabled )
  {
    return "";
  }

  QString res;
  QString str = QStringLiteral( "qgis.utils.pluginMetadata('%1', '%2')" ).arg( pluginName, function );
  evalString( str, res );
  //QgsDebugMsg("metadata "+pluginName+" - '"+function+"' = "+res);
  return res;
}

bool KadasPythonIntegration::canUninstallPlugin( const QString &packageName )
{
  if ( !mPythonEnabled )
  {
    return false;
  }

  QString output;
  evalString( QStringLiteral( "qgis.utils.canUninstallPlugin('%1')" ).arg( packageName ), output );
  return ( output == QLatin1String( "True" ) );
}

bool KadasPythonIntegration::disablePlugin( const QString &packageName )
{
  if ( !mPythonEnabled )
  {
    return false;
  }
  QgsSettings().setValue( "/PythonPlugins/" + packageName, false );
  return unloadPlugin( packageName );
}

bool KadasPythonIntegration::unloadPlugin( const QString &packageName )
{
  if ( !mPythonEnabled )
  {
    return false;
  }

  bool success = false;

  if ( isPluginLoaded( packageName ) )
  {
    QString output;
    evalString( QStringLiteral( "qgis.utils.unloadPlugin('%1')" ).arg( packageName ), output );
    success = ( output == QLatin1String( "True" ) );
  }

  return success;
}

void KadasPythonIntegration::unloadAllPlugins()
{
  if ( !mPythonEnabled )
  {
    return;
  }

  for ( const QString &pluginName : listActivePlugins() )
  {
    unloadPlugin( pluginName );
  }
}

QStringList KadasPythonIntegration::listActivePlugins()
{
  if ( !mPythonEnabled )
  {
    return QStringList();
  }

  QString output;
  evalString( QStringLiteral( "'\\n'.join(qgis.utils.active_plugins)" ), output );
  return output.split( QChar( '\n' ), QString::SkipEmptyParts );
}

QStringList KadasPythonIntegration::pluginList()
{
  if ( !mPythonEnabled )
  {
    return QStringList();
  }

  runString( QStringLiteral( "qgis.utils.updateAvailablePlugins()" ) );

  QString output;
  evalString( QStringLiteral( "'\\n'.join(qgis.utils.available_plugins)" ), output );
  return output.split( QChar( '\n' ), QString::SkipEmptyParts );
}

bool KadasPythonIntegration::isPluginEnabled( const QString &packageName ) const
{
  return QgsSettings().value( QStringLiteral( "/PythonPlugins/" ) + packageName, QVariant( false ) ).toBool();
}

bool KadasPythonIntegration::isPluginLoaded( const QString &packageName ) const
{
  if ( !mPythonEnabled )
  {
    return false;
  }

  QString output;
  evalString( QStringLiteral( "qgis.utils.isPluginLoaded('%1')" ).arg( packageName ), output );
  return ( output == QLatin1String( "True" ) );
}

bool KadasPythonIntegration::isPythonPluginCompatible( const QString &packageName ) const
{
  QString minVersion = getPluginMetadata( packageName, QStringLiteral( "qgisMinimumVersion" ) );
  // try to read qgisMaximumVersion. Note checkQgisVersion can cope with "__error__" value.
  QString maxVersion = getPluginMetadata( packageName, QStringLiteral( "qgisMaximumVersion" ) );
  return minVersion != QLatin1String( "__error__" ) && checkQgisVersion( minVersion, maxVersion );
}

bool KadasPythonIntegration::checkQgisVersion( const QString &minVersion, const QString &maxVersion ) const
{
  // Parse qgisMinVersion. Must be in form x.y.z or just x.y
  QStringList minVersionParts = minVersion.split( '.' );
  if ( minVersionParts.count() != 2 && minVersionParts.count() != 3 )
    return false;

  int minVerMajor, minVerMinor, minVerBugfix = 0;
  bool ok;
  minVerMajor = minVersionParts.at( 0 ).toInt( &ok );
  if ( !ok )
    return false;
  minVerMinor = minVersionParts.at( 1 ).toInt( &ok );
  if ( !ok )
    return false;
  if ( minVersionParts.count() == 3 )
  {
    minVerBugfix = minVersionParts.at( 2 ).toInt( &ok );
    if ( !ok )
      return false;
  }

  // Parse qgisMaxVersion. Must be in form x.y.z or just x.y
  int maxVerMajor, maxVerMinor, maxVerBugfix = 99;
  if ( maxVersion.isEmpty() || maxVersion == QLatin1String( "__error__" ) )
  {
    maxVerMajor = minVerMajor;
    maxVerMinor = 99;
  }
  else
  {
    QStringList maxVersionParts = maxVersion.split( '.' );
    if ( maxVersionParts.count() != 2 && maxVersionParts.count() != 3 )
      return false;

    bool ok;
    maxVerMajor = maxVersionParts.at( 0 ).toInt( &ok );
    if ( !ok )
      return false;
    maxVerMinor = maxVersionParts.at( 1 ).toInt( &ok );
    if ( !ok )
      return false;
    if ( maxVersionParts.count() == 3 )
    {
      maxVerBugfix = maxVersionParts.at( 2 ).toInt( &ok );
      if ( !ok )
        return false;
    }
  }

  // our qgis version - cut release name after version number
  QString qgisVersion = Qgis::QGIS_VERSION.section( '-', 0, 0 );

  QStringList qgisVersionParts = qgisVersion.split( '.' );

  int qgisMajor = qgisVersionParts.at( 0 ).toInt();
  int qgisMinor = qgisVersionParts.at( 1 ).toInt();
  int qgisBugfix = qgisVersionParts.at( 2 ).toInt();

  if ( qgisMinor == 99 )
  {
    // we want the API version, so for x.99 bump it up to the next major release: e.g. 2.99 to 3.0.0
    qgisMajor ++;
    qgisMinor = 0;
    qgisBugfix = 0;
  };

  // build XxYyZz strings with trailing zeroes if needed
  QString minVer = QStringLiteral( "%1%2%3" ).arg( minVerMajor, 2, 10, QChar( '0' ) )
                   .arg( minVerMinor, 2, 10, QChar( '0' ) )
                   .arg( minVerBugfix, 2, 10, QChar( '0' ) );
  QString maxVer = QStringLiteral( "%1%2%3" ).arg( maxVerMajor, 2, 10, QChar( '0' ) )
                   .arg( maxVerMinor, 2, 10, QChar( '0' ) )
                   .arg( maxVerBugfix, 2, 10, QChar( '0' ) );
  QString curVer = QStringLiteral( "%1%2%3" ).arg( qgisMajor, 2, 10, QChar( '0' ) )
                   .arg( qgisMinor, 2, 10, QChar( '0' ) )
                   .arg( qgisBugfix, 2, 10, QChar( '0' ) );

  // compare
  return ( minVer <= curVer && maxVer >= curVer );
}
