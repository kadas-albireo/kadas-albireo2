/***************************************************************************
    kadasninecellfilter.cpp
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

#include <cpl_string.h>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsunittypes.h>

#include "kadas/core/kadas.h"
#include "kadas/analysis/kadasninecellfilter.h"


KadasNineCellFilter::KadasNineCellFilter( const QgsRasterLayer *layer, const QString &outputFile, const QString &outputFormat, const QgsRectangle &region, const QgsCoordinateReferenceSystem &regionCrs )
  : mLayer( layer )
  , mOutputFile( outputFile )
  , mOutputFormat( outputFormat )
  , mFilterRegion( region )
  , mFilterRegionCrs( regionCrs )
  , mCellSizeX( -1.0 )
  , mCellSizeY( -1.0 )
  , mInputNodataValue( -1.0 )
  , mOutputNodataValue( -1.0 )
  , mZFactor( -1 )
{
}


int KadasNineCellFilter::processRaster( QProgressDialog *p, QString &errorMsg )
{
  GDALAllRegister();

  //open input file
  int xSize, ySize;
  GDALDatasetH inputDataset = openInputFile( xSize, ySize );
  if ( inputDataset == NULL )
  {
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to open input file" );
    return 1; //opening of input file failed
  }
  double gtrans[6] = {};
  if ( GDALGetGeoTransform( inputDataset, &gtrans[0] ) != CE_None )
  {
    GDALClose( inputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Invalid input geotransform" );
    return 1;
  }

  //output driver
  GDALDriverH outputDriver = openOutputDriver();
  if ( outputDriver == 0 )
  {
    GDALClose( inputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to open output driver" );
    return 2;
  }

  QgsCoordinateReferenceSystem gdalCrs( QString( GDALGetProjectionRef( inputDataset ) ) );
  QgsCoordinateReferenceSystem inputCrs = mLayer->crs().authid() != gdalCrs.authid() ? gdalCrs : mLayer->crs();


  //determine the window
  int rowStart, rowEnd, colStart, colEnd;
  if ( !computeWindow( inputDataset, inputCrs, mFilterRegion, mFilterRegionCrs, rowStart, rowEnd, colStart, colEnd ) )
  {
    GDALClose( inputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to compute input window, no elevation data?" );
    return 2;
  }
  xSize = colEnd - colStart;
  ySize = rowEnd - rowStart;

  GDALDatasetH outputDataset = openOutputFile( inputDataset, inputCrs, outputDriver, colStart, rowStart, xSize, ySize );
  if ( outputDataset == NULL )
  {
    GDALClose( inputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to create output file" );
    return 3; //create operation on output file failed
  }

  //open first raster band for reading (operation is only for single band raster)
  GDALRasterBandH rasterBand = GDALGetRasterBand( inputDataset, 1 );
  if ( rasterBand == NULL )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to get input raster band" );
    return 4;
  }
  mInputNodataValue = GDALGetRasterNoDataValue( rasterBand, NULL );

  GDALRasterBandH outputRasterBand = GDALGetRasterBand( outputDataset, 1 );
  if ( outputRasterBand == NULL )
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Unable to create output raster band" );
    return 5;
  }
  //try to set -9999 as nodata value
  GDALSetRasterNoDataValue( outputRasterBand, -9999 );
  mOutputNodataValue = GDALGetRasterNoDataValue( outputRasterBand, NULL );

  // Autocompute the zFactor if it is -1
  if ( mZFactor == -1 )
  {
    Qgis::DistanceUnit vertUnit = strcmp( GDALGetRasterUnitType( rasterBand ), "ft" ) == 0 ? Qgis::DistanceUnit::Feet : Qgis::DistanceUnit::Meters;
    if ( inputCrs.mapUnits() == Qgis::DistanceUnit::Meters && vertUnit == Qgis::DistanceUnit::Feet )
    {
      mZFactor = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, Qgis::DistanceUnit::Feet );
    }
    else if ( inputCrs.mapUnits() == Qgis::DistanceUnit::Feet && vertUnit == Qgis::DistanceUnit::Meters )
    {
      mZFactor = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Feet, Qgis::DistanceUnit::Meters );
    }
    else if ( inputCrs.mapUnits() == Qgis::DistanceUnit::Degrees && vertUnit == Qgis::DistanceUnit::Meters )
    {
      // Take latitude in the middle of the window
      double px = 0.5 * ( colStart + colEnd );
      double py = 0.5 * ( rowStart + rowEnd );
      double latitude = gtrans[3] + px * gtrans[4] + py * gtrans[5];
      mZFactor = ( 111320 * std::cos( latitude * M_PI / 180. ) );
    }
    else if ( inputCrs.mapUnits() == Qgis::DistanceUnit::Degrees && vertUnit == Qgis::DistanceUnit::Feet )
    {
      // Take latitude in the middle of the window
      double px = 0.5 * ( colStart + colEnd );
      double py = 0.5 * ( rowStart + rowEnd );
      double latitude = gtrans[3] + px * gtrans[4] + py * gtrans[5];
      mZFactor = ( ( 111320 * std::cos( latitude * M_PI / 180. ) ) ) * QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, Qgis::DistanceUnit::Feet );
    }
    else
    {
      QgsDebugMsgLevel( "Warning: Failed to automatically compute zFactor, defaulting to 1", 2 );
      mZFactor = 1;
    }
  }

  if ( ySize < 3 ) //we require at least three rows (should be true for most datasets)
  {
    GDALClose( inputDataset );
    GDALClose( outputDataset );
    errorMsg = QApplication::translate( "KadasNineCellFilter", "Too small input dataset" );
    return 6;
  }

  //keep only three scanlines in memory at a time
  float *scanLine1 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
  float *scanLine2 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
  float *scanLine3 = ( float * ) CPLMalloc( sizeof( float ) * xSize );

  float *resultLine = ( float * ) CPLMalloc( sizeof( float ) * xSize );

  if ( p )
  {
    p->setMaximum( ySize );
  }

  //values outside the layer extent (if the 3x3 window is on the border) are sent to the processing method as (input) nodata values
  for ( int i = 0; i < ySize; ++i )
  {
    if ( p )
    {
      p->setValue( i );
    }

    if ( p && p->wasCanceled() )
    {
      break;
    }

    if ( i == 0 )
    {
      //fill scanline 1 with (input) nodata for the values above the first row and feed scanline2 with the first row
      for ( int a = 0; a < xSize; ++a )
      {
        scanLine1[a] = mInputNodataValue;
      }
      CPLErr err = GDALRasterIO( rasterBand, GF_Read, colStart, rowStart, xSize, 1, scanLine2, xSize, 1, GDT_Float32, 0, 0 );
      Q_UNUSED( err );
    }
    else
    {
      //normally fetch only scanLine3 and release scanline 1 if we move forward one row
      CPLFree( scanLine1 );
      scanLine1 = scanLine2;
      scanLine2 = scanLine3;
      scanLine3 = ( float * ) CPLMalloc( sizeof( float ) * xSize );
    }

    if ( i == ySize - 1 ) //fill the row below the bottom with nodata values
    {
      for ( int a = 0; a < xSize; ++a )
      {
        scanLine3[a] = mInputNodataValue;
      }
    }
    else
    {
      CPLErr err = GDALRasterIO( rasterBand, GF_Read, colStart, rowStart + i + 1, xSize, 1, scanLine3, xSize, 1, GDT_Float32, 0, 0 );
      Q_UNUSED( err );
    }

#pragma omp parallel for schedule( static )
    for ( int j = 0; j < xSize; ++j )
    {
      if ( j == 0 )
      {
        resultLine[j] = processNineCellWindow( &mInputNodataValue, &scanLine1[j], &scanLine1[j + 1], &mInputNodataValue, &scanLine2[j],
                                               &scanLine2[j + 1], &mInputNodataValue, &scanLine3[j], &scanLine3[j + 1] );
      }
      else if ( j == xSize - 1 )
      {
        resultLine[j] = processNineCellWindow( &scanLine1[j - 1], &scanLine1[j], &mInputNodataValue, &scanLine2[j - 1], &scanLine2[j],
                                               &mInputNodataValue, &scanLine3[j - 1], &scanLine3[j], &mInputNodataValue );
      }
      else
      {
        resultLine[j] = processNineCellWindow( &scanLine1[j - 1], &scanLine1[j], &scanLine1[j + 1], &scanLine2[j - 1], &scanLine2[j],
                                               &scanLine2[j + 1], &scanLine3[j - 1], &scanLine3[j], &scanLine3[j + 1] );
      }
    }

    CPLErr err = GDALRasterIO( outputRasterBand, GF_Write, 0, i, xSize, 1, resultLine, xSize, 1, GDT_Float32, 0, 0 );
    Q_UNUSED( err );
  }

  if ( p )
  {
    p->setValue( ySize );
  }

  CPLFree( resultLine );
  CPLFree( scanLine1 );
  CPLFree( scanLine2 );
  CPLFree( scanLine3 );

  GDALClose( inputDataset );

  if ( p && p->wasCanceled() )
  {
    //delete the dataset without closing (because it is faster)
    GDALDeleteDataset( outputDriver, mOutputFile.toUtf8().constData() );
    return 7;
  }
  GDALClose( outputDataset );

  return 0;
}

GDALDatasetH KadasNineCellFilter::openInputFile( int &nCellsX, int &nCellsY )
{
  GDALDatasetH inputDataset = Kadas::gdalOpenForLayer( mLayer );
  if ( inputDataset != NULL )
  {
    nCellsX = GDALGetRasterXSize( inputDataset );
    nCellsY = GDALGetRasterYSize( inputDataset );

    //we need at least one band
    if ( GDALGetRasterCount( inputDataset ) < 1 )
    {
      GDALClose( inputDataset );
      return NULL;
    }
  }
  return inputDataset;
}

bool KadasNineCellFilter::computeWindow( GDALDatasetH dataset, const QgsCoordinateReferenceSystem &datasetCrs, const QgsRectangle &region, const QgsCoordinateReferenceSystem &regionCrs, int &rowStart, int &rowEnd, int &colStart, int &colEnd )
{
  int nCellsX = GDALGetRasterXSize( dataset );
  int nCellsY = GDALGetRasterYSize( dataset );

  if ( region.isEmpty() )
  {
    rowStart = 0;
    rowEnd = nCellsY;
    colStart = 0;
    colEnd = nCellsX;
    return true;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( dataset, &gtrans[0] ) != CE_None )
  {
    return false;
  }

  QgsCoordinateTransform ct( regionCrs, datasetCrs, QgsProject::instance() );

  // Transform raster geo position to pixel coordinates
  QgsPointXY regionPoints[4] = {
    QgsPointXY( region.xMinimum(), region.yMinimum() ),
    QgsPointXY( region.xMaximum(), region.yMinimum() ),
    QgsPointXY( region.xMaximum(), region.yMaximum() ),
    QgsPointXY( region.xMinimum(), region.yMaximum() ) };
  QgsPointXY pRaster = ct.transform( regionPoints[0] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  colStart = std::floor( col );
  colEnd = std::ceil( col );
  rowStart = std::floor( row );
  rowEnd = std::ceil( row );

  for ( int i = 1; i < 4; ++i )
  {
    pRaster = ct.transform( regionPoints[i] );
    col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
    row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
    colStart = std::min( colStart, static_cast<int>( std::floor( col ) ) );
    colEnd = std::max( colEnd, static_cast<int>( std::ceil( col ) ) );
    rowStart = std::min( rowStart, static_cast<int>( std::floor( row ) ) );
    rowEnd = std::max( rowEnd, static_cast<int>( std::ceil( row ) ) );
  }

  colStart = std::max( colStart, 0 );
  colEnd = std::min( colEnd + 1, nCellsX );
  rowStart = std::max( rowStart, 0 );
  rowEnd = std::min( rowEnd + 1, nCellsY );

  return colEnd > colStart && rowEnd > rowStart;
}

GDALDriverH KadasNineCellFilter::openOutputDriver()
{
  char **driverMetadata;

  //open driver
  GDALDriverH outputDriver = GDALGetDriverByName( mOutputFormat.toLocal8Bit().data() );

  if ( outputDriver == NULL )
  {
    return outputDriver; //return NULL, driver does not exist
  }

  driverMetadata = GDALGetMetadata( outputDriver, NULL );
  if ( !CSLFetchBoolean( driverMetadata, GDAL_DCAP_CREATE, false ) )
  {
    return NULL; //driver exist, but it does not support the create operation
  }

  return outputDriver;
}

GDALDatasetH KadasNineCellFilter::openOutputFile( GDALDatasetH inputDataset, const QgsCoordinateReferenceSystem &inputCrs, GDALDriverH outputDriver, int colStart, int rowStart, int xSize, int ySize )
{
  if ( inputDataset == NULL )
  {
    return NULL;
  }

  //open output file
  char **papszOptions = NULL;
  papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "LZW" );
  GDALDatasetH outputDataset = GDALCreate( outputDriver, mOutputFile.toUtf8().constData(), xSize, ySize, 1, GDT_Float32, papszOptions );
  if ( outputDataset == NULL )
  {
    return outputDataset;
  }


  //get geotransform from inputDataset
  double geotransform[6];
  if ( GDALGetGeoTransform( inputDataset, geotransform ) != CE_None )
  {
    GDALClose( outputDataset );
    return NULL;
  }

  // Shift for origin of window
  geotransform[0] += colStart * geotransform[1] + rowStart * geotransform[2];
  geotransform[3] += colStart * geotransform[4] + rowStart * geotransform[5];

  GDALSetGeoTransform( outputDataset, geotransform );

  //make sure mCellSizeX and mCellSizeY are always > 0
  mCellSizeX = geotransform[1];
  if ( mCellSizeX < 0 )
  {
    mCellSizeX = -mCellSizeX;
  }
  mCellSizeY = geotransform[5];
  if ( mCellSizeY < 0 )
  {
    mCellSizeY = -mCellSizeY;
  }

  GDALSetProjection( outputDataset, inputCrs.toWkt().toLocal8Bit().data() );

  return outputDataset;
}

float KadasNineCellFilter::calcFirstDerX( float *x11, float *x21, float *x31, float *x12, float *x22, float *x32, float *x13, float *x23, float *x33 )
{
  //the basic formula would be simple, but we need to test for nodata values...
  //return (( (*x31 - *x11) + 2 * (*x32 - *x12) + (*x33 - *x13) ) / (8 * mCellSizeX));

  int weight = 0;
  double sum = 0;

  //first row
  if ( *x31 != mInputNodataValue && *x11 != mInputNodataValue ) //the normal case
  {
    sum += ( *x31 - *x11 );
    weight += 2;
  }
  else if ( *x31 == mInputNodataValue && *x11 != mInputNodataValue && *x21 != mInputNodataValue ) //probably 3x3 window is at the border
  {
    sum += ( *x21 - *x11 );
    weight += 1;
  }
  else if ( *x11 == mInputNodataValue && *x31 != mInputNodataValue && *x21 != mInputNodataValue ) //probably 3x3 window is at the border
  {
    sum += ( *x31 - *x21 );
    weight += 1;
  }

  //second row
  if ( *x32 != mInputNodataValue && *x12 != mInputNodataValue ) //the normal case
  {
    sum += 2 * ( *x32 - *x12 );
    weight += 4;
  }
  else if ( *x32 == mInputNodataValue && *x12 != mInputNodataValue && *x22 != mInputNodataValue )
  {
    sum += 2 * ( *x22 - *x12 );
    weight += 2;
  }
  else if ( *x12 == mInputNodataValue && *x32 != mInputNodataValue && *x22 != mInputNodataValue )
  {
    sum += 2 * ( *x32 - *x22 );
    weight += 2;
  }

  //third row
  if ( *x33 != mInputNodataValue && *x13 != mInputNodataValue ) //the normal case
  {
    sum += ( *x33 - *x13 );
    weight += 2;
  }
  else if ( *x33 == mInputNodataValue && *x13 != mInputNodataValue && *x23 != mInputNodataValue )
  {
    sum += ( *x23 - *x13 );
    weight += 1;
  }
  else if ( *x13 == mInputNodataValue && *x33 != mInputNodataValue && *x23 != mInputNodataValue )
  {
    sum += ( *x33 - *x23 );
    weight += 1;
  }

  if ( weight == 0 )
  {
    return mOutputNodataValue;
  }

  return sum / ( weight * mCellSizeX * mZFactor );
}

float KadasNineCellFilter::calcFirstDerY( float *x11, float *x21, float *x31, float *x12, float *x22, float *x32, float *x13, float *x23, float *x33 )
{
  //the basic formula would be simple, but we need to test for nodata values...
  //return (((*x11 - *x13) + 2 * (*x21 - *x23) + (*x31 - *x33)) / ( 8 * mCellSizeY));

  double sum = 0;
  int weight = 0;

  //first row
  if ( *x11 != mInputNodataValue && *x13 != mInputNodataValue ) //normal case
  {
    sum += ( *x11 - *x13 );
    weight += 2;
  }
  else if ( *x11 == mInputNodataValue && *x13 != mInputNodataValue && *x12 != mInputNodataValue )
  {
    sum += ( *x12 - *x13 );
    weight += 1;
  }
  else if ( *x31 == mInputNodataValue && *x11 != mInputNodataValue && *x12 != mInputNodataValue )
  {
    sum += ( *x11 - *x12 );
    weight += 1;
  }

  //second row
  if ( *x21 != mInputNodataValue && *x23 != mInputNodataValue )
  {
    sum += 2 * ( *x21 - *x23 );
    weight += 4;
  }
  else if ( *x21 == mInputNodataValue && *x23 != mInputNodataValue && *x22 != mInputNodataValue )
  {
    sum += 2 * ( *x22 - *x23 );
    weight += 2;
  }
  else if ( *x23 == mInputNodataValue && *x21 != mInputNodataValue && *x22 != mInputNodataValue )
  {
    sum += 2 * ( *x21 - *x22 );
    weight += 2;
  }

  //third row
  if ( *x31 != mInputNodataValue && *x33 != mInputNodataValue )
  {
    sum += ( *x31 - *x33 );
    weight += 2;
  }
  else if ( *x31 == mInputNodataValue && *x33 != mInputNodataValue && *x32 != mInputNodataValue )
  {
    sum += ( *x32 - *x33 );
    weight += 1;
  }
  else if ( *x33 == mInputNodataValue && *x31 != mInputNodataValue && *x32 != mInputNodataValue )
  {
    sum += ( *x31 - *x32 );
    weight += 1;
  }

  if ( weight == 0 )
  {
    return mOutputNodataValue;
  }

  return sum / ( weight * mCellSizeY * mZFactor );
}
