/***************************************************************************
    main.cpp
    ---------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <csignal>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QIcon>
#include <QSettings>
#include <QSplashScreen>
#include <QTranslator>

#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>

#include <kadas/core/kadas.h>
#include <kadas/app/kadascrashrpt.h>
#include <kadas/app/kadasmainwindow.h>

int main( int argc, char *argv[] )
{
  // Setup environment
#ifdef __MINGW32__
  QString gdalDir = QDir( QString( "%1/../share/gdal" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "GDAL_DATA", gdalDir.toLocal8Bit() );
#endif

  QgsDebugMsg( QString( "Starting qgis main" ) );

  // Set up the custom qWarning/qDebug custom handler
  // TODO: qInstallMsgHandler( myMessageOutput );

  //  if ( clearSettings )
  //  {
  //    QDir settingsDir( QgsApplication::qgisSettingsDirPath() );
  //    if ( settingsDir.exists() )
  //    {
  //      settingsDir.removeRecursively();
  //    }
  //  }

  // Setup application
  QApplication::setOrganizationName(Kadas::KADAS_RELEASE_NAME);
  QApplication::setOrganizationDomain(Kadas::KADAS_RELEASE_NAME);
  QApplication::setApplicationName(Kadas::KADAS_RELEASE_NAME);

  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, Kadas::configPath());

  QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

  QApplication app( argc, argv );
  app.setWindowIcon(QIcon(":/images/icon"));
  QFile styleSheet(":/stylesheet");
  if (styleSheet.open(QIODevice::ReadOnly))
  {
    app.setStyleSheet(QString::fromLocal8Bit(styleSheet.readAll()));
  }

  // Install crash reporter
  KadasCrashRpt crashReporter;
  crashReporter.install();

  // Create / migrate settings
  QSettings settings;

  QFile srcSettings;
  bool settingsEmpty = false;
  if ( QFile( settings.fileName() ).exists() )
  {
    QgsDebugMsg( "Patching settings" );
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_patch.ini" ) );
  }
  else
  {
    QgsDebugMsg( "Copying full settings" );
    settingsEmpty = true;
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_full.ini" ) );
  }
  if ( srcSettings.exists() )
  {
    QSettings newSettings( srcSettings.fileName(), QSettings::IniFormat );
    QString timestamp = settings.value( "timestamp", "0" ).toString();
    QString newtimestamp = newSettings.value( "timestamp" ).toString();
    if ( settingsEmpty || newtimestamp > timestamp )
    {
      settings.setValue( "timestamp", newtimestamp );
      // Merge new settings to old settings
      foreach ( const QString &group, newSettings.childGroups() )
      {
        newSettings.beginGroup( group );
        settings.beginGroup( group );
        foreach ( const QString &key, newSettings.childKeys() )
        {
          settings.setValue( key, newSettings.value( key ) );
        }
        newSettings.endGroup();
        settings.endGroup();
      }
    }
  }

  // Setup localization
  QString userLocale = settings.value( "locale/userLocale", QLocale::system().name() ).toString();

  QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  QTranslator qtTranslator;
  qtTranslator.load("qt_" + userLocale, qtTranslationsPath);
  qtTranslator.load("qtbase_" + userLocale, qtTranslationsPath);
  QApplication::instance()->installTranslator(&qtTranslator);

  QString i18nPath = QDir(Kadas::pkgDataPath()).absoluteFilePath("i18n");
  QTranslator appTranslator;
  appTranslator.load(QString("%1_%2").arg(Kadas::KADAS_RELEASE_NAME), userLocale, i18nPath);
  QApplication::instance()->installTranslator(&appTranslator);

  // Show splash screen
  QSplashScreen splash(QPixmap(":/images/splash"));
  splash.show();

  // TODO: QgsApplication::setMaxThreads( QSettings().value( "/Qgis/max_threads", -1 ).toInt() );

  // Check whether runnning online or offline
  QString onlineTestUrl = settings.value( "/app/onlineTestUrl" ).toString();
  QString projectTemplate;
  if ( !onlineTestUrl.isEmpty() )
  {
    QEventLoop eventLoop;
    QNetworkReply* reply = QgsNetworkAccessManager::instance()->head( QNetworkRequest( onlineTestUrl ) );
    QObject::connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
    eventLoop.exec();

    if ( reply->error() == QNetworkReply::NoError )
    {
      projectTemplate = settings.value( "/app/onlineDefaultProject" ).toString();
      settings.setValue( "/app/isOffline", false );
    }
    else
    {
      projectTemplate = settings.value( "/app/offlineDefaultProject" ).toString();
      settings.setValue( "/app/isOffline", true );
    }
    delete reply;
  }
  if(!projectTemplate.isEmpty())
  {
    projectTemplate = QDir(Kadas::projectTemplatesPath()).absoluteFilePath(projectTemplate);
  }

  // Ensure values are written to disk
  settings.sync();

  KadasMainWindow mainWindow(&splash);
  if(!projectTemplate.isEmpty()) {
    mainWindow.openProject(projectTemplate);
  }

  mainWindow.show();
  QObject::connect(&app, &QApplication::lastWindowClosed, &app, &QApplication::quit);
  splash.finish(&mainWindow);

  return app.exec();
}
