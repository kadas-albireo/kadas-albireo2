/***************************************************************************
    kadascataloglayersource.cpp
    ---------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QRegularExpression>
#include <QSettings>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsdatasourceuri.h>
#include <qgis/qgslogger.h>

#include "kadas/gui/kadascataloglayersource.h"

QString KadasCatalogLayerSource::adjustUri( const QgsMimeDataUtils::Uri &uri, const QgsCoordinateReferenceSystem &canvasCrs )
{
  QString adjustedUri = uri.uri;

  // Adjust layer CRS to project CRS
  QgsCoordinateReferenceSystem testCrs;
  for ( const QString &c : uri.supportedCrs )
  {
    testCrs.createFromOgcWmsCrs( c );
    if ( testCrs == canvasCrs )
    {
      adjustedUri.replace( QRegularExpression( "crs=[^&]+" ), "crs=" + c );
      QgsDebugMsgLevel( QString( "Changing layer crs to %1, new uri: %2" ).arg( c, adjustedUri ), 2 );
      break;
    }
  }

  // Use the last used image format
  const QString lastImageEncoding = QSettings().value( "/Qgis/lastWmsImageEncoding", "image/png" ).toString();
  for ( const QString &fmt : uri.supportedFormats )
  {
    if ( fmt == lastImageEncoding )
    {
      adjustedUri.replace( QRegularExpression( "format=[^&]+" ), "format=" + fmt );
      QgsDebugMsgLevel( QString( "Changing layer format to %1, new uri: %2" ).arg( fmt, adjustedUri ), 2 );
      break;
    }
  }

  return adjustedUri;
}

KadasCatalogLayerSource KadasCatalogLayerSource::resolve( const QString &providerKey, const QString &adjustedUri, const QVariant &sublayerId, const QString &name )
{
  KadasCatalogLayerSource source;
  source.providerKey = providerKey;
  source.name = name;
  source.uri = adjustedUri;
  // Providers not recognized below are loaded through their provider as a
  // raster layer, which is also what most of the recognized ones are.
  source.type = Type::Raster;

  if ( providerKey == QLatin1String( "arcgismapserver" ) )
  {
    if ( sublayerId.isValid() )
    {
      QgsDataSourceUri dataSource( adjustedUri );
      dataSource.removeParam( "layer" );
      dataSource.setParam( "layer", QString::number( sublayerId.toInt() ) );
      source.uri = dataSource.uri( false );
    }
  }
  else if ( providerKey == QLatin1String( "arcgisfeatureserver" ) )
  {
    source.type = Type::Vector;
    if ( sublayerId.isValid() )
    {
      QgsDataSourceUri dataSource( adjustedUri );
      const QString urlParameter = QString( "%1/%2" ).arg( dataSource.param( "url" ) ).arg( sublayerId.toInt() );
      dataSource.removeParam( "url" );
      dataSource.setParam( "url", urlParameter );
      source.uri = dataSource.uri( false );
    }
  }
  else if ( providerKey == QLatin1String( "arcgisvectortileservice" ) )
  {
    // The service is a single layer; a sublayer id does not narrow it down.
    source.type = Type::VectorTile;
  }
  else if ( providerKey == QLatin1String( "wms" ) && sublayerId.isValid() )
  {
    source.uri.replace( QRegularExpression( "layers=[^&]*" ), "layers=" + sublayerId.toString() );
  }

  return source;
}
