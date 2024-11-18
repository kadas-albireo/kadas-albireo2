/***************************************************************************
    kadasslopefilter.cpp
    --------------------
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

#include "kadas/analysis/kadasslopefilter.h"


KadasSlopeFilter::KadasSlopeFilter( const QgsRasterLayer *layer, const QString &outputFile, const QString &outputFormat, const QgsRectangle &filterRegion, const QgsCoordinateReferenceSystem &filterRegionCrs )
  : KadasNineCellFilter( layer, outputFile, outputFormat, filterRegion, filterRegionCrs )
{
}

float KadasSlopeFilter::processNineCellWindow( float *x11, float *x21, float *x31,
                                               float *x12, float *x22, float *x32, float *x13, float *x23, float *x33 )
{
  float derX = calcFirstDerX( x11, x21, x31, x12, x22, x32, x13, x23, x33 );
  float derY = calcFirstDerY( x11, x21, x31, x12, x22, x32, x13, x23, x33 );

  if ( derX == mOutputNodataValue || derY == mOutputNodataValue )
  {
    return mOutputNodataValue;
  }

  return atan( sqrt( derX * derX + derY * derY ) ) * 180.0 / M_PI;
}
