/***************************************************************************
    kadasrectangleitem.cpp
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


#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>

#include <kadas/core/mapitems/kadasrectangleitem.h>


KadasRectangleItem::KadasRectangleItem(const QgsCoordinateReferenceSystem &crs, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  clear();
}

bool KadasRectangleItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->p1.append(firstPoint);
  state()->p2.append(firstPoint);
  recomputeDerived();
  return true;
}

bool KadasRectangleItem::startPart(const AttribValues& attributeValues)
{
  QgsPoint point(attributeValues[AttrX], attributeValues[AttrY]);
  return startPart(point);
}

void KadasRectangleItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  state()->p2.last() = p;
  recomputeDerived();
}

void KadasRectangleItem::setCurrentAttributes(const AttribValues& values)
{
  state()->p2.last().setX(values[AttrX]);
  state()->p2.last().setY(values[AttrY]);
  recomputeDerived();
}

bool KadasRectangleItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasRectangleItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasRectangleItem::drawAttribs() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  KadasMapItem::AttribDefs attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  return attributes;
}

QgsPointXY KadasRectangleItem::positionFromDrawAttribs(const AttribValues& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::AttribValues KadasRectangleItem::drawAttribsFromPosition(const QgsPointXY& pos) const
{
  AttribValues values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

KadasMapItem::EditContext KadasRectangleItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  QgsCoordinateTransform crst(mCrs, mapSettings.destinationCrs(), mapSettings.transformContext());
  QgsPointXY canvasPos = mapSettings.mapToPixel().transform(crst.transform(pos));
  for(int iPart = 0, nParts = state()->p1.size(); iPart < nParts; ++iPart) {
    QList<QgsPointXY> points = QList<QgsPointXY>()
          << state()->p1[iPart]
          << QgsPoint( state()->p2[iPart].x(), state()->p1[iPart].y() )
          << state()->p2[iPart]
          << QgsPoint( state()->p1[iPart].x(), state()->p2[iPart].y() );
    for(int iVert = 0, nVerts = points.size(); iVert < nVerts; ++iVert) {
      QgsPointXY testPos = mapSettings.mapToPixel().transform(crst.transform(points[iVert]));
      if ( canvasPos.sqrDist(testPos) < 25 ) {
        return EditContext(QgsVertexId(iPart, 0, iVert));
      }
    }
  }
  return EditContext();
}

void KadasRectangleItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings& mapSettings)
{
  if(context.vidx.part >= 0 && context.vidx.part < state()->p1.size()
  && context.vidx.vertex >= 0 && context.vidx.vertex < 4) {
    if(context.vidx.vertex == 0) {
      state()->p1[context.vidx.part] = newPoint;
    } else if(context.vidx.vertex == 1) {
      state()->p2[context.vidx.part].setX( newPoint.x() );
      state()->p1[context.vidx.part].setY( newPoint.y() );
    } else if ( context.vidx.vertex == 2 ) {
      state()->p2[context.vidx.part] = newPoint;
    } else if ( context.vidx.vertex == 3 ) {
      state()->p1[context.vidx.part].setX( newPoint.x() );
      state()->p2[context.vidx.part].setY( newPoint.y() );
    }
    recomputeDerived();
  }
}

const QgsMultiPolygon* KadasRectangleItem::geometry() const
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

QgsMultiPolygon* KadasRectangleItem::geometry()
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

void KadasRectangleItem::measureGeometry()
{
  double totalArea = 0;
  for(int i = 0, n = geometry()->numGeometries(); i < n; ++i) {
    const QgsPolygon* polygon = static_cast<QgsPolygon*>(geometry()->geometryN(i));

    double area = mDa.measureArea( QgsGeometry( polygon->clone() ) );
    QStringList measurements;
    measurements.append( formatArea(area, areaBaseUnit()) );

    const QgsPointXY& p1 = state()->p1[i];
    const QgsPointXY& p2 = state()->p2[i];
    QString width = formatLength( mDa.measureLine( p1, QgsPoint( p2.x(), p1.y() ) ), distanceBaseUnit() );
    QString height = formatLength( mDa.measureLine( p1, QgsPoint( p1.x(), p2.y() ) ), distanceBaseUnit() );
    measurements.append( QString( "(%1 x %2)" ).arg( width ).arg( height ) );

    addMeasurements( measurements, polygon->centroid() );
    totalArea += area;
  }
  mTotalMeasurement = formatArea(totalArea, areaBaseUnit());
}

void KadasRectangleItem::recomputeDerived()
{
  QgsGeometryCollection* multiGeom = new QgsMultiPolygon();
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    const QgsPointXY& p1 = state()->p1[i];
    const QgsPointXY& p2 = state()->p2[i];
    QgsLineString* ring = new QgsLineString();
    ring->addVertex( QgsPoint( p1 ) );
    ring->addVertex( QgsPoint( p2.x(), p1.y() ) );
    ring->addVertex( QgsPoint( p2 ) );
    ring->addVertex( QgsPoint( p1.x(), p2.y() ) );
    ring->addVertex( QgsPoint( p1 ) );
    QgsPolygon* poly = new QgsPolygon();
    poly->setExteriorRing( ring );
    multiGeom->addGeometry( poly );
  }
  setGeometry(multiGeom);
}
