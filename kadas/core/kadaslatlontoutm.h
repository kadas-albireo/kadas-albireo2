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
#include <QList>
#include <QPair>
#include <QPolygonF>

#include <qgis/qgis_sip.h>
#include "kadas/core/kadas_core.h"

class QgsPointXY;
class QgsRectangle;

class KADAS_CORE_EXPORT KadasLatLonToUTM
{
  public:
    struct UTMCoo
    {
      int easting;
      int northing;
      int zoneNumber;
      QString zoneLetter;
    };

    struct MGRSCoo
    {
      int easting;
      int northing;
      int zoneNumber;
      QString zoneLetter;
      QString letter100kID;
    };

    struct ZoneLabel
    {
      QPointF pos;
      QString label;
      QPointF maxPos;
    };

    struct GridLabel
    {
      QString label;
      int lineIdx;
      bool horiz;
    };

    struct Grid
    {
      QList<QPolygonF> zoneLines;
      QList<QPolygonF> subZoneLines;
      QList<QPolygonF> gridLines;
      QList<KadasLatLonToUTM::ZoneLabel> zoneLabels;
      QList<KadasLatLonToUTM::ZoneLabel> subZoneLabel;
      QList<KadasLatLonToUTM::GridLabel> gridLabels;

      friend Grid& operator<<( Grid &lhs, const Grid& rhs ) SIP_SKIP
      {
        lhs.zoneLines << rhs.zoneLines;
        lhs.subZoneLines << rhs.subZoneLines;
        lhs.gridLines << rhs.gridLines;
        lhs.zoneLabels << rhs.zoneLabels;
        lhs.subZoneLabel << rhs.subZoneLabel;
        lhs.gridLabels << rhs.gridLabels;
        return lhs;
      }
    };

    static QgsPointXY UTM2LL( const UTMCoo &utm, bool &ok );
    static UTMCoo LL2UTM( const QgsPointXY &pLatLong );
    static MGRSCoo UTM2MGRS( const UTMCoo &utmcoo );
    static UTMCoo MGRS2UTM( const MGRSCoo &mgrs, bool &ok );

    static int getZoneNumber( double lon, double lat );
    static QString getHemisphereLetter( double lat );

    enum class GridMode SIP_MONKEYPATCH_SCOPEENUM
    {
      GridUTM,
      GridMGRS
    };

    static Grid computeGrid( const QgsRectangle &bbox, double mapScale, KadasLatLonToUTM::GridMode gridMode, int cellSize );

  private:
    static const int NUM_100K_SETS;
    static const QString SET_ORIGIN_COLUMN_LETTERS;
    static const QString SET_ORIGIN_ROW_LETTERS;

    static QString getLetter100kID( int column, int row, int parm );
    static double getMinNorthing( int zoneLetter );
    typedef ZoneLabel( zoneLabelCallback_t )( double, double, double, double );
    typedef void ( gridLabelCallback_t )( double, double, int, bool, int, QList<GridLabel> & );
    static Grid computeSubGrid( int cellSize, double xMin, double xMax, double yMin, double yMax, zoneLabelCallback_t *zoneLabelCallback = nullptr, gridLabelCallback_t *lineLabelCallback = nullptr );
    static ZoneLabel mgrs100kIDLabelCallback( double posX, double posY, double maxLon, double maxLat );
    static void utmGridLabelCallback( double lon, double lat, int cellSize, bool horiz, int lineIdx, QList<GridLabel> &gridLabels );
    static void mgrsGridLabelCallback( double lon, double lat, int cellSize, bool horiz, int lineIdx, QList<GridLabel> &gridLabels );
};

#endif // KADASLATLONTOUTM_H
