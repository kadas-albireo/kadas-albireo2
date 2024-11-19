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

#include <QJsonArray>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>

#include "kadas/gui/mapitems/kadasrectangleitem.h"


QJsonObject KadasRectangleItem::State::serialize() const
{
  QJsonArray pt1;
  for ( const KadasItemPos &pos : p1 )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    pt1.append( p );
  }
  QJsonArray pt2;
  for ( const KadasItemPos &pos : p2 )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    pt2.append( p );
  }
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  json["p1"] = pt1;
  json["p2"] = pt2;
  return json;
}

bool KadasRectangleItem::State::deserialize( const QJsonObject &json )
{
  p1.clear();
  p2.clear();

  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  for ( QJsonValue val : json["p1"].toArray() )
  {
    QJsonArray pos = val.toArray();
    p1.append( KadasItemPos( pos.at( 0 ).toDouble(), pos.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue val : json["p2"].toArray() )
  {
    QJsonArray pos = val.toArray();
    p2.append( KadasItemPos( pos.at( 0 ).toDouble(), pos.at( 1 ).toDouble() ) );
  }
  return p1.size() == p2.size();
}


KadasRectangleItem::KadasRectangleItem( const QgsCoordinateReferenceSystem &crs )
  : KadasGeometryItem( crs )
{
  clear();
}

KadasItemPos KadasRectangleItem::position() const
{
  double x = 0., y = 0.;
  for ( const KadasItemPos &point : constState()->p1 )
  {
    x += point.x();
    y += point.y();
  }
  for ( const KadasItemPos &point : constState()->p2 )
  {
    x += point.x();
    y += point.y();
  }
  int n = std::max( 1, constState()->p1.size() + constState()->p2.size() );
  return KadasItemPos( x / n, y / n );
}

void KadasRectangleItem::setPosition( const KadasItemPos &pos )
{
  KadasItemPos prevPos = position();
  double dx = pos.x() - prevPos.x();
  double dy = pos.y() - prevPos.y();
  for ( KadasItemPos &point : state()->p1 )
  {
    point.setX( point.x() + dx );
    point.setY( point.y() + dy );
  }
  for ( KadasItemPos &point : state()->p2 )
  {
    point.setX( point.x() + dx );
    point.setY( point.y() + dy );
  }
  if ( mGeometry )
  {
    mGeometry->transformVertices( [dx, dy]( const QgsPoint &p ) { return QgsPoint( p.x() + dx, p.y() + dy ); } );
  }
  update();
}

bool KadasRectangleItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  KadasItemPos itemPos = toItemPos( firstPoint, mapSettings );
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->p1.append( itemPos );
  state()->p2.append( itemPos );
  recomputeDerived();
  return true;
}

bool KadasRectangleItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasRectangleItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  state()->p2.last() = toItemPos( p, mapSettings );
  recomputeDerived();
}

void KadasRectangleItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  setCurrentPoint( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

bool KadasRectangleItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasRectangleItem::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasRectangleItem::drawAttribs() const
{
  KadasMapItem::AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  return attributes;
}

KadasMapPos KadasRectangleItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::AttribValues KadasRectangleItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapItem::EditContext KadasRectangleItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  for ( int iPart = 0, nParts = constState()->p1.size(); iPart < nParts; ++iPart )
  {
    QList<KadasItemPos> points = QList<KadasItemPos>()
                                 << constState()->p1[iPart]
                                 << KadasItemPos( constState()->p2[iPart].x(), constState()->p1[iPart].y() )
                                 << constState()->p2[iPart]
                                 << KadasItemPos( constState()->p1[iPart].x(), constState()->p2[iPart].y() );
    for ( int iVert = 0, nVerts = points.size(); iVert < nVerts; ++iVert )
    {
      KadasMapPos testPos = toMapPos( points[iVert], mapSettings );
      if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
      {
        return EditContext( QgsVertexId( iPart, 0, iVert ), testPos, drawAttribs() );
      }
    }
  }
  if ( intersects( KadasMapRect( pos, pickTol( mapSettings ) ), mapSettings ) )
  {
    KadasMapPos refPos = toMapPos( constState()->p1.front(), mapSettings );
    return EditContext( QgsVertexId(), refPos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasRectangleItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  KadasItemPos newItemPos = toItemPos( newPoint, mapSettings );
  if ( context.vidx.part >= 0 && context.vidx.part < state()->p1.size()
       && context.vidx.vertex >= 0 && context.vidx.vertex < 4 )
  {
    if ( context.vidx.vertex == 0 )
    {
      state()->p1[context.vidx.part] = newItemPos;
    }
    else if ( context.vidx.vertex == 1 )
    {
      state()->p2[context.vidx.part].setX( newItemPos.x() );
      state()->p1[context.vidx.part].setY( newItemPos.y() );
    }
    else if ( context.vidx.vertex == 2 )
    {
      state()->p2[context.vidx.part] = newItemPos;
    }
    else if ( context.vidx.vertex == 3 )
    {
      state()->p1[context.vidx.part].setX( newItemPos.x() );
      state()->p2[context.vidx.part].setY( newItemPos.y() );
    }
    recomputeDerived();
  }
  else
  {
    // Move geometry a whole
    KadasMapPos refMapPos = toMapPos( constState()->p1.front(), mapSettings );
    for ( KadasItemPos &pos : state()->p1 )
    {
      KadasMapPos mapPos = toMapPos( pos, mapSettings );
      pos = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
    }
    for ( KadasItemPos &pos : state()->p2 )
    {
      KadasMapPos mapPos = toMapPos( pos, mapSettings );
      pos = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
    }
    recomputeDerived();
  }
}

void KadasRectangleItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

KadasMapItem::AttribValues KadasRectangleItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasRectangleItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

void KadasRectangleItem::addPartFromGeometry( const QgsAbstractGeometry &geom )
{
  QList<const QgsPolygon *> geoms;
  if ( dynamic_cast<const QgsGeometryCollection *>( &geom ) )
  {
    const QgsGeometryCollection &collection = dynamic_cast<const QgsGeometryCollection &>( geom );
    for ( int i = 0, n = collection.numGeometries(); i < n; ++i )
    {
      if ( dynamic_cast<const QgsPolygon *>( collection.geometryN( i ) ) )
      {
        geoms.append( static_cast<const QgsPolygon *>( collection.geometryN( i ) ) );
      }
    }
  }
  else if ( dynamic_cast<const QgsPolygon *>( &geom ) )
  {
    geoms.append( static_cast<const QgsPolygon *>( &geom ) );
  }
  for ( const QgsPolygon *poly : geoms )
  {
    QgsRectangle bbox = poly->boundingBox();
    state()->p1.append( KadasItemPos( bbox.xMinimum(), bbox.yMinimum() ) );
    state()->p2.append( KadasItemPos( bbox.xMaximum(), bbox.yMaximum() ) );
    endPart();
  }
  recomputeDerived();
}

const QgsMultiPolygon *KadasRectangleItem::geometry() const
{
  return static_cast<QgsMultiPolygon *>( mGeometry );
}

QgsMultiPolygon *KadasRectangleItem::geometry()
{
  return static_cast<QgsMultiPolygon *>( mGeometry );
}

void KadasRectangleItem::measureGeometry()
{
  double totalArea = 0;
  for ( int i = 0, n = geometry()->numGeometries(); i < n; ++i )
  {
    const QgsPolygon *polygon = static_cast<QgsPolygon *>( geometry()->geometryN( i ) );

    double area = mDa.measureArea( QgsGeometry( polygon->clone() ) );
    QStringList measurements;
    measurements.append( formatArea( area, areaBaseUnit() ) );

    const KadasItemPos &p1 = state()->p1[i];
    const KadasItemPos &p2 = state()->p2[i];
    QString width = formatLength( mDa.measureLine( p1, KadasItemPos( p2.x(), p1.y() ) ), distanceBaseUnit() );
    QString height = formatLength( mDa.measureLine( p1, KadasItemPos( p1.x(), p2.y() ) ), distanceBaseUnit() );
    measurements.append( QString( "(%1 x %2)" ).arg( width ).arg( height ) );

    addMeasurements( measurements, KadasItemPos::fromPoint( polygon->centroid() ) );
    totalArea += area;
  }
  mTotalMeasurement = formatArea( totalArea, areaBaseUnit() );
}

void KadasRectangleItem::recomputeDerived()
{
  QgsGeometryCollection *multiGeom = new QgsMultiPolygon();
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    const KadasItemPos &p1 = state()->p1[i];
    const KadasItemPos &p2 = state()->p2[i];
    QgsLineString *ring = new QgsLineString();
    ring->addVertex( QgsPoint( p1 ) );
    ring->addVertex( QgsPoint( p2.x(), p1.y() ) );
    ring->addVertex( QgsPoint( p2 ) );
    ring->addVertex( QgsPoint( p1.x(), p2.y() ) );
    ring->addVertex( QgsPoint( p1 ) );
    QgsPolygon *poly = new QgsPolygon();
    poly->setExteriorRing( ring );
    multiGeom->addGeometry( poly );
  }
  setInternalGeometry( multiGeom );
}
