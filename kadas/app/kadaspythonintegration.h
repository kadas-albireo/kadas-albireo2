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

// forward declaration for PyObject
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

class KadasPythonInterface;


class KadasPythonIntegration : public QObject
{
  public:

    KadasPythonIntegration( QObject *parent = nullptr );
    ~KadasPythonIntegration();

    /* general purpose functions */
    void initPython( KadasPythonInterface *interface, bool installErrorHook );
    void exitPython();
    bool isEnabled();
    bool runString( const QString &command, QString msgOnError = QString(), bool single = true );
    bool runStringUnsafe( const QString &command, bool single = true );
    bool evalString( const QString &command, QString &result );
    bool getError( QString &errorClassName, QString &errorText );

    /**
     * Returns the path where QGIS Python related files are located.
     */
    QString pythonPath() const;

    /**
     * Returns an object's type name as a string
     */
    QString getTypeAsString( PyObject *obj );

    /* plugins related functions */

    /**
     * Returns the current path for Python plugins
     */
    QString pluginsPath() const;

    /**
     * Returns the current path for Python in home directory.
     */
    QString homePythonPath() const;

    /**
     * Returns the current path for home directory Python plugins.
     */
    QString homePluginsPath() const;

    /**
     * Returns a list of extra plugins paths passed with QGIS_PLUGINPATH environment variable.
     */
    QStringList extraPluginsPaths() const;

    QStringList pluginList();
    bool isPluginLoaded( const QString &packageName );
    QStringList listActivePlugins();
    bool loadPlugin( const QString &packageName );
    bool startPlugin( const QString &packageName );
    bool startProcessingPlugin( const QString &packageName );
    QString getPluginMetadata( const QString &pluginName, const QString &function );
    bool pluginHasProcessingProvider( const QString &pluginName );
    bool canUninstallPlugin( const QString &packageName );
    bool unloadPlugin( const QString &packageName );
    bool isPluginEnabled( const QString &packageName ) const;

  protected:

    /* functions that do the initialization work */

    //! initialize Python context
    void init();

    //! check qgis imports and plugins
    //\returns true if all imports worked
    bool checkSystemImports();

    //\returns true if qgis.user could be imported
    bool checkQgisUser();

    //! import custom user and global Python code (startup scripts)
    void doCustomImports();

    //! cleanup Python context
    void finish();

    void installErrorHook();

    void uninstallErrorHook();

    QString getTraceback();

    //! convert Python object to QString. If the object isn't unicode/str, it will be converted
    QString PyObjectToQString( PyObject *obj );

    //! reference to module __main__
    PyObject *mMainModule = nullptr;

    //! dictionary of module __main__
    PyObject *mMainDict = nullptr;

    //! flag determining that Python support is enabled
    bool mPythonEnabled = false;

  private:

    bool mErrorHookInstalled = false;
};

#endif // KADASPYTHONINTEGRATION_H
