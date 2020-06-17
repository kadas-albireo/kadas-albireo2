/***************************************************************************
    kadas.cpp
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

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>
#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <winhttp.h>
#include <pacparser.h>
#endif

#include <qgis/qgssettings.h>
#include <qgis/qgsrasterlayer.h>

#include <kadas/core/kadas.h>
#include <kadas/core/kadas_config.h>

const char *Kadas::KADAS_VERSION = _KADAS_VERSION_;
const int Kadas::KADAS_VERSION_INT = _KADAS_VERSION_INT_;
const char *Kadas::KADAS_RELEASE_NAME = _KADAS_NAME_;
const char *Kadas::KADAS_FULL_RELEASE_NAME = _KADAS_FULL_NAME_;
const char *Kadas::KADAS_DEV_VERSION = _KADAS_DEV_VERSION_;
const char *Kadas::KADAS_BUILD_DATE = __DATE__;

static QString resolveDataPath()
{
  QString dataPath = qgetenv( "KADAS_DATA_PATH" );
  if ( !dataPath.isEmpty() )
  {
    return dataPath;
  }
  QFile file( QDir( QApplication::applicationDirPath() ).absoluteFilePath( "kadassourcedir.txt" ) );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    return QDir( file.readAll().trimmed() ).absoluteFilePath( "data" );
  }
  else
  {
    return QDir( QString( "%1/../share/%2" ).arg( QApplication::applicationDirPath(), QString( Kadas::KADAS_RELEASE_NAME ).toLower() ) ).absolutePath();
  }
}

static QString resolveResourcePath()
{
  QFile file( QDir( QApplication::applicationDirPath() ).absoluteFilePath( "kadassourcedir.txt" ) );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    return QDir( file.readAll().trimmed() ).absoluteFilePath( "kadas/resources" );
  }
  else
  {
    return QDir( QString( "%1/../share/%2/resources" ).arg( QApplication::applicationDirPath(), QString( Kadas::KADAS_RELEASE_NAME ).toLower() ) ).absolutePath();
  }
}

QString Kadas::configPath()
{
  QDir appDataDir = QDir( QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) );
  return appDataDir.absoluteFilePath( Kadas::KADAS_RELEASE_NAME );
}

QString Kadas::pkgDataPath()
{
  static QString dataPath = resolveDataPath();
  return dataPath;
}

QString Kadas::pkgResourcePath()
{
  static QString resourcePath = resolveResourcePath();
  return resourcePath;
}

QString Kadas::projectTemplatesPath()
{
  return QDir( pkgDataPath() ).absoluteFilePath( "project_templates" );
}

QString Kadas::gdalSource( const QgsMapLayer *layer )
{
  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer )
  {
    return QString();
  }

  QString providerType  = layer->providerType();
  QString layerSource = layer->source();
  if ( providerType == "gdal" )
  {
    return layerSource;
  }
  else if ( providerType == "wcs" )
  {
    QgsDataSourceUri uri;
    uri.setEncodedUri( layerSource );

    QString wcsUrl;
    if ( !uri.hasParam( "url" ) )
    {
      return QString();
    }
    wcsUrl = uri.param( "url" );
    if ( !wcsUrl.endsWith( "?" ) && !wcsUrl.endsWith( "&" ) )
    {
      if ( wcsUrl.contains( "?" ) )
      {
        wcsUrl.append( "&" );
      }
      else
      {
        wcsUrl.append( "?" );
      }
    }
    QString gdalSource = QString( "WCS:%1" ).arg( wcsUrl );
    if ( uri.hasParam( "version" ) )
    {
      gdalSource.append( QString( "&version=%1" ).arg( uri.param( "version" ) ) );
    }
    if ( uri.hasParam( "identifier" ) )
    {
      gdalSource.append( QString( "&coverage=%1" ).arg( uri.param( "identifier" ) ) );
    }
    if ( uri.hasParam( "crs" ) )
    {
      gdalSource.append( QString( "&crs=%1" ).arg( uri.param( "crs" ) ) );
    }
    return gdalSource;
  }
  return QString();
}

void Kadas::gdalProxyConfig()
{
  QSettings settings;
  QString gdalHttpProxy = settings.value( "proxy/gdalHttpProxy", "" ).toString();
  QString gdalProxyAuth = settings.value( "proxy/gdalProxyAuth", "" ).toString();
  QString gdalProxyUserPwd = settings.value( "proxy/gdalProxyUserPwd", "" ).toString();

#ifdef Q_OS_WINDOWS
  if ( gdalHttpProxy.isEmpty() )
  {
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyInfo;
    WinHttpGetIEProxyConfigForCurrentUser( &proxyInfo );

    QString pacUrl;
    QString proxyBypass;

    if ( proxyInfo.lpszProxy != NULL )
    {
      gdalHttpProxy = QString::fromWCharArray( proxyInfo.lpszProxy );
      GlobalFree( proxyInfo.lpszProxy );
    }

    if ( proxyInfo.lpszProxyBypass != NULL )
    {
      proxyBypass = QString::fromWCharArray( proxyInfo.lpszProxyBypass );
      GlobalFree( proxyInfo.lpszProxyBypass );
    }

    if ( proxyInfo.lpszAutoConfigUrl != NULL )
    {
      pacUrl = QString::fromWCharArray( proxyInfo.lpszAutoConfigUrl );
      GlobalFree( proxyInfo.lpszAutoConfigUrl );
    }

    if ( !pacUrl.isEmpty() )
    {
      QNetworkAccessManager nam;
      QNetworkReply *reply = nam.get( QNetworkRequest( pacUrl ) );
      QEventLoop evloop;
      QObject::connect( reply, &QNetworkReply::finished, &evloop, &QEventLoop::quit );
      evloop.exec( QEventLoop::ExcludeUserInputEvents );
      QByteArray data = reply->readAll();


      pacparser_init();
      pacparser_parse_pac_string( data.data() );
      gdalHttpProxy = QString::fromLocal8Bit( pacparser_find_proxy( "http://example.com", "example.com" ) );
      if ( gdalHttpProxy.startsWith( "PROXY", Qt::CaseInsensitive ) )
      {
        gdalHttpProxy = gdalHttpProxy.mid( 6 );
      }
      pacparser_cleanup();
    }
  }
#endif

  QgsDebugMsg( QString( "GDAL_HTTP_PROXY: %1" ).arg( gdalHttpProxy ) );
  QgsDebugMsg( QString( "GDAL_HTTP_PROXYUSERPWD: %1" ).arg( gdalProxyUserPwd ) );
  QgsDebugMsg( QString( "GDAL_PROXY_AUTH: %1" ).arg( gdalProxyAuth ) );
  qputenv( "GDAL_HTTP_PROXY", gdalHttpProxy.toLocal8Bit() );
  qputenv( "GDAL_HTTP_PROXYUSERPWD", gdalProxyUserPwd.toLocal8Bit() );
  qputenv( "GDAL_PROXY_AUTH", gdalProxyAuth.toLocal8Bit() );
}
