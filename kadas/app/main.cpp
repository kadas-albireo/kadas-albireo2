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

  QString configLocalStorageLocation = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation ).value( 0 );
  QString rootProfileFolder = QgsUserProfileManager::resolveProfilesFolder( configLocalStorageLocation );

  bool clearsettings = false;
  QString profileName = "default";
  for ( int i = 1; i < argc; ++i )
  {
    if ( qstrcmp( argv[i], "--clearsettings" ) == 0 )
    {
      clearsettings = true;
    }
  }

  QString locale = QLocale::system().name();
  if ( clearsettings )
  {
    QDir( QString( "%1/%2" ).arg( rootProfileFolder, profileName ) ).removeRecursively();
  }
  else
  {
    QSettings settings( QDir( rootProfileFolder ).absoluteFilePath( QString( "%1/Kadas/%2.ini" ).arg( profileName, Kadas::KADAS_RELEASE_NAME ) ), QSettings::IniFormat );
    if ( settings.value( "/locale/overrideFlag", false ).toBool() )
    {
      locale = settings.value( "/locale/userLocale", locale ).toString();
    }
    else
    {
      settings.setValue( "/locale/userLocale", locale );
    }
  }
  KadasApplication::setTranslation( locale );

  KadasApplication *app = new KadasApplication( argc, argv );

#ifdef __MINGW32__
  QString gdalDir = QDir( QString( "%1/../share/gdal" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "GDAL_DATA", gdalDir.toLocal8Bit() );
  QString projDir = QDir( QString( "%1/../share/proj" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
  qputenv( "PROJ_LIB", projDir.toLocal8Bit() );
#endif

  // Set current dir equal to application dir path
  QDir::setCurrent( QApplication::applicationDirPath() );

  app->init();
  int status = app->exec();
  delete app;
  delete QgsProject::instance();
  return status;
}
