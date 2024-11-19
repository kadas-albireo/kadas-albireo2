/***************************************************************************
    kadaspointitem.cpp
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

#include <QJsonArray>

#include <qgis/qgspoint.h>
#include <qgis/qgsmultipoint.h>
#include <qgis/qgsmapsettings.h>

#include "kadas/gui/mapitems/kadaspointitem.h"


QJsonObject KadasPointItem::State::serialize() const
{
  QJsonArray pts;
  for ( const KadasItemPos &pos : points )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    pts.append( p );
  }
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  json["points"] = pts;
  return json;
}

bool KadasPointItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  points.clear();
  QJsonArray pts = json["points"].toArray();
  for ( QJsonValue pValue : pts )
  {
    QJsonArray p = pValue.toArray();
    points.append( KadasItemPos( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() ) );
  }
  return true;
}

KadasPointItem::KadasPointItem( const QgsCoordinateReferenceSystem &crs, IconType icon )
  : KadasGeometryItem( crs )
{
  setIconType( icon );
  clear();
}

KadasItemPos KadasPointItem::position() const
{
  double x = 0., y = 0.;
  for ( const KadasItemPos &point : constState()->points )
  {
    x += point.x();
    y += point.y();
  }
  int n = std::max( 1, constState()->points.size() );
  return KadasItemPos( x / n, y / n );
}

void KadasPointItem::setPosition( const KadasItemPos &pos )
{
  if ( state()->points.isEmpty() )
  {
    state()->points.append( pos );
    endPart();
    recomputeDerived();
  }
  else
  {
    KadasItemPos prevPos = position();
    double dx = pos.x() - prevPos.x();
    double dy = pos.y() - prevPos.y();
    for ( KadasItemPos &point : state()->points )
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
}

bool KadasPointItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->points.append( toItemPos( firstPoint, mapSettings ) );
  recomputeDerived();
  return false;
}

bool KadasPointItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPointItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

void KadasPointItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasPointItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasPointItem::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasPointItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasPointItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasPointItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPointItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  for ( int i = 0, n = constState()->points.size(); i < n; ++i )
  {
    KadasMapPos testPos = toMapPos( constState()->points[i], mapSettings );
    if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
    {
      return EditContext( QgsVertexId( i, 0, 0 ), testPos, drawAttribs() );
    }
  }
  return EditContext();
}

void KadasPointItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part >= 0 && context.vidx.part < state()->points.size() )
  {
    state()->points[context.vidx.part] = toItemPos( newPoint, mapSettings );
    recomputeDerived();
  }
}

void KadasPointItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

KadasMapItem::AttribValues KadasPointItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasPointItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

void KadasPointItem::addPartFromGeometry( const QgsAbstractGeometry &geom )
{
  QList<const QgsPoint *> geoms;
  if ( dynamic_cast<const QgsGeometryCollection *>( &geom ) )
  {
    const QgsGeometryCollection &collection = dynamic_cast<const QgsGeometryCollection &>( geom );
    for ( int i = 0, n = collection.numGeometries(); i < n; ++i )
    {
      if ( dynamic_cast<const QgsPoint *>( collection.geometryN( i ) ) )
      {
        geoms.append( static_cast<const QgsPoint *>( collection.geometryN( i ) ) );
      }
    }
  }
  else if ( dynamic_cast<const QgsPoint *>( &geom ) )
  {
    geoms.append( static_cast<const QgsPoint *>( &geom ) );
  }
  for ( const QgsPoint *point : geoms )
  {
    state()->points.append( KadasItemPos( point->x(), point->y(), point->z() ) );
    endPart();
  }
  recomputeDerived();
}

const QgsMultiPoint *KadasPointItem::geometry() const
{
  return static_cast<QgsMultiPoint *>( mGeometry );
}

QgsMultiPoint *KadasPointItem::geometry()
{
  return static_cast<QgsMultiPoint *>( mGeometry );
}

void KadasPointItem::recomputeDerived()
{
  QgsMultiPoint *multiGeom = new QgsMultiPoint();
  for ( const KadasItemPos &point : state()->points )
  {
    multiGeom->addGeometry( new QgsPoint( point ) );
  }
  setInternalGeometry( multiGeom );
}
