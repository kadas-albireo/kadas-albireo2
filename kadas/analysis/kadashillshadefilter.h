/***************************************************************************
    kadashillshadefilter.h
    ----------------------
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

#ifndef KADASHILLSHADEFILTER_H
#define KADASHILLSHADEFILTER_H

#include "kadas/analysis/kadas_analysis.h"
#include "kadas/analysis/kadasninecellfilter.h"


class KADAS_ANALYSIS_EXPORT KadasHillshadeFilter : public KadasNineCellFilter
{
  public:
    KadasHillshadeFilter( const QgsRasterLayer *layer, const QString &outputFile, const QString &outputFormat, double lightAzimuth = 300,
                          double lightAngle = 40, const QgsRectangle &filterRegion = QgsRectangle(), const QgsCoordinateReferenceSystem &filterRegionCrs = QgsCoordinateReferenceSystem() );

    /**Calculates output value from nine input values. The input values and the output value can be equal to the
    nodata value if not present or outside of the border. Must be implemented by subclasses*/
    float processNineCellWindow( float *x11, float *x21, float *x31,
                                 float *x12, float *x22, float *x32,
                                 float *x13, float *x23, float *x33 ) override;

    float lightAzimuth() const { return mLightAzimuth; }
    void setLightAzimuth( float azimuth ) { mLightAzimuth = azimuth; }
    float lightAngle() const { return mLightAngle; }
    void setLightAngle( float angle ) { mLightAngle = angle; }

  private:
    float mLightAzimuth;
    float mLightAngle;
};

#endif // KADASHILLSHADEFILTER_H
