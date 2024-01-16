/***************************************************************************
    kadascoordinateutils.cpp
    ------------------------
    copyright            : (C) 2022 by Sandro Mani
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

#include <gdal.h>
#include <zonedetect/zonedetect.h>

#include <qgis/qgscoordinateformatter.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>

#include <kadas/core/kadas.h>
#include <kadas/core/kadascoordinateutils.h>
#include <kadas/core/kadaslatlontoutm.h>

double KadasCoordinateUtils::getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, Qgis::DistanceUnit unit, QString *errMsg )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    if ( errMsg )
    {
      *errMsg = QObject::tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." );
    }
    return 0;
  }

  GDALDatasetH raster = Kadas::gdalOpenForLayer( static_cast<QgsRasterLayer *>( layer ), errMsg );
  if ( !raster )
  {
    return 0;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    if ( errMsg )
    {
      *errMsg = QObject::tr( "Failed to get raster geotransform" );
    }
    GDALClose( raster );
    return 0;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs = QgsCoordinateReferenceSystem::fromWkt( proj );
  if ( !rasterCrs.isValid() )
  {
    if ( errMsg )
    {
      *errMsg = QObject::tr( "Failed to get raster CRS" );
    }
    GDALClose( raster );
    return 0;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !raster )
  {
    if ( errMsg )
    {
      *errMsg = QObject::tr( "Failed to open raster band 0" );
    }
    GDALClose( raster );
    return 0;
  }

  // Get vertical unit
  Qgis::DistanceUnit vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? Qgis::DistanceUnit::Feet : Qgis::DistanceUnit::Meters;

  // Transform geo position to raster CRS
  QgsPointXY pRaster = QgsCoordinateTransform( crs, rasterCrs, QgsProject::instance() ).transform( p );

  // Transform raster geo position to pixel coordinates
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );

  double pixValues[4] = {};
  if ( CE_None != GDALRasterIO( band, GF_Read,
                                std::floor( col ), std::floor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
  {
    if ( errMsg )
    {
      *errMsg = QObject::tr( "Failed to read pixel values" );
    }
    GDALClose( raster );
    return 0;
  }

  GDALClose( raster );

  // Interpolate values
  double lambdaR = row - std::floor( row );
  double lambdaC = col - std::floor( col );

  double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                 + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
  if ( rasterCrs.mapUnits() != unit )
  {
    value *= QgsUnitTypes::fromUnitToUnitFactor( vertUnit, unit );
  }
  return value;
}

QByteArray KadasCoordinateUtils::getTimezoneAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs )
{
  QString db = QDir( Kadas::pkgResourcePath() ).absoluteFilePath( "timezone21.bin" );
  ZoneDetect *zd = ZDOpenDatabase( db.toLocal8Bit().data() );
  if ( !zd )
  {
    return "";
  }
  QgsPointXY pLatLon = QgsCoordinateTransform( crs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( p );

  char *zone = ZDHelperSimpleLookupString( zd, pLatLon.y(), pLatLon.x() );
  QByteArray zoneStr = QByteArray( zone );
  free( zone );
  ZDCloseDatabase( zd );
  return zoneStr;
}
