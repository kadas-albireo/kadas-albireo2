/***************************************************************************
    qgspythonutilsimpl.h - routines for interfacing Python
    ---------------------
    begin                : May 2008
    copyright            : (C) 2008 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASPYTHONINTEGRATION_H
#define KADASPYTHONINTEGRATION_H

#include <QObject>
#include <QStringList>

#include <qgis/qgspythonrunner.h>

// forward declaration for PyObject
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

class KadasPluginInterface;


class KadasPythonIntegration : public QObject
{
    Q_OBJECT

  public:

    KadasPythonIntegration( QObject *parent = nullptr );
    ~KadasPythonIntegration();

    void initPython( KadasPluginInterface *iface, bool installErrorHook );
    void exitPython();
    bool isEnabled();

    void showConsole();

    bool runString( const QString &command, QString msgOnError = QString(), bool single = true );
    bool runStringUnsafe( const QString &command, bool single = true );
    bool evalString( const QString &command, QString &result ) const;
    bool getError( QString &errorClassName, QString &errorText );
    QString getTypeAsString( PyObject *obj );

    QString qgisPythonPath() const;
    QString qgisPluginsPath() const;
    QString kadasPythonPath() const;
    QString kadasPluginsPath() const;
    QString homePythonPath() const;
    QString homePluginsPath() const;

    void restorePlugins();

    bool loadPlugin( const QString &packageName );
    QString getPluginMetadata( const QString &pluginName, const QString &function ) const;

    bool canUninstallPlugin( const QString &packageName );
    bool disablePlugin( const QString &packageName );
    bool unloadPlugin( const QString &packageName );
    void unloadAllPlugins();

    QStringList pluginList();
    QStringList listActivePlugins();
    bool isPluginLoaded( const QString &packageName ) const;
    bool isPluginEnabled( const QString &packageName ) const;
    bool isPythonPluginCompatible( const QString &packageName ) const;

  protected:
    void init();
    bool checkSystemImports();
    bool checkQgisUser();
    void finish();

    void installErrorHook();
    void uninstallErrorHook();

    QString getTraceback();

    //! convert Python object to QString. If the object isn't unicode/str, it will be converted
    QString PyObjectToQString( PyObject *obj ) const;

    //! reference to module __main__
    PyObject *mMainModule = nullptr;

    //! dictionary of module __main__
    PyObject *mMainDict = nullptr;

    bool mPythonEnabled = false;

  private:
    bool checkQgisVersion( const QString &minVersion, const QString &maxVersion ) const;

    bool mErrorHookInstalled = false;
};


class KadasPythonRunner : public QgsPythonRunner
{
  public:
    explicit KadasPythonRunner( KadasPythonIntegration *pythonIntegration ) : mPythonIntegration( pythonIntegration ) {}

    bool runCommand( QString command, QString messageOnError = QString() ) override
    {
      if ( mPythonIntegration && mPythonIntegration->isEnabled() )
      {
        return mPythonIntegration->runString( command, messageOnError, false );
      }
      return false;
    }

    bool evalCommand( QString command, QString &result ) override
    {
      if ( mPythonIntegration && mPythonIntegration->isEnabled() )
      {
        return mPythonIntegration->evalString( command, result );
      }
      return false;
    }

  protected:
    KadasPythonIntegration *mPythonIntegration = nullptr;
};


#endif // KADASPYTHONINTEGRATION_H
