/***************************************************************************
    kadasslopefilter.h
    ------------------
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

#ifndef KADASSLOPEFILTER_H
#define KADASSLOPEFILTER_H

#include <qgis/qgscoordinatereferencesystem.h>

#include "kadas/analysis/kadas_analysis.h"
#include "kadas/analysis/kadasninecellfilter.h"

class KADAS_ANALYSIS_EXPORT KadasSlopeFilter : public KadasNineCellFilter {
public:
  KadasSlopeFilter(const QgsRasterLayer *layer, const QString &outputFile,
                   const QString &outputFormat,
                   const QgsRectangle &filterRegion = QgsRectangle(),
                   const QgsCoordinateReferenceSystem &filterRegionCrs =
                       QgsCoordinateReferenceSystem());

  float processNineCellWindow(float *x11, float *x21, float *x31, float *x12,
                              float *x22, float *x32, float *x13, float *x23,
                              float *x33) override;
};

#endif // KADASSLOPEFILTER_H
