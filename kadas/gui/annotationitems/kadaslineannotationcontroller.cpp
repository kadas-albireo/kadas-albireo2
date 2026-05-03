/***************************************************************************
    kadaslineannotationcontroller.cpp
    ---------------------------------
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

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"

namespace
{
  inline QgsAnnotationLineItem *asLine( QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<QgsAnnotationLineItem *>( item ) );
    return static_cast<QgsAnnotationLineItem *>( item );
  }
  inline const QgsAnnotationLineItem *asLine( const QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<const QgsAnnotationLineItem *>( item ) );
    return static_cast<const QgsAnnotationLineItem *>( item );
  }

  //! Returns a mutable QgsLineString backing the item, or nullptr if the curve is not a LineString.
  //! Mutates the item via setGeometry() with a clone of the underlying line for in-place edits.
  QgsLineString *takeMutableLine( QgsAnnotationLineItem *item )
  {
    const QgsCurve *curve = item->geometry();
    if ( !curve )
    {
      auto fresh = new QgsLineString();
      item->setGeometry( fresh );
      return fresh;
    }
    QgsLineString *clone = qgsgeometry_cast<QgsLineString *>( curve->clone() );
    Q_ASSERT( clone );
    item->setGeometry( clone );
    return clone;
  }
} // namespace


QString KadasLineAnnotationController::itemType() const
{
  return QStringLiteral( "linestring" );
}

QString KadasLineAnnotationController::itemName() const
{
  return QObject::tr( "Line" );
}

QgsAnnotationItem *KadasLineAnnotationController::createItem() const
{
  auto *item = new QgsAnnotationLineItem( new QgsLineString() );
  item->setZIndex( KadasAnnotationZIndex::Line );
  return item;
}

QList<KadasNode> KadasLineAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasNode> result;
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve )
    return result;
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    result.append( { toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) } );
  }
  return result;
}

bool KadasLineAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Mirror legacy behavior: seed with two coincident points so that
  // setCurrentPoint() can update the trailing one as a "rubber band".
  auto *ls = new QgsLineString();
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  asLine( item )->setGeometry( ls );
  return true;
}

bool KadasLineAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasLineAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  if ( n == 0 )
    return;
  const QgsPointXY ip = toItemPos( p, ctx );
  ls->moveVertex( QgsVertexId( 0, 0, n - 1 ), QgsPoint( ip.x(), ip.y() ) );
}

void KadasLineAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasLineAnnotationController::continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext & )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  // If current (trailing) point coincides with the previous one, drop it and finish.
  if ( n > 2 && ls->pointN( n - 1 ) == ls->pointN( n - 2 ) )
  {
    ls->deleteVertex( QgsVertexId( 0, 0, n - 1 ) );
    return false;
  }
  if ( n == 0 )
    return true;
  // Append a duplicate of the trailing point as the new rubber-band point.
  ls->addVertex( ls->pointN( n - 1 ) );
  return true;
}

void KadasLineAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasLineAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasLineAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasLineAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasLineAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve )
    return KadasEditContext();
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    const QgsPointXY mp = toMapPos( QgsPointXY( p.x(), p.y() ), ctx );
    if ( pos.sqrDist( mp ) < pickTolSqr( ctx ) )
    {
      return KadasEditContext( QgsVertexId( 0, 0, i ), mp, drawAttribs() );
    }
  }
  // Fall back: bbox hit → whole-geometry move handle anchored at the first vertex.
  if ( toMapRect( asLine( item )->boundingBox(), ctx ).contains( pos ) && n > 0 )
  {
    const QgsPoint p0 = curve->vertexAt( QgsVertexId( 0, 0, 0 ) );
    const QgsPointXY refPos = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
    return KadasEditContext( QgsVertexId(), refPos, KadasAttribDefs(), Qt::ArrowCursor );
  }
  return KadasEditContext();
}

void KadasLineAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  if ( editContext.vidx.vertex >= 0 && editContext.vidx.vertex < n )
  {
    const QgsPointXY ip = toItemPos( newPoint, ctx );
    ls->moveVertex( QgsVertexId( 0, 0, editContext.vidx.vertex ), QgsPoint( ip.x(), ip.y() ) );
  }
  else if ( n > 0 )
  {
    // Whole-geometry move: shift every vertex by (newPoint - refMapPos).
    const QgsPoint p0 = ls->pointN( 0 );
    const QgsPointXY refMap = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
    const double dxMap = newPoint.x() - refMap.x();
    const double dyMap = newPoint.y() - refMap.y();
    for ( int i = 0; i < n; ++i )
    {
      const QgsPoint pi = ls->pointN( i );
      const QgsPointXY mp = toMapPos( QgsPointXY( pi.x(), pi.y() ), ctx );
      const QgsPointXY shifted = toItemPos( QgsPointXY( mp.x() + dxMap, mp.y() + dyMap ), ctx );
      ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( shifted.x(), shifted.y() ) );
    }
  }
}

void KadasLineAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasLineAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasLineAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasLineAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve || curve->numPoints() == 0 )
    return QgsPointXY();
  const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, 0 ) );
  return QgsPointXY( p.x(), p.y() );
}

void KadasLineAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  if ( n == 0 )
    return;
  const QgsPoint p0 = ls->pointN( 0 );
  const double dx = pos.x() - p0.x();
  const double dy = pos.y() - p0.y();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ls->pointN( i );
    ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

void KadasLineAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ls->pointN( i );
    ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

QString KadasLineAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve || curve->numPoints() == 0 )
    return QString();

  QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<LineString>\n<coordinates>";
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    QgsPointXY p( curve->vertexAt( QgsVertexId( 0, 0, i ) ) );
    p = ct.transform( p );
    if ( i > 0 )
      outStream << " ";
    outStream << QString::number( p.x(), 'f', 10 ) << "," << QString::number( p.y(), 'f', 10 );
  }
  outStream << "</coordinates>\n</LineString>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
