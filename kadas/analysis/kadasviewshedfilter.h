/***************************************************************************
    kadasviewshedfilter.h
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


#ifndef KADASVIEWSHEDFILTER_H
#define KADASVIEWSHEDFILTER_H

#include <QVector>

#include <qgis/qgscoordinatereferencesystem.h>

#include <kadas/analysis/kadas_analysis.h>

class QProgressDialog;
class QgsRasterLayer;


class KADAS_ANALYSIS_EXPORT KadasViewshedFilter
{
  public:
    static bool computeViewshed( const QgsRasterLayer *layer,
                                 const QString &outputFile, const QString &outputFormat,
                                 QgsPointXY observerPos, const QgsCoordinateReferenceSystem &observerPosCrs,
                                 double observerHeight, double targetHeight, bool heightRelToTerr, double radius,
                                 const QgsUnitTypes::DistanceUnit distanceElevUnit, QProgressDialog *progress, QString *errMsg,
                                 const QVector<QgsPointXY> &filterRegion = QVector<QgsPointXY>(), bool displayVisible = true, int accuracyFactor = 1 );

};

#endif // KADASVIEWSHEDFILTER_H
