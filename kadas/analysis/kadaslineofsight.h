/***************************************************************************
    kadaslineofsight.h
    ------------------
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

#ifndef KADAS_LINE_OF_SIGHT
#define KADAS_LINE_OF_SIGHT

#include <kadas/analysis/kadas_analysis.h>

class QgsCoordinateReferenceSystem;
class QgsPoint;

class KADAS_ANALYSIS_EXPORT KadasLineOfSight
{
  public:
    static bool computeTargetVisibility( const QgsPoint &observerPos, const QgsPoint &targetPos, const QgsCoordinateReferenceSystem &crs, double nTarrainSamples, bool observerPosAbsolute = false, bool targetPosAbsolute = false );
};

#endif // KADAS_LINE_OF_SIGHT
