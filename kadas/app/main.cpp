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
#include <QStandardPaths>
#include <QTextCodec>

#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsuserprofilemanager.h>

#include <kadas/core/kadas.h>
#include <kadas/app/kadasapplication.h>

int main( int argc, char *argv[] )
{
#ifdef Q_OS_WINDOWS
  // Clean environment
  QList<QByteArray> path = qgetenv( "PATH" ).split( ';' );
  QList<QByteArray> cleanPath;
  for ( const QByteArray &entry : path )
  {
    if ( entry.toLower().replace( '/', '\\' ).startsWith( "c:\\windows" ) )
    {
      cleanPath.append( entry );
    }
  }
  qputenv( "PATH", cleanPath.join( ';' ) );
#endif // Q_OS_WINDOWS

  QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );

  // Setup application
  QApplication::setOrganizationName( "Kadas" );
  QApplication::setOrganizationDomain( "kadas.sourcepole.com" );
  QApplication::setApplicationName( Kadas::KADAS_RELEASE_NAME );

  QSettings::setDefaultFormat( QSettings::IniFormat );
  QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, Kadas::configPath() );

  QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );
  QApplication::setAttribute( Qt::AA_DisableWindowContextHelpButton );  
  QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );

  // Delete any leftover wcs cache
  QString configLocalStorageLocation = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation ).value( 0 );
  QDir( QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).absoluteFilePath( "wcs_cache" ) ).removeRecursively();

  KadasApplication *app = new KadasApplication( argc, argv );

#ifdef __MINGW32__
  QString gdalDataDir = QDir( QString( "%1/../share/gdal" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "GDAL_DATA", gdalDataDir.toLocal8Bit() );
  QString gdalDriverDir = QDir( QString( "%1/../lib/gdalplugins" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "GDAL_DRIVER_PATH", gdalDriverDir.toLocal8Bit() );
  qputenv( "GDAL_HTTP_USE_CAPI_STORE", "YES" );
  QString projDir = QDir( QString( "%1/../share/proj" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "PROJ_LIB", projDir.toLocal8Bit() );
#endif

  // Set current dir equal to application dir path
  QDir::setCurrent( QApplication::applicationDirPath() );

  app->init();
  int status = app->exec();

  // Delete wcs cache on close
  QDir( QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).absoluteFilePath( "wcs_cache" ) ).removeRecursively();
  delete app;
  return status;
}
