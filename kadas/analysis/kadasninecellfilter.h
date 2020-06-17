/***************************************************************************
    kadasninecellfilter.h
    ---------------------
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

#ifndef KADASNINECELLFILTER_H
#define KADASNINECELLFILTER_H

#include <gdal.h>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsrectangle.h>

#include <kadas/analysis/kadas_analysis.h>

class QProgressDialog;
class QgsRasterLayer;

class KADAS_ANALYSIS_EXPORT KadasNineCellFilter
{
  public:
    /**Constructor that takes input file, output file and output format (GDAL string)*/
    KadasNineCellFilter( const QgsRasterLayer *layer, const QString &outputFile, const QString &outputFormat, const QgsRectangle &filterRegion = QgsRectangle(), const QgsCoordinateReferenceSystem &filterRegionCrs = QgsCoordinateReferenceSystem() );
    virtual ~KadasNineCellFilter() = default;

    /**Starts the calculation, reads from mInputFile and stores the result in mOutputFile
      @param p progress dialog that receives update and that is checked for abort. 0 if no progress bar is needed.
      @return 0 in case of success*/
    int processRaster( QProgressDialog *p, QString &errorMsg );

    double cellSizeX() const { return mCellSizeX; }
    void setCellSizeX( double size ) { mCellSizeX = size; }
    double cellSizeY() const { return mCellSizeY; }
    void setCellSizeY( double size ) { mCellSizeY = size; }

    double zFactor() const { return mZFactor; }
    /** Set to -1 to automatically compute */
    void setZFactor( double factor ) { mZFactor = factor; }

    double inputNodataValue() const { return mInputNodataValue; }
    void setInputNodataValue( double value ) { mInputNodataValue = value; }
    double outputNodataValue() const { return mOutputNodataValue; }
    void setOutputNodataValue( double value ) { mOutputNodataValue = value; }

    /**Calculates output value from nine input values. The input values and the output value can be equal to the
      nodata value if not present or outside of the border. Must be implemented by subclasses*/
    virtual float processNineCellWindow( float *x11, float *x21, float *x31,
                                         float *x12, float *x22, float *x32,
                                         float *x13, float *x23, float *x33 ) = 0;

  private:

    /**Opens the input file and returns the dataset handle and the number of pixels in x-/y- direction*/
    GDALDatasetH openInputFile( int &nCellsX, int &nCellsY );
    /**Opens the output driver and tests if it supports the creation of a new dataset
      @return NULL on error and the driver handle on success*/
    GDALDriverH openOutputDriver();
    /**Opens the output file and sets the same geotransform and CRS as the input data
      @return the output dataset or NULL in case of error*/
    GDALDatasetH openOutputFile( GDALDatasetH inputDataset, const QgsCoordinateReferenceSystem &inputCrs, GDALDriverH outputDriver, int colStart, int rowStart, int xSize, int ySize );
    /**Computes the window of the raster which contains the specified region of the raster*/
    bool computeWindow( GDALDatasetH dataset, const QgsCoordinateReferenceSystem &datasetCrs, const QgsRectangle &region, const QgsCoordinateReferenceSystem &regionCrs, int &rowStart, int &rowEnd, int &colStart, int &colEnd );

  protected:

    /**Calculates the first order derivative in x-direction according to Horn (1981)*/
    float calcFirstDerX( float *x11, float *x21, float *x31, float *x12, float *x22, float *x32, float *x13, float *x23, float *x33 );
    /**Calculates the first order derivative in y-direction according to Horn (1981)*/
    float calcFirstDerY( float *x11, float *x21, float *x31, float *x12, float *x22, float *x32, float *x13, float *x23, float *x33 );

    const QgsRasterLayer *mLayer;
    QString mOutputFile;
    QString mOutputFormat;
    QgsRectangle mFilterRegion;
    QgsCoordinateReferenceSystem mFilterRegionCrs;

    double mCellSizeX = -1.0;
    double mCellSizeY = -1.0;
    /**The nodata value of the input layer*/
    float mInputNodataValue = -1.0;
    /**The nodata value of the output layer*/
    float mOutputNodataValue = -1.0;
    /**Scale factor for z-value if x-/y- units are different to z-units (111120 for degree->meters and 370400 for degree->feet)*/
    double mZFactor = -1.0;
};

#endif // KADASNINECELLFILTER_H
