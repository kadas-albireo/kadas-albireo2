/***************************************************************************
    kadaslineofsight.cpp
    --------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <QString>

#include <gdal.h>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>

#include <kadas/analysis/kadaslineofsight.h>
#include <kadas/core/kadas.h>


bool KadasLineOfSight::computeTargetVisibility( const QgsPoint &observerPos, const QgsPoint &targetPos, const QgsCoordinateReferenceSystem &crs, double nTerrainSamples, bool observerPosAbsolute, bool targetPosAbsolute )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );

  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer )
  {
    // Assume visible if no terrain model available
    return true;
  }

  QString errMsg;
  GDALDatasetH raster = Kadas::gdalOpenForLayer( static_cast<QgsRasterLayer *>( layer ), &errMsg );
  if ( !raster )
  {
    return true;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    QgsDebugMsgLevel( "Failed to get raster geotransform" , 2 );
    GDALClose( raster );
    return true;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs( proj );
  if ( !rasterCrs.isValid() )
  {
    QgsDebugMsgLevel( "Failed to get raster CRS" , 2 );
    GDALClose( raster );
    return true;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !band )
  {
    QgsDebugMsgLevel( "Failed to open raster band 0" , 2 );
    GDALClose( raster );
    return true;
  }

  // Get vertical unit
  QgsUnitTypes::DistanceUnit vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? QgsUnitTypes::DistanceFeet : QgsUnitTypes::DistanceMeters;
  double heightConversion = QgsUnitTypes::fromUnitToUnitFactor( vertUnit, QgsUnitTypes::DistanceMeters );

  // Sample terrain under line from observer to target
  QgsCoordinateTransform crst( crs, rasterCrs, QgsProject::instance() );
  QgsPointXY obsPosRaster = crst.transform( observerPos );
  QgsPointXY targetPosRaster = crst.transform( targetPos );
  double dist = std::sqrt( obsPosRaster.sqrDist( targetPosRaster ) );
  QgsVector dir( ( targetPosRaster.x() - obsPosRaster.x() ) / dist, ( targetPosRaster.y() - obsPosRaster.y() ) / dist );

  // Read terrain samples
  QVector<QPointF> samples;
  double earthRadius = 6370000;
  for ( int i = 0; i < nTerrainSamples; ++i )
  {
    double k = double( i ) / nTerrainSamples * dist;
    QgsPointXY pRaster = obsPosRaster + dir * k;

    // Transform raster geo position to pixel coordinates
    double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
    double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );

    double pixValues[4] = {};
    if ( CE_None != GDALRasterIO( band, GF_Read,
                                  std::floor( col ), std::floor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
    {
      QgsDebugMsgLevel( "Failed to read pixel values" , 2 );
      samples.append( QPointF( samples.size(), 0 ) );
    }
    else
    {
      // Interpolate values
      double lambdaR = row - std::floor( row );
      double lambdaC = col - std::floor( col );

      double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                     + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
      double hCorr = 0.87 * k * k / ( 2 * earthRadius );
      samples.append( QPointF( k, value * heightConversion - hCorr ) );
    }
  }

  double zConv = QgsUnitTypes::fromUnitToUnitFactor( crs.mapUnits(), QgsUnitTypes::DistanceMeters );

  QPointF p1( samples.front().x(), ( observerPosAbsolute ? 0 : samples.front().y() ) + observerPos.z() * zConv );
  QPointF p2( samples.back().x(), ( targetPosAbsolute ? 0 : samples.back().y() ) + targetPos.z() * zConv );
  // For each sample position along line [p1, p2], check if y is below terrain
  // X = p1.x() + d * (p2.x() - p1.x())
  // Y = p1.y() + d * (p2.y() - p1.y())
  // => d = (X - p1.x()) / (p2.x() - p1.x())
  // => Y = p1.y() + (X - p1.x()) / (p2.x() - p1.x()) * (p2.y() - p1.y())
  bool visible = true;
  for ( int j = 1; j < nTerrainSamples; ++j )
  {
    double Y = p1.y() + ( samples[j].x() - p1.x() ) / ( p2.x() - p1.x() ) * ( p2.y() - p1.y() );
    if ( Y < samples[j].y() )
    {
      visible = false;
      break;
    }
  }
  GDALClose( raster );
  return visible;
}
