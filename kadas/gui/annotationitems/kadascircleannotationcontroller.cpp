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

QList<KadasNode> KadasCircleAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const KadasCircleAnnotationItem *circle = asCircle( item );
  return {
    { toMapPos( circle->center(), ctx ) },
    { toMapPos( circle->ringPoint(), ctx ) },
  };
}

bool KadasCircleAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  asCircle( item )->setCenter( ip );
  asCircle( item )->setRingPoint( ip );
  return true;
}

bool KadasCircleAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasCircleAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  asCircle( item )->setRingPoint( toItemPos( p, ctx ) );
}

void KadasCircleAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
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
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasCircleAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  // Two-click item: first click started, second click finishes.
  return false;
}

void KadasCircleAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasCircleAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  attributes.insert( AttrR, KadasNumericAttribute { "r" } );
  return attributes;
}

KadasAttribValues KadasCircleAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  if ( item )
  {
    const QgsPointXY ip = toItemPos( pos, ctx );
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

QgsPointXY KadasCircleAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasCircleAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const KadasCircleAnnotationItem *circle = asCircle( item );
  const QgsPointXY centerMap = toMapPos( circle->center(), ctx );
  const QgsPointXY ringMap = toMapPos( circle->ringPoint(), ctx );
  if ( pos.sqrDist( centerMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( 0, 0, 0 ), centerMap, drawAttribs() );
  if ( pos.sqrDist( ringMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( 0, 0, 1 ), ringMap, drawAttribs() );
  if ( toMapRect( circle->boundingBox(), ctx ).contains( pos ) )
    return KadasEditContext( QgsVertexId(), centerMap, KadasAttribDefs(), Qt::ArrowCursor );
  return KadasEditContext();
}

void KadasCircleAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  KadasCircleAnnotationItem *circle = asCircle( item );
  const QgsPointXY newIp = toItemPos( newPoint, ctx );
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
    const QgsPointXY oldCenterMap = toMapPos( circle->center(), ctx );
    const double dxMap = newPoint.x() - oldCenterMap.x();
    const double dyMap = newPoint.y() - oldCenterMap.y();
    const QgsPointXY newCenterMap( oldCenterMap.x() + dxMap, oldCenterMap.y() + dyMap );
    const QgsPointXY oldRingMap = toMapPos( circle->ringPoint(), ctx );
    const QgsPointXY newRingMap( oldRingMap.x() + dxMap, oldRingMap.y() + dyMap );
    circle->setCenter( toItemPos( newCenterMap, ctx ) );
    circle->setRingPoint( toItemPos( newRingMap, ctx ) );
  }
}

void KadasCircleAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasCircleAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasCircleAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasCircleAnnotationController::position( const QgsAnnotationItem *item ) const
{
  return asCircle( item )->center();
}

void KadasCircleAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
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
