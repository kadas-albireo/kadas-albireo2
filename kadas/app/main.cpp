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
#include <QDir>
#include <QSettings>

#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>

#include <kadas/core/kadas.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadascrashrpt.h>

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

  // Setup application
  QApplication::setOrganizationName(Kadas::KADAS_RELEASE_NAME);
  QApplication::setOrganizationDomain(Kadas::KADAS_RELEASE_NAME);
  QApplication::setApplicationName(Kadas::KADAS_RELEASE_NAME);

  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, Kadas::configPath());

  QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

  KadasApplication app( argc, argv );

  // Install crash reporter
  KadasCrashRpt crashReporter;
  crashReporter.install();

  return app.exec();
}
