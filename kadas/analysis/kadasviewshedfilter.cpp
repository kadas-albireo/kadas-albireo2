/***************************************************************************
    kadasviewshedfilter.cpp
    -----------------------
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
#include <QProgressDialog>

#include <cstring>
#include <cpl_string.h>
#include <gdal.h>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>

#include <kadas/core/kadas.h>
#include <kadas/analysis/kadasviewshedfilter.h>


static inline double geoToPixelX( double gtrans[6], double x, double y )
{
  return ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * y + gtrans[5] * x ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
}

static inline double geoToPixelY( double gtrans[6], double x, double y )
{
  return ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * y + gtrans[4] * x ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
}

static inline double pixelToGeoX( double gtrans[6], double px, double py )
{
  return gtrans[0] + px * gtrans[1] + py * gtrans[2];
}

static inline double pixelToGeoY( double gtrans[6], double px, double py )
{
  return gtrans[3] + px * gtrans[4] + py * gtrans[5];
}

bool KadasViewshedFilter::computeViewshed( const QgsRasterLayer *layer, const QString &outputFile, const QString &outputFormat, QgsPointXY observerPos,
    const QgsCoordinateReferenceSystem &observerPosCrs, double observerHeight, double targetHeight,
    bool observerHeightRelToTerr, bool targetHeightRelToTerr, double observerMinVertAngle, double observerMaxVertAngle,
    double radius, const QgsUnitTypes::DistanceUnit distanceElevUnit, QProgressDialog *progress,
    QString *errMsg, const QVector<QgsPointXY> &filterRegion, int accuracyFactor )
{
  // Open input file
  GDALDatasetH inputDataset = Kadas::gdalOpenForLayer( layer );
  if ( inputDataset == nullptr )
  {
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to open input dataset" );
    return false;
  }

  // Transform positions and measurements to dataset CRS
  QgsCoordinateReferenceSystem gdalCrs( QString( GDALGetProjectionRef( inputDataset ) ) );
  QgsCoordinateReferenceSystem datasetCrs = layer->crs().authid() != gdalCrs.authid() ? gdalCrs : layer->crs();
  if ( !datasetCrs.isValid() )
  {
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Could not determine input dataset CRS" );
    GDALClose( inputDataset );
    return false;
  }
  QgsCoordinateTransform ct( observerPosCrs, datasetCrs, QgsProject::instance() );
  observerPos = ct.transform( observerPos );
  if ( datasetCrs.mapUnits() != distanceElevUnit )
  {
    observerHeight *= QgsUnitTypes::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
    targetHeight *= QgsUnitTypes::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
    radius *= QgsUnitTypes::fromUnitToUnitFactor( distanceElevUnit, datasetCrs.mapUnits() );
  }


  // Open input band
  GDALRasterBandH inputBand = GDALGetRasterBand( inputDataset, 1 );
  if ( inputBand == NULL )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to open input dataset band 1" );
    return false;
  }
  float noDataValue = GDALGetRasterNoDataValue( inputBand, NULL );


  // Compute window of raster to read
  double gtrans[6] = {};
  if ( GDALGetGeoTransform( inputDataset, &gtrans[0] ) != CE_None )
  {
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to query input dataset geotransform" );
    return false;
  }
  int terWidth = GDALGetRasterXSize( inputDataset );
  int terHeight = GDALGetRasterYSize( inputDataset );

  int obs[2] =
  {
    qRound( geoToPixelX( gtrans, observerPos.x(), observerPos.y() ) ),
    qRound( geoToPixelY( gtrans, observerPos.x(), observerPos.y() ) )
  };
  double earthRadius = 6370000;
  if ( datasetCrs.mapUnits() != QgsUnitTypes::DistanceMeters )
  {
    earthRadius *= QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceMeters, datasetCrs.mapUnits() );
  }

  QList<QgsPointXY> cornerPoints = QList<QgsPointXY>()
                                   << QgsPointXY( observerPos.x() - radius, observerPos.y() - radius )
                                   << QgsPointXY( observerPos.x() + radius, observerPos.y() - radius )
                                   << QgsPointXY( observerPos.x() + radius, observerPos.y() + radius )
                                   << QgsPointXY( observerPos.x() - radius, observerPos.y() + radius );
  int colStart = std::numeric_limits<int>::max();
  int rowStart = std::numeric_limits<int>::max();
  int colEnd = -std::numeric_limits<int>::max();
  int rowEnd = -std::numeric_limits<int>::max();
  for ( const QgsPointXY &p : cornerPoints )
  {
    double x = geoToPixelX( gtrans, p.x(), p.y() );
    double y = geoToPixelY( gtrans, p.x(), p.y() );
    colStart = std::min( colStart, static_cast<int>( std::floor( x ) ) );
    colEnd = std::max( colEnd, static_cast<int>( std::ceil( x ) ) );
    rowStart = std::min( rowStart, static_cast<int>( std::floor( y ) ) );
    rowEnd = std::max( rowEnd, static_cast<int>( std::ceil( y ) ) );
  }
  colStart = std::max( 0, colStart );
  colEnd = std::min( terWidth - 1, colEnd );
  rowStart = std::max( 0, rowStart );
  rowEnd = std::min( terHeight - 1, rowEnd );
  int hmapWidth = colEnd - colStart + 1;
  int hmapHeight = rowEnd - rowStart + 1;
  QPolygon filterPoly;
  for ( int i = 0, n = filterRegion.size(); i < n; ++i )
  {
    QgsPointXY p = ct.transform( filterRegion[i] );
    filterPoly.append( QPoint( qRound( geoToPixelX( gtrans, p.x(), p.y() ) ), qRound( geoToPixelY( gtrans, p.x(), p.y() ) ) ) );
  }

  if ( obs[0] < colStart || obs[0] > colEnd || obs[1] < rowStart || obs[1] > rowEnd )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Observer pos is outside vieweshed area, reprojection distortion?" );
    return false;
  }

  int scaledHmapHeight = hmapHeight / accuracyFactor;
  int scaledHmapWidth = hmapWidth / accuracyFactor;

  // Read input heightmap
  // Allow at most 1GB allocated
  if ( scaledHmapWidth * scaledHmapHeight * sizeof( float ) > 1073741824 )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Too much memory required" );
    return false;
  }

  progress->setLabelText( QApplication::translate( "KadasViewshedFilter", "Loading elevation data..." ) );
  progress->setRange( 0, hmapHeight );

  // Read in lines of 4096 pixels max
  CPLErr err = CE_None;
  QVector<float> fullheightMap( hmapWidth * hmapHeight, noDataValue );
  int maxLineSize = std::min( 4096, hmapWidth );
  for ( int y = 0; y < hmapHeight; ++y )
  {
    if ( progress->wasCanceled() )
    {
      GDALClose( inputDataset );
      return false;
    }
    progress->setValue( y );
    QApplication::processEvents();

    for ( int x = 0; x < hmapWidth; x += maxLineSize )
    {
      int lineSize = std::min( maxLineSize, hmapWidth - x );
      int bufOff = ( y * hmapWidth + x );
      err = GDALRasterIOEx( inputBand, GF_Read, colStart + x, rowStart + y, lineSize, 1, &fullheightMap.data()[bufOff], lineSize, 1, GDT_Float32, 0, 0, nullptr );
      if ( err != CE_None )
      {
        GDALClose( inputDataset );
        *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to fetch raster pixels" );
        return false;
      }
    }
  }

  // In-memory dataset from full heightmap
  GDALDriverH driver = GDALGetDriverByName( "MEM" );
  GDALDatasetH memdataset = GDALCreate( driver, "", hmapWidth, hmapHeight, 1, GDT_Float32, nullptr );
  GDALRasterBandH memband = GDALGetRasterBand( memdataset, 1 );
  int bsX, bsY;
  GDALGetBlockSize( memband, &bsX, &bsY );
  Q_ASSERT( bsY == 1 ); // Vertical block size for MEM dataset should be 1
  int nXBlocks = ( hmapWidth + bsX - 1 ) / bsX;
  for ( int y = 0; y < hmapHeight && err == CE_None; ++y )
  {
    for ( int iXBlock = 0; iXBlock < nXBlocks && err == CE_None; ++iXBlock )
    {
      err = GDALWriteBlock( memband, iXBlock, y, fullheightMap.data() + y * hmapWidth + iXBlock * bsX );
    }
  }

  if ( err != CE_None )
  {
    GDALClose( memdataset );
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to fetch raster pixels" );
    return false;
  }

  // Downscale heightmap with rasterio
  QVector<float> heightmap( scaledHmapWidth * scaledHmapHeight, noDataValue );
  GDALRasterIOExtraArg rioargs;
  INIT_RASTERIO_EXTRA_ARG( rioargs );
  rioargs.eResampleAlg = GRIORA_Average;

  err = GDALRasterIOEx( memband, GF_Read, 0, 0, hmapWidth, hmapHeight, heightmap.data(), scaledHmapWidth, scaledHmapHeight, GDT_Float32, 0, 0, &rioargs );
  GDALClose( memdataset );

  if ( err != CE_None )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to fetch raster pixels" );
    return false;
  }

  // Adjust for reduced resolution
  gtrans[1] *= accuracyFactor;
  gtrans[2] *= accuracyFactor;
  gtrans[4] *= accuracyFactor;
  gtrans[5] *= accuracyFactor;
  colStart /= accuracyFactor;
  colEnd /= accuracyFactor;
  rowStart /= accuracyFactor;
  rowEnd /= accuracyFactor;
  obs[0] /= accuracyFactor;
  obs[1] /= accuracyFactor;
  hmapWidth = scaledHmapWidth;
  hmapHeight = scaledHmapHeight;
  for ( int i = 0, n = filterPoly.size(); i < n; ++i )
  {
    filterPoly[i] = QPoint( filterPoly[i].x() / accuracyFactor, filterPoly[i].y() / accuracyFactor );
  }

  // Prepare output
  GDALDriverH outputDriver = GDALGetDriverByName( outputFormat.toLocal8Bit().data() );
  if ( outputDriver == 0 )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to get driver for output" );
    return false;
  }
  if ( !CSLFetchBoolean( GDALGetMetadata( outputDriver, NULL ), GDAL_DCAP_CREATE, false ) )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Driver for output does not support creation" );
    return false;
  }
  char **papszOptions = CSLSetNameValue( 0, "COMPRESS", "LZW" );
  GDALDatasetH outputDataset = GDALCreate( outputDriver, outputFile.toLocal8Bit().data(), hmapWidth, hmapHeight, 1, GDT_Byte, papszOptions );
  if ( outputDataset == NULL )
  {
    GDALClose( inputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to open output dataset" );
    return false;
  }

  double outgtrans[6];
  std::memcpy( outgtrans, gtrans, sizeof( gtrans ) );

  // Shift for origin of window
  outgtrans[0] += colStart * outgtrans[1] + rowStart * outgtrans[2];
  outgtrans[3] += colStart * outgtrans[4] + rowStart * outgtrans[5];

  GDALSetGeoTransform( outputDataset, outgtrans );
  GDALSetProjection( outputDataset, GDALGetProjectionRef( inputDataset ) );

  GDALRasterBandH outputBand = GDALGetRasterBand( outputDataset, 1 );
  if ( outputBand == 0 )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to get output dataset band 1" );
    return false;
  }
  GDALSetRasterNoDataValue( outputBand, 127 );

  // Offset observer elevation by position at point
  if ( observerHeightRelToTerr )
  {
    observerHeight += heightmap[( obs[1] - rowStart ) * hmapWidth + ( obs[0] - colStart )];
  }


  // Compute viewshed
  int roi = std::min( hmapWidth, hmapHeight );
  progress->setLabelText( QApplication::translate( "KadasViewshedFilter", "Computing viewshed..." ) );
  progress->setRange( 0, 8 * roi );

  QVector<unsigned char> viewshed( hmapWidth * hmapHeight, 127 );
  for ( int radiusNumber = 0; radiusNumber < 8 * roi; ++radiusNumber )
  {
    if ( progress->wasCanceled() )
    {
      GDALClose( inputDataset );
      GDALClose( outputDataset );
      return false;
    }
    progress->setValue( radiusNumber );
    QApplication::processEvents();

    int target[2];
    if ( radiusNumber <= roi )
    {
      target[0] = obs[0] + roi;
      target[1] = obs[1] + radiusNumber;
    }
    else if ( radiusNumber <= 3 * roi )
    {
      target[0] = obs[0] + 2 * roi - radiusNumber;
      target[1] = obs[1] + roi;
    }
    else if ( radiusNumber <= 5 * roi )
    {
      target[0] = obs[0] - roi;
      target[1] = obs[1] + 4 * roi - radiusNumber;
    }
    else if ( radiusNumber <= 7 * roi )
    {
      target[0] = obs[0] + radiusNumber - 6 * roi;
      target[1] = obs[1] - roi;
    }
    else if ( radiusNumber < 8 * roi )
    {
      target[0] = obs[0] + roi;
      target[1] = obs[1] + radiusNumber - 8 * roi;
    }
    else
    {
      break; // All terrain points processed.
    }

    // Line of sight from observer to target.
    int delta[2] = {target[0] - obs[0], target[1] - obs[1]};
    int inciny = qAbs( delta[0] ) < qAbs( delta[1] );

    // Step along coord (X or Y) that varies most from observer to target.
    // That coord is inciny. Slope is how fast the other coord varies.
    double slope = ( double ) delta[1 - inciny] / ( double ) delta[inciny];
    int step = delta[inciny] > 0 ? 1 : -1;
    double horizon_slope = -99999; // Slope (in vertical plane) to horizon so far.

    // i = 0 would be the observer, which is always visible.
    for ( int i = step; true; i += step )
    {
      int p[2];
      p[inciny] = obs[inciny] + i;

      if ( i * slope > 0 )
      {
        p[1 - inciny] = obs[1 - inciny] + int ( std::ceil( i * slope - 0.5 ) );
      }
      else
      {
        p[1 - inciny] = obs[1 - inciny] + int ( std::floor( i * slope + 0.5 ) );
      }

      if ( p[0] < colStart || p[0] > colEnd || p[1] < rowStart || p[1] > rowEnd )
      {
        break;
      }

      //Is the point in the outside of the viewshed area?
      double dx = qAbs( p[0] - obs[0] ), dy = qAbs( p[1] - obs[1] );
      if ( !( dx <= roi && dy <= roi && dx * dx + dy * dy <= double ( roi ) * double ( roi ) ) )
      {
        break;
      }
      if ( !filterPoly.isEmpty() && !filterPoly.containsPoint( QPoint( p[0], p[1] ), Qt::OddEvenFill ) )
      {
        continue;
      }

      int idx = ( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart );
      if ( idx >= heightmap.size() )
      {
        continue;
      }
      float pElev = heightmap[idx];
      if ( pElev == noDataValue )
      {
        continue;
      }

      // Earth curvature correction
      double pGeoX = pixelToGeoX( gtrans, p[0], p[1] );
      double pGeoY = pixelToGeoY( gtrans, p[0], p[1] );
      double geoDistSqr = ( observerPos.x() - pGeoX ) * ( observerPos.x() - pGeoX ) + ( observerPos.y() - pGeoY ) * ( observerPos.y() - pGeoY );
      // http://www.swisstopo.admin.ch/internet/swisstopo/de/home/topics/survey/faq/curvature.html
      pElev -= 0.87 * geoDistSqr / ( 2 * earthRadius );

      // Update the slope if the current slope is greater than the old one
      double s = double ( pElev - observerHeight ) / double ( qAbs( p[inciny] - obs[inciny] ) );
      horizon_slope = std::max( horizon_slope, s );

      double horizon_alt =  observerHeight + horizon_slope * qAbs( p[inciny] - obs[inciny] );
      double tHeight = targetHeight;
      if ( targetHeightRelToTerr )
      {
        tHeight += pElev;
      }

      // Compute vertical angle from observer to target and ensure it is in range
      double vx = pGeoX - observerPos.x();
      double vy = pGeoY - observerPos.y();
      double vz = tHeight - observerHeight;
      double n = std::sqrt( vx * vx + vy * vy + vz * vz );
      double vangle = std::asin( vz / n ) / M_PI * 180.;

      if ( tHeight >= horizon_alt && vangle >= observerMinVertAngle && vangle <= observerMaxVertAngle )
      {
        viewshed[( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart )] = 255;
      }
      else
      {
        viewshed[( p[1] - rowStart ) * hmapWidth + ( p[0] - colStart )] = 0;
      }
    }
  }
  // The observer is always visible from itself
  viewshed[( obs[1] - rowStart ) * hmapWidth + ( obs[0] - colStart )] = 255;


  // Write output
  err = GDALRasterIO( outputBand, GF_Write, 0, 0, hmapWidth, hmapHeight, viewshed.data(), hmapWidth, hmapHeight, GDT_Byte, 0, 0 );
  GDALClose( inputDataset );
  GDALClose( outputDataset );
  if ( err != CE_None )
  {
    *errMsg = QApplication::translate( "KadasViewshedFilter", "Failed to write to output dataset" );
    return false;
  }
  return true;
}
