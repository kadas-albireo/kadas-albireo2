/***************************************************************************
    kadascoordinateutils.h
    -----------------------
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

#ifndef KADASCOORDINATEUTILS_H
#define KADASCOORDINATEUTILS_H

#include <qgis/qgsunittypes.h>

#include "kadas/core/kadas_core.h"

class QgsPointXY;
class QgsCoordinateReferenceSystem;

class KADAS_CORE_EXPORT KadasCoordinateUtils {
public:
  static double getHeightAtPos(const QgsPointXY &p,
                               const QgsCoordinateReferenceSystem &crs,
                               Qgis::DistanceUnit unit, QString *errMsg = 0);
  static QByteArray getTimezoneAtPos(const QgsPointXY &p,
                                     const QgsCoordinateReferenceSystem &crs);
};

#endif // KADASCOORDINATEUTILS_H
