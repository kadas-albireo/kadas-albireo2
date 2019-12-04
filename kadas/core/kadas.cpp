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
#include <QStandardPaths>

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
    return QDir( QString( "%1/../share/%2" ).arg( QApplication::applicationDirPath(), Kadas::KADAS_RELEASE_NAME ) ).absolutePath();
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
    return QDir( QString( "%1/../share/%2/resources" ).arg( QApplication::applicationDirPath(), Kadas::KADAS_RELEASE_NAME ) ).absolutePath();
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
