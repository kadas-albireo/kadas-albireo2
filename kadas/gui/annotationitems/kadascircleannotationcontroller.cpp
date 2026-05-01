/***************************************************************************
    kadascircleannotationcontroller.cpp
    -----------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>

#include <QObject>
#include <QTextStream>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadascircleannotationcontroller.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"

namespace
{
  inline KadasCircleAnnotationItem *asCircle( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == KadasCircleAnnotationItem::itemTypeId() );
    return static_cast<KadasCircleAnnotationItem *>( item );
  }
  inline const KadasCircleAnnotationItem *asCircle( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == KadasCircleAnnotationItem::itemTypeId() );
    return static_cast<const KadasCircleAnnotationItem *>( item );
  }
} // namespace


QString KadasCircleAnnotationController::itemType() const
{
  return KadasCircleAnnotationItem::itemTypeId();
}

QString KadasCircleAnnotationController::itemName() const
{
  return QObject::tr( "Circle" );
}

QgsAnnotationItem *KadasCircleAnnotationController::createItem() const
{
  auto *item = new KadasCircleAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::Circle );
  return item;
}

QList<KadasMapItem::Node> KadasCircleAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const KadasCircleAnnotationItem *circle = asCircle( item );
  return {
    { toMapPos( KadasItemPos::fromPoint( circle->center() ), ctx ) },
    { toMapPos( KadasItemPos::fromPoint( circle->ringPoint() ), ctx ) },
  };
}

bool KadasCircleAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const KadasItemPos ip = toItemPos( firstPoint, ctx );
  asCircle( item )->setCenter( ip );
  asCircle( item )->setRingPoint( ip );
  return true;
}

bool KadasCircleAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

void KadasCircleAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const KadasMapPos &p, const KadasAnnotationItemContext &ctx )
{
  asCircle( item )->setRingPoint( toItemPos( p, ctx ) );
}

void KadasCircleAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  // Interpret AttrX/AttrY as the desired ring point in map CRS, or AttrR as a
  // radius around the (already-set) center.
  KadasCircleAnnotationItem *c = asCircle( item );
  if ( values.contains( AttrR ) )
  {
    const QgsPointXY center = c->center();
    const double r = values[AttrR];
    c->setRingPoint( QgsPointXY( center.x() + r, center.y() ) );
    return;
  }
  setCurrentPoint( item, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

bool KadasCircleAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  // Two-click item: first click started, second click finishes.
  return false;
}

void KadasCircleAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasMapItem::AttribDefs KadasCircleAnnotationController::drawAttribs() const
{
  KadasMapItem::AttribDefs attributes;
  attributes.insert( AttrX, KadasMapItem::NumericAttribute { "x" } );
  attributes.insert( AttrY, KadasMapItem::NumericAttribute { "y" } );
  attributes.insert( AttrR, KadasMapItem::NumericAttribute { "r" } );
  return attributes;
}

KadasMapItem::AttribValues KadasCircleAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const
{
  KadasMapItem::AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  if ( item )
  {
    const KadasItemPos ip = toItemPos( pos, ctx );
    const QgsPointXY c = asCircle( item )->center();
    const double dx = ip.x() - c.x();
    const double dy = ip.y() - c.y();
    values.insert( AttrR, std::sqrt( dx * dx + dy * dy ) );
  }
  else
  {
    values.insert( AttrR, 0.0 );
  }
  return values;
}

KadasMapPos KadasCircleAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext & ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasCircleAnnotationController::getEditContext( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const
{
  const KadasCircleAnnotationItem *circle = asCircle( item );
  const KadasMapPos centerMap = toMapPos( KadasItemPos::fromPoint( circle->center() ), ctx );
  const KadasMapPos ringMap = toMapPos( KadasItemPos::fromPoint( circle->ringPoint() ), ctx );
  if ( pos.sqrDist( centerMap ) < pickTolSqr( ctx ) )
    return KadasMapItem::EditContext( QgsVertexId( 0, 0, 0 ), centerMap, drawAttribs() );
  if ( pos.sqrDist( ringMap ) < pickTolSqr( ctx ) )
    return KadasMapItem::EditContext( QgsVertexId( 0, 0, 1 ), ringMap, drawAttribs() );
  if ( toMapRect( circle->boundingBox(), ctx ).contains( pos ) )
    return KadasMapItem::EditContext( QgsVertexId(), centerMap, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  return KadasMapItem::EditContext();
}

void KadasCircleAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx )
{
  KadasCircleAnnotationItem *circle = asCircle( item );
  const KadasItemPos newIp = toItemPos( newPoint, ctx );
  if ( editContext.vidx.vertex == 0 )
  {
    // Move center; keep radius (translate ring point by the same delta).
    const QgsPointXY oldC = circle->center();
    const QgsPointXY oldR = circle->ringPoint();
    const double dx = newIp.x() - oldC.x();
    const double dy = newIp.y() - oldC.y();
    circle->setCenter( newIp );
    circle->setRingPoint( QgsPointXY( oldR.x() + dx, oldR.y() + dy ) );
  }
  else if ( editContext.vidx.vertex == 1 )
  {
    circle->setRingPoint( newIp );
  }
  else
  {
    // Whole-item move: shift both points by the map-space delta.
    const KadasMapPos oldCenterMap = toMapPos( KadasItemPos::fromPoint( circle->center() ), ctx );
    const double dxMap = newPoint.x() - oldCenterMap.x();
    const double dyMap = newPoint.y() - oldCenterMap.y();
    const KadasMapPos newCenterMap( oldCenterMap.x() + dxMap, oldCenterMap.y() + dyMap );
    const KadasMapPos oldRingMap = toMapPos( KadasItemPos::fromPoint( circle->ringPoint() ), ctx );
    const KadasMapPos newRingMap( oldRingMap.x() + dxMap, oldRingMap.y() + dyMap );
    circle->setCenter( toItemPos( newCenterMap, ctx ) );
    circle->setRingPoint( toItemPos( newRingMap, ctx ) );
  }
}

void KadasCircleAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

KadasMapItem::AttribValues KadasCircleAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx
) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

KadasMapPos KadasCircleAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

KadasItemPos KadasCircleAnnotationController::position( const QgsAnnotationItem *item ) const
{
  return KadasItemPos::fromPoint( asCircle( item )->center() );
}

void KadasCircleAnnotationController::setPosition( QgsAnnotationItem *item, const KadasItemPos &pos )
{
  KadasCircleAnnotationItem *circle = asCircle( item );
  const QgsPointXY oldC = circle->center();
  const double dx = pos.x() - oldC.x();
  const double dy = pos.y() - oldC.y();
  circle->setCenter( pos );
  const QgsPointXY oldR = circle->ringPoint();
  circle->setRingPoint( QgsPointXY( oldR.x() + dx, oldR.y() + dy ) );
}

void KadasCircleAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  KadasCircleAnnotationItem *circle = asCircle( item );
  const QgsPointXY c = circle->center();
  const QgsPointXY r = circle->ringPoint();
  circle->setCenter( QgsPointXY( c.x() + dx, c.y() + dy ) );
  circle->setRingPoint( QgsPointXY( r.x() + dx, r.y() + dy ) );
}

QString KadasCircleAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const KadasCircleAnnotationItem *circle = asCircle( item );
  const double r = circle->radius();
  if ( r <= 0 )
    return QString();

  QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<Polygon>\n<outerBoundaryIs><LinearRing>\n<coordinates>";
  constexpr int segments = 64;
  for ( int i = 0; i <= segments; ++i )
  {
    const double a = ( 2.0 * M_PI * i ) / segments;
    QgsPointXY p( circle->center().x() + r * std::cos( a ), circle->center().y() + r * std::sin( a ) );
    p = ct.transform( p );
    if ( i > 0 )
      outStream << " ";
    outStream << QString::number( p.x(), 'f', 10 ) << "," << QString::number( p.y(), 'f', 10 );
  }
  outStream << "</coordinates>\n</LinearRing></outerBoundaryIs>\n</Polygon>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
