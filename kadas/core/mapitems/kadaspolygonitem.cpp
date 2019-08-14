/***************************************************************************
    kadaspolygonitem.cpp
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

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadaspolygonitem.h>


KadasPolygonItem::KadasPolygonItem(const QgsCoordinateReferenceSystem &crs, bool geodesic, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  mGeodesic = geodesic;
  clear();
}

QList<KadasMapItem::Node> KadasPolygonItem::nodes(const QgsMapSettings &settings) const
{
  QList<Node> nodes;
  for(const QList<QgsPointXY>& part : state()->points) {
    for(const QgsPointXY& pos : part) {
      nodes.append({pos});
    }
  }
  return nodes;
}

bool KadasPolygonItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->points.append(QList<QgsPointXY>());
  state()->points.last().append(firstPoint);
  state()->points.last().append(firstPoint);
  recomputeDerived();
  return true;
}

bool KadasPolygonItem::startPart(const AttribValues& values)
{
  return startPart(QgsPointXY(values[AttrX], values[AttrY]));
}

void KadasPolygonItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings* mapSettings)
{
  state()->points.last().last() = p;
  recomputeDerived();
}

void KadasPolygonItem::setCurrentAttributes(const AttribValues& values)
{
  return setCurrentPoint(QgsPoint(values[AttrX], values[AttrY]));
}

bool KadasPolygonItem::continuePart()
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

void KadasPolygonItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasPolygonItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x"});
  attributes.insert(AttrY, NumericAttribute{"y"});
  return attributes;
}

KadasMapItem::AttribValues KadasPolygonItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasPolygonItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasPolygonItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
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

void KadasPolygonItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings)
{
  if(context.vidx.part >= 0 && context.vidx.part < state()->points.size()
  && context.vidx.vertex >= 0 && context.vidx.vertex < state()->points[context.vidx.part].size()) {
    state()->points[context.vidx.part][context.vidx.vertex] = newPoint;
    recomputeDerived();
  }
}

void KadasPolygonItem::edit(const EditContext& context, const AttribValues& values)
{
  edit(context, QgsPointXY(values[AttrX], values[AttrY]));
}

KadasMapItem::AttribValues KadasPolygonItem::editAttribsFromPosition(const EditContext& context, const QgsPointXY& pos) const
{
  return drawAttribsFromPosition(pos);
}

QgsPointXY KadasPolygonItem::positionFromEditAttribs(const EditContext& context, const AttribValues& values, const QgsMapSettings &mapSettings) const
{
  return positionFromDrawAttribs(values);
}

void KadasPolygonItem::addPartFromGeometry(const QgsAbstractGeometry *geom)
{
  if(dynamic_cast<const QgsPolygon*>(geom)) {
    QList<QgsPointXY> points;
    QgsVertexId vidx;
    QgsPoint p;
    const QgsCurve* ring = static_cast<const QgsPolygon*>(geom)->exteriorRing();
    while(ring->nextVertex(vidx, p)) {
      points.append(p);
    }
    state()->points.append(points);
    recomputeDerived();
    endPart();
  }
}

const QgsMultiPolygon* KadasPolygonItem::geometry() const
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

QgsMultiPolygon* KadasPolygonItem::geometry()
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

void KadasPolygonItem::measureGeometry()
{
  double totalArea = 0;
  for(int i = 0, n = geometry()->numGeometries(); i < n; ++i) {
    const QgsPolygon* polygon = static_cast<QgsPolygon*>(geometry()->geometryN(i));

    double area = mDa.measureArea( QgsGeometry( polygon->clone() ) );
    addMeasurements( QStringList() << formatArea(area, areaBaseUnit()), polygon->centroid() );
    totalArea += area;
  }
  mTotalMeasurement = formatArea(totalArea, areaBaseUnit());
}

void KadasPolygonItem::recomputeDerived()
{
  QgsMultiPolygon* multiGeom = new QgsMultiPolygon();

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
        wgsPoints.append( t1.transform(part.front()));

        double sdist = 500000; // 500km segments
        for ( int i = 0; i < nPoints; ++i )
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
          if ( i == nPoints - 1 )
          {
            double lat, lon;
            line.Position( dist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
        }
      }
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
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
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
  setInternalGeometry(multiGeom);
}
