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
#include <fstream>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCodec>
#include <QSurfaceFormat>

#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsuserprofilemanager.h>

#include "kadas/core/kadas.h"
#include "kadasapplication.h"
#include "kadasgpsintegration.h"

int main( int argc, char *argv[] )
{
  // --debug-level <n> / --debug-level=<n>: QgsLogger reads QGIS_DEBUG once, early during
  // application construction, so this has to be applied before KadasApplication is created.
  for ( int i = 1; i < argc; ++i )
  {
    const QString arg = QString::fromLocal8Bit( argv[i] );
    QString value;
    if ( arg == QLatin1String( "--debug-level" ) && i + 1 < argc )
    {
      value = QString::fromLocal8Bit( argv[i + 1] );
    }
    else if ( arg.startsWith( QLatin1String( "--debug-level=" ) ) )
    {
      value = arg.mid( QStringLiteral( "--debug-level=" ).length() );
    }
    bool ok = false;
    const int debugLevel = value.toInt( &ok );
    if ( ok )
    {
      qputenv( "QGIS_DEBUG", QByteArray::number( debugLevel ) );
      break;
    }
  }

  try
  {
    std::ifstream file;
    file.open( "kadas.env" );

    std::string var;
    std::string name;
    std::string value;
    while ( std::getline( file, var ) )
    {
      size_t pos = var.find( '=' );
      if ( pos != std::string::npos )
      {
        name = var.substr( 0, pos );
        value = var.substr( pos + 1 );
        if ( !qputenv( name.c_str(), QByteArray::fromStdString( value ) ) )
        {
          std::string message = "Could not set environment variable:" + var;
          std::cerr << message;
          return EXIT_FAILURE;
        }
      }
    }
  }
  catch ( std::ifstream::failure &e )
  {
    std::string message = std::string( "Could not read environment file " ) + "`kadas.env`" + "[" + e.what() + "]";
    std::cerr << message;
    return EXIT_FAILURE;
  }

#ifdef Q_OS_MACOS
  // When running from the macOS app bundle (see platform/macos), use the Python runtime,
  // PROJ and GDAL data shipped in it. Variables which are already set (e.g. from kadas.env) win.
  // KadasApplication::init() points QGIS to its plugins and data in the bundle.
  const QDir bundleContentsDir( QDir::cleanPath( QFileInfo( QString::fromLocal8Bit( argv[0] ) ).absolutePath() + QStringLiteral( "/.." ) ) );
  if ( bundleContentsDir.exists( QStringLiteral( "Resources/qgis" ) ) )
  {
    const QList<QPair<const char *, QString>> bundleEnv {
      { "PYTHONHOME", bundleContentsDir.absoluteFilePath( QStringLiteral( "Frameworks" ) ) },
      { "PROJ_DATA", bundleContentsDir.absoluteFilePath( QStringLiteral( "Resources/qgis/proj" ) ) },
      { "GDAL_DATA", bundleContentsDir.absoluteFilePath( QStringLiteral( "Resources/qgis/gdal" ) ) },
    };
    for ( const auto &[name, value] : bundleEnv )
    {
      if ( !qEnvironmentVariableIsSet( name ) )
      {
        qputenv( name, value.toUtf8() );
      }
    }
  }
#endif // Q_OS_MACOS

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

  // Set Qt plugin path relative to executable if not pointing to a valid directory
  QByteArray qtPluginPath = qgetenv( "QT_PLUGIN_PATH" );
  if ( qtPluginPath.isEmpty() || !QDir( QString::fromLocal8Bit( qtPluginPath ) ).exists() )
  {
    // Resolve relative to the directory containing argv[0]
    QString appDir = QFileInfo( QString::fromLocal8Bit( argv[0] ) ).absolutePath();
    QString pluginDir = QDir( appDir ).absoluteFilePath( "Qt6/plugins" );
    if ( QDir( pluginDir ).exists() )
    {
      qputenv( "QT_PLUGIN_PATH", pluginDir.toLocal8Bit() );
    }
  }
#endif // Q_OS_WINDOWS

  QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );

  // Setup application
  QApplication::setOrganizationName( "Kadas" );
  QApplication::setOrganizationDomain( "kadas.sourcepole.com" );
  QApplication::setApplicationName( Kadas::KADAS_RELEASE_NAME );

  QSettings::setDefaultFormat( QSettings::IniFormat );
  QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, Kadas::configPath() );

  // Initialize the default surface format for all
  // QWindow and QWindow derived components
#if !defined( QT_NO_OPENGL )
  QSurfaceFormat format;
  format.setRenderableType( QSurfaceFormat::OpenGL );
#ifdef Q_OS_MAC
  format.setVersion( 4, 1 ); //OpenGL is deprecated on MacOS, use last supported version
  format.setProfile( QSurfaceFormat::CoreProfile );
#else
  format.setVersion( 4, 3 );
  format.setProfile( QSurfaceFormat::CompatibilityProfile ); // Chromium only supports core profile on mac
#endif
  format.setDepthBufferSize( 24 );
  format.setSamples( 4 );
  format.setStencilBufferSize( 8 );
  QSurfaceFormat::setDefaultFormat( format );
#endif

  // Delete any leftover wcs cache
  QString configLocalStorageLocation = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation ).value( 0 );
  QDir( QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).absoluteFilePath( "wcs_cache" ) ).removeRecursively();

  KadasApplication *app = new KadasApplication( argc, argv );

  // Parse Kadas-specific command line options. We do this after constructing
  // QApplication so that QCommandLineParser sees the post-Qt argv (Qt strips
  // its own -platform / -style / -stylesheet etc.).
  QCommandLineParser parser;
  parser.setApplicationDescription( QStringLiteral( "KADAS Albireo" ) );
  QCommandLineOption networkLoggerOption( QStringList() << QStringLiteral( "network-logger" ), QStringLiteral( "Open the network logger panel at startup." ) );
  parser.addOption( networkLoggerOption );
  QCommandLineOption gpsSimulatorOption( QStringList() << QStringLiteral( "gps-simulator" ), QStringLiteral( "Feed the GPS from a built-in simulator following a route over Switzerland." ) );
  parser.addOption( gpsSimulatorOption );
  // Handled in main() before the application is constructed, declared here for --help only
  QCommandLineOption
    debugLevelOption( QStringList() << QStringLiteral( "debug-level" ), QStringLiteral( "Set the debug level (0 = errors only, higher = more verbose). Overrides the QGIS_DEBUG environment variable." ), QStringLiteral( "level" ) );
  parser.addOption( debugLevelOption );
  // Tolerate unknown options instead of aborting (Qt may still inject some).
  parser.parse( QApplication::arguments() );
  const bool openNetworkLogger = parser.isSet( networkLoggerOption );
  KadasGpsIntegration::setSimulatorEnabled( parser.isSet( gpsSimulatorOption ) );

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
  if ( openNetworkLogger )
  {
    app->setNetworkLoggingEnabled( true );
    app->showNetworkLogger();
  }
  int status = app->exec();

  // Delete wcs cache on close
  QDir( QDir( QStandardPaths::writableLocation( QStandardPaths::CacheLocation ) ).absoluteFilePath( "wcs_cache" ) ).removeRecursively();
  delete app;
  return status;
}
