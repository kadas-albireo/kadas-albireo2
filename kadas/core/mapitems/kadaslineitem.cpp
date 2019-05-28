/***************************************************************************
    kadaslineitem.cpp
    -----------------
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

#include <qmath.h>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadaslineitem.h>

void KadasLineItem::State::assign(const KadasMapItem::State* other) {
  *this = *static_cast<const State*>(other);
}

KadasLineItem::KadasLineItem(const QgsCoordinateReferenceSystem &crs, bool geodesic, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  mGeodesic = geodesic;
  clear();
}

bool KadasLineItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->points.append(QList<QgsPointXY>());
  state()->points.last().append(firstPoint);
  state()->points.last().append(firstPoint);
  recomputeDerived();
  return true;
}

bool KadasLineItem::startPart(const AttribValues& values)
{
  return startPart(QgsPointXY(values[AttrX], values[AttrY]));
}

void KadasLineItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings* mapSettings)
{
  state()->points.last().last() = p;
  recomputeDerived();
}

void KadasLineItem::setCurrentAttributes(const AttribValues& values)
{
  setCurrentPoint(QgsPointXY(values[AttrX], values[AttrY]));
}

bool KadasLineItem::continuePart()
{
  // If current point is same as last one, drop last point and end geometry
  int n = state()->points.last().size();
  if(n > 2 && state()->points.last()[n - 1] == state()->points.last()[n - 2]) {
    state()->points.last().removeLast();
    recomputeDerived();
    return false;
  }
  state()->points.last().append(state()->points.last().last());
  recomputeDerived();
  return true;
}


void KadasLineItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasLineItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x"});
  attributes.insert(AttrY, NumericAttribute{"y"});
  return attributes;
}

KadasMapItem::AttribValues KadasLineItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasLineItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasLineItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  QgsCoordinateTransform crst(mCrs, mapSettings.destinationCrs(), mapSettings.transformContext());
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform(crst.transform(pos));
  for(int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart) {
    const QList<QgsPointXY> part = state()->points[iPart];
    for(int iVert = 0, nVerts = part.size(); iVert < nVerts; ++iVert) {
      QgsPointXY testPos = mapSettings.mapToPixel().transform(crst.transform(part[iVert]));
      if ( canvasPos.sqrDist(testPos) < 25 ) {
        return EditContext(QgsVertexId(iPart, 0, iVert), part[iVert], drawAttribs());
      }
    }
  }
  return EditContext();
}

void KadasLineItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings)
{
  if(context.vidx.part >= 0 && context.vidx.part < state()->points.size()
  && context.vidx.vertex >= 0 && context.vidx.vertex < state()->points[context.vidx.part].size()) {
    state()->points[context.vidx.part][context.vidx.vertex] = newPoint;
    recomputeDerived();
  }
}

void KadasLineItem::edit(const EditContext& context, const AttribValues& values)
{
  edit(context, QgsPointXY(values[AttrX], values[AttrY]));
}

KadasMapItem::AttribValues KadasLineItem::editAttribsFromPosition(const EditContext& context, const QgsPointXY& pos) const
{
  return drawAttribsFromPosition(pos);
}

QgsPointXY KadasLineItem::positionFromEditAttribs(const EditContext& context, const AttribValues& values) const
{
  return positionFromDrawAttribs(values);
}

const QgsMultiLineString* KadasLineItem::geometry() const
{
  return static_cast<QgsMultiLineString*>(mGeometry);
}

QgsMultiLineString* KadasLineItem::geometry()
{
  return static_cast<QgsMultiLineString*>(mGeometry);
}

void KadasLineItem::setMeasurementMode(MeasurementMode measurementMode, QgsUnitTypes::AngleUnit angleUnit)
{
  mMeasurementMode = measurementMode;
  mAngleUnit = angleUnit;
  emit geometryChanged(); // Trigger re-measurement
}

void KadasLineItem::measureGeometry()
{
  double totalLength = 0;
  for(int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart) {
    const QList<QgsPointXY>& part = state()->points[iPart];
    if(part.size() < 2) {
      continue;
    }

    switch(mMeasurementMode) {
    case MeasureLineAndSegments: {
      double totLength = 0;
      for(int i = 1, n = part.size(); i < n; ++i) {
        const QgsPointXY& p1 = part[i - 1];
        const QgsPointXY& p2 = part[i];
        double length = mDa.measureLine(p1, p2);
        totLength += length;
        addMeasurements( QStringList() << formatLength( length, distanceBaseUnit() ), QgsPointXY( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
      }
      QString totLengthStr = tr( "Tot.: %1" ).arg( formatLength( totLength, distanceBaseUnit() ) );
      addMeasurements( QStringList() << totLengthStr, part.last(), false );
      totalLength += totLength;
      break;
    }
    case MeasureAzimuthGeoNorth:
    case MeasureAzimuthMapNorth: {
      for(int i = 1, n = part.size(); i < n; ++i) {
        const QgsPointXY& p1 = part[i - 1];
        const QgsPointXY& p2 = part[i];
        double angle = 0;
        if ( mMeasurementMode == MeasureAzimuthGeoNorth )
        {
          angle = mDa.bearing( p1, p2 );
        }
        else
        {
          angle = qAtan2( p2.x() - p1.x(), p2.y() - p1.y() );
        }
        angle = qRound( angle *  1000 ) / 1000.;
        angle = angle < 0 ? angle + 2 * M_PI : angle;
        angle = angle >= 2 * M_PI ? angle - 2 * M_PI : angle;
        QString segmentAngle = formatAngle( angle, mAngleUnit );
        addMeasurements( QStringList() << segmentAngle, QgsPointXY( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
      }
      break;
    }
    }
  }
  mTotalMeasurement = formatLength(totalLength, distanceBaseUnit());
}

void KadasLineItem::recomputeDerived()
{
  QgsMultiLineString* multiGeom = new QgsMultiLineString();

  if ( mGeodesic )
  {
    QgsCoordinateTransform t1( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326"), QgsProject::instance() );
    QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326"), mCrs, QgsProject::instance() );
    GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );

    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<QgsPointXY>& part = state()->points[iPart];
      QgsLineString* ring = new QgsLineString();

      int nPoints = part.size();
      if ( nPoints >= 2 )
      {
        QList<QgsPointXY> wgsPoints;
        for ( const QgsPointXY& point : part )
        {
          wgsPoints.append( t1.transform( point ) );
        }

        double sdist = 500000; // 500km segments
        for ( int i = 0; i < nPoints - 1; ++i )
        {
          int ringSize = ring->vertexCount();
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i+1].y(), wgsPoints[i+1].x() );
          double dist = line.Distance();
          int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
          for ( int j = 0; j < nIntervals; ++j )
          {
            double lat, lon;
            line.Position( j * sdist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
          if ( i == nPoints - 2 )
          {
            double lat, lon;
            line.Position( dist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
        }
        ring->addVertex( QgsPoint(part.front()) );
      }
      multiGeom->addGeometry( ring );
    }
  }
  else
  {
    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<QgsPointXY>& part = state()->points[iPart];
      QgsLineString* ring = new QgsLineString();
      for ( const QgsPointXY& point : part )
      {
        ring->addVertex( QgsPoint(point) );
      }
      multiGeom->addGeometry( ring );
    }
  }
  setGeometry(multiGeom);
}
