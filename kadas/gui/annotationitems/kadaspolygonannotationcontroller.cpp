/***************************************************************************
    kadaspolygonannotationcontroller.cpp
    ------------------------------------
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

#include <QObject>
#include <QTextStream>

#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaspolygonannotationcontroller.h"

namespace
{
  inline QgsAnnotationPolygonItem *asPolygon( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "polygon" ) );
    return static_cast<QgsAnnotationPolygonItem *>( item );
  }
  inline const QgsAnnotationPolygonItem *asPolygon( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "polygon" ) );
    return static_cast<const QgsAnnotationPolygonItem *>( item );
  }

  //! Returns the mutable exterior LineString of the polygon, replacing the polygon with a clone if needed.
  //! May return nullptr if the polygon has no exterior ring or it is not a LineString-backed curve.
  QgsLineString *takeMutableExterior( QgsAnnotationPolygonItem *item )
  {
    const QgsCurvePolygon *poly = item->geometry();
    if ( !poly )
    {
      auto fresh = new QgsPolygon();
      fresh->setExteriorRing( new QgsLineString() );
      item->setGeometry( fresh );
      return qgsgeometry_cast<QgsLineString *>( fresh->exteriorRing() );
    }
    QgsCurvePolygon *cloned = poly->clone();
    item->setGeometry( cloned );
    return qgsgeometry_cast<QgsLineString *>( cloned->exteriorRing() );
  }
} // namespace


QString KadasPolygonAnnotationController::itemType() const
{
  return QStringLiteral( "polygon" );
}

QString KadasPolygonAnnotationController::itemName() const
{
  return QObject::tr( "Polygon" );
}

QgsAnnotationItem *KadasPolygonAnnotationController::createItem() const
{
  auto poly = new QgsPolygon();
  poly->setExteriorRing( new QgsLineString() );
  auto *item = new QgsAnnotationPolygonItem( poly );
  item->setZIndex( KadasAnnotationZIndex::Polygon );
  return item;
}

QList<KadasMapItem::Node> KadasPolygonAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasMapItem::Node> result;
  const QgsCurvePolygon *poly = asPolygon( item )->geometry();
  if ( !poly )
    return result;
  const QgsCurve *ring = poly->exteriorRing();
  if ( !ring )
    return result;
  const int n = ring->numPoints();
  // Skip the closing duplicate vertex when the ring is closed.
  const int last = ( n > 1 && ring->vertexAt( QgsVertexId( 0, 0, 0 ) ) == ring->vertexAt( QgsVertexId( 0, 0, n - 1 ) ) ) ? n - 1 : n;
  for ( int i = 0; i < last; ++i )
  {
    const QgsPoint p = ring->vertexAt( QgsVertexId( 0, 0, i ) );
    result.append( { toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) } );
  }
  return result;
}

bool KadasPolygonAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Seed with two coincident points (anchor + rubber band) plus the closing
  // vertex required to keep the ring valid.
  auto *ring = new QgsLineString();
  ring->addVertex( QgsPoint( ip.x(), ip.y() ) );
  ring->addVertex( QgsPoint( ip.x(), ip.y() ) );
  ring->addVertex( QgsPoint( ip.x(), ip.y() ) );
  auto *poly = new QgsPolygon();
  poly->setExteriorRing( ring );
  asPolygon( item )->setGeometry( poly );
  return true;
}

bool KadasPolygonAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasPolygonAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  QgsLineString *ring = takeMutableExterior( asPolygon( item ) );
  if ( !ring )
    return;
  const int n = ring->numPoints();
  if ( n < 2 )
    return;
  const QgsPointXY ip = toItemPos( p, ctx );
  // Move the rubber-band vertex (the second-to-last; the last is the closing
  // duplicate of the first vertex).
  ring->moveVertex( QgsVertexId( 0, 0, n - 2 ), QgsPoint( ip.x(), ip.y() ) );
}

void KadasPolygonAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasPolygonAnnotationController::continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext & )
{
  QgsLineString *ring = takeMutableExterior( asPolygon( item ) );
  if ( !ring )
    return false;
  const int n = ring->numPoints();
  // Ring layout while drawing: [v0, ..., v_{n-3}, rubber_band, closing=v0].
  if ( n < 3 )
    return true;
  // If rubber band collapsed onto the previous vertex, drop it and finish.
  if ( ring->pointN( n - 2 ) == ring->pointN( n - 3 ) )
  {
    ring->deleteVertex( QgsVertexId( 0, 0, n - 2 ) );
    return false;
  }
  // Insert a new rubber-band vertex before the closing one.
  ring->insertVertex( QgsVertexId( 0, 0, n - 1 ), ring->pointN( n - 2 ) );
  return true;
}

void KadasPolygonAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasMapItem::AttribDefs KadasPolygonAnnotationController::drawAttribs() const
{
  KadasMapItem::AttribDefs attributes;
  attributes.insert( AttrX, KadasMapItem::NumericAttribute { "x" } );
  attributes.insert( AttrY, KadasMapItem::NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasPolygonAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasMapItem::AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasPolygonAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPolygonAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsCurvePolygon *poly = asPolygon( item )->geometry();
  if ( !poly )
    return KadasMapItem::EditContext();
  const QgsCurve *ring = poly->exteriorRing();
  if ( !ring )
    return KadasMapItem::EditContext();
  const int n = ring->numPoints();
  const int last = ( n > 1 && ring->vertexAt( QgsVertexId( 0, 0, 0 ) ) == ring->vertexAt( QgsVertexId( 0, 0, n - 1 ) ) ) ? n - 1 : n;
  for ( int i = 0; i < last; ++i )
  {
    const QgsPoint p = ring->vertexAt( QgsVertexId( 0, 0, i ) );
    const QgsPointXY mp = toMapPos( QgsPointXY( p.x(), p.y() ), ctx );
    if ( pos.sqrDist( mp ) < pickTolSqr( ctx ) )
    {
      return KadasMapItem::EditContext( QgsVertexId( 0, 0, i ), mp, drawAttribs() );
    }
  }
  if ( toMapRect( asPolygon( item )->boundingBox(), ctx ).contains( pos ) && last > 0 )
  {
    const QgsPoint p0 = ring->vertexAt( QgsVertexId( 0, 0, 0 ) );
    const QgsPointXY refPos = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
    return KadasMapItem::EditContext( QgsVertexId(), refPos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return KadasMapItem::EditContext();
}

void KadasPolygonAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsLineString *ring = takeMutableExterior( asPolygon( item ) );
  if ( !ring )
    return;
  const int n = ring->numPoints();
  if ( editContext.vidx.vertex >= 0 && editContext.vidx.vertex < n )
  {
    const QgsPointXY ip = toItemPos( newPoint, ctx );
    ring->moveVertex( QgsVertexId( 0, 0, editContext.vidx.vertex ), QgsPoint( ip.x(), ip.y() ) );
    // Keep the closing vertex in sync if we moved the first one.
    if ( editContext.vidx.vertex == 0 && n > 1 && ring->pointN( 0 ) != ring->pointN( n - 1 ) )
    {
      ring->moveVertex( QgsVertexId( 0, 0, n - 1 ), QgsPoint( ip.x(), ip.y() ) );
    }
  }
  else if ( n > 0 )
  {
    const QgsPoint p0 = ring->pointN( 0 );
    const QgsPointXY refMap = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
    const double dxMap = newPoint.x() - refMap.x();
    const double dyMap = newPoint.y() - refMap.y();
    for ( int i = 0; i < n; ++i )
    {
      const QgsPoint pi = ring->pointN( i );
      const QgsPointXY mp = toMapPos( QgsPointXY( pi.x(), pi.y() ), ctx );
      const QgsPointXY shifted = toItemPos( QgsPointXY( mp.x() + dxMap, mp.y() + dyMap ), ctx );
      ring->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( shifted.x(), shifted.y() ) );
    }
  }
}

void KadasPolygonAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasMapItem::AttribValues KadasPolygonAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx
) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasPolygonAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasPolygonAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsCurvePolygon *poly = asPolygon( item )->geometry();
  if ( !poly || !poly->exteriorRing() || poly->exteriorRing()->numPoints() == 0 )
    return QgsPointXY();
  const QgsPoint p = poly->exteriorRing()->vertexAt( QgsVertexId( 0, 0, 0 ) );
  return QgsPointXY( p.x(), p.y() );
}

void KadasPolygonAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  QgsLineString *ring = takeMutableExterior( asPolygon( item ) );
  if ( !ring )
    return;
  const int n = ring->numPoints();
  if ( n == 0 )
    return;
  const QgsPoint p0 = ring->pointN( 0 );
  const double dx = pos.x() - p0.x();
  const double dy = pos.y() - p0.y();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ring->pointN( i );
    ring->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

void KadasPolygonAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsLineString *ring = takeMutableExterior( asPolygon( item ) );
  if ( !ring )
    return;
  const int n = ring->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ring->pointN( i );
    ring->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

QString KadasPolygonAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const QgsCurvePolygon *poly = asPolygon( item )->geometry();
  if ( !poly || !poly->exteriorRing() || poly->exteriorRing()->numPoints() == 0 )
    return QString();
  const QgsCurve *ring = poly->exteriorRing();
  QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<Polygon>\n<outerBoundaryIs><LinearRing>\n<coordinates>";
  const int n = ring->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    QgsPointXY p( ring->vertexAt( QgsVertexId( 0, 0, i ) ) );
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
