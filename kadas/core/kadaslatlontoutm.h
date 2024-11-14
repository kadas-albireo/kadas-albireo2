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

    enum class Level
    {
      Major,
      Minor,
      OnlyLabels
    };

    struct Grid
    {
      QList<KadasLatLonToUTM::ZoneLabel> zoneLabels;

      QList<std::pair<Level,QPolygonF>> lines;
      QList<KadasLatLonToUTM::GridLabel> gridLabels;

      friend Grid& operator<<( Grid &lhs, const Grid& rhs ) SIP_SKIP
      {
        int lineCount = lhs.lines.count();
        int labelCount = lhs.gridLabels.count();

        lhs.zoneLabels << rhs.zoneLabels;
        lhs.lines << rhs.lines;
        lhs.gridLabels << rhs.gridLabels;
        for ( int i = labelCount; i < lhs.gridLabels.count(); i++ )
          lhs.gridLabels[i].lineIdx += lineCount;
        return lhs;
      }
    };

    static QgsPointXY UTM2LL( const UTMCoo &utm, bool &ok );
    static UTMCoo LL2UTM( const QgsPointXY &pLatLong );
    static MGRSCoo UTM2MGRS( const UTMCoo &utmcoo );
    static UTMCoo MGRS2UTM( const MGRSCoo &mgrs, bool &ok );

    static int zoneNumber( double lon, double lat );
    static QString hemisphereLetter( double lat );
    static QString zoneName( double lon, double lat );

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

    static QString mgrsLetter100kID( int column, int row, int parm );
    static double minNorthing( int zoneLetter );
    typedef ZoneLabel( zoneLabelCallback_t )( double, double, double, double );
    typedef GridLabel ( gridLabelCallback_t )( double, double, int, bool, int );
    static Grid computeSubGrid( int cellSize, Level level, double xMin, double xMax, double yMin, double yMax, zoneLabelCallback_t *zoneLabelCallback = nullptr, gridLabelCallback_t *lineLabelCallback = nullptr );
    static ZoneLabel mgrs100kIDLabelCallback( double posX, double posY, double maxLon, double maxLat );
    static GridLabel utmGridLabelCallback(double lon, double lat, int cellSize, bool horiz, int lineIdx);
    static GridLabel mgrsGridLabelCallback(double lon, double lat, int cellSize, bool horiz, int lineIdx );
};

#endif // KADASLATLONTOUTM_H
