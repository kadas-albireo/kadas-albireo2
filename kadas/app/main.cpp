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
#include <qgis/qgsproject.h>
#include <qgis/qgsuserprofilemanager.h>

#include <kadas/core/kadas.h>
#include <kadas/app/kadasapplication.h>

int main( int argc, char *argv[] )
{
  // Setup application
  QApplication::setOrganizationName( Kadas::KADAS_RELEASE_NAME );
  QApplication::setOrganizationDomain( Kadas::KADAS_RELEASE_NAME );
  QApplication::setApplicationName( Kadas::KADAS_RELEASE_NAME );

  QSettings::setDefaultFormat( QSettings::IniFormat );
  QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, Kadas::configPath() );

  QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

  QString configLocalStorageLocation = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation ).value( 0 );
  QString rootProfileFolder = QgsUserProfileManager::resolveProfilesFolder( configLocalStorageLocation );

  QSettings settings( QDir( rootProfileFolder ).absoluteFilePath( QString( "default/%1/%1.ini" ).arg( Kadas::KADAS_RELEASE_NAME ) ), QSettings::IniFormat );
  QString locale = QLocale::system().name();
  if ( settings.value( "/locale/overrideFlag", false ).toBool() )
  {
    locale = settings.value( "/locale/userLocale", locale ).toString();
  }
  else
  {
    settings.setValue( "/locale/userLocale", locale );
  }
  KadasApplication::setTranslation( locale );

  KadasApplication *app = new KadasApplication( argc, argv );

#ifdef __MINGW32__
  QString gdalDir = QDir( QString( "%1/../share/gdal" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "GDAL_DATA", gdalDir.toLocal8Bit() );
  QString projDir = QDir( QString( "%1/../share/proj" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "PROJ_LIB", projDir.toLocal8Bit() );
#endif

  app->init();
  int status = app->exec();
  delete app;
  delete QgsProject::instance();
  return status;
}
