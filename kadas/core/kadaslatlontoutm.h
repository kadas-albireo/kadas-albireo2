/***************************************************************************
    kadaslatlontoutm.h
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

#ifndef KADASLATLONTOUTM_H
#define KADASLATLONTOUTM_H

#include <QString>
#include <QPair>
#include <QPolygonF>

#include <kadas/core/kadas_core.h>

class QgsPointXY;
class QgsRectangle;

class KADAS_CORE_EXPORT KadasLatLonToUTM
{
public:
  struct UTMCoo {
    int easting;
    int northing;
    int zoneNumber;
    QString zoneLetter;
  };

  struct MGRSCoo {
    int easting;
    int northing;
    int zoneNumber;
    QString zoneLetter;
    QString letter100kID;
  };

  struct ZoneLabel {
    QPointF pos;
    QString label;
    QPointF maxPos;
  };

  struct GridLabel {
    QString label;
    int lineIdx;
    bool horiz;
  };

  static QgsPointXY UTM2LL ( const UTMCoo& utm, bool& ok );
  static UTMCoo LL2UTM ( const QgsPointXY& pLatLong );
  static MGRSCoo UTM2MGRS ( const UTMCoo& utmcoo );
  static UTMCoo MGRS2UTM ( const MGRSCoo& mgrs, bool& ok );

  static int getZoneNumber ( double lon, double lat );
  static QString getHemisphereLetter ( double lat );

  enum GridMode { GridUTM, GridMGRS };
  static void computeGrid ( const QgsRectangle& bbox, double mapScale,
                            QList<QPolygonF>& zoneLines, QList<QPolygonF>& subZoneLines, QList<QPolygonF>& gridLines,
                            QList<ZoneLabel>& zoneLabels, QList<ZoneLabel>& subZoneLabels, QList<GridLabel>& gridLabels, GridMode gridMode );

private:
  static const int NUM_100K_SETS;
  static const QString SET_ORIGIN_COLUMN_LETTERS;
  static const QString SET_ORIGIN_ROW_LETTERS;

  static QString getLetter100kID ( int column, int row, int parm );
  static double getMinNorthing ( int zoneLetter );
  typedef ZoneLabel ( zoneLabelCallback_t ) ( double, double, double, double );
  typedef void ( gridLabelCallback_t ) ( double, double, double, bool, int, QList<GridLabel>& );
  static void computeSubGrid ( int cellSize, double xMin, double xMax, double yMin, double yMax, QList<QPolygonF>& gridLines, QList<ZoneLabel>* zoneLabels = 0, QList<GridLabel>* gridLabels = 0, zoneLabelCallback_t* zoneLabelCallback = 0, gridLabelCallback_t* lineLabelCallback = 0 );
  static ZoneLabel mgrs100kIDLabelCallback ( double posX, double posY, double maxLon, double maxLat );
  static void utmGridLabelCallback ( double lon, double lat, double cellSize, bool horiz, int lineIdx, QList<GridLabel>& gridLabels );
  static void mgrsGridLabelCallback ( double lon, double lat, double cellSize, bool horiz, int lineIdx, QList<GridLabel>& gridLabels );
};

#endif // KADASLATLONTOUTM_H
