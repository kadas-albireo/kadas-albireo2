/***************************************************************************
    kadasrectangleannotationcontroller.cpp
    --------------------------------------
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
#include "kadas/gui/annotationitems/kadasrectangleannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"

namespace
{
  inline KadasRectangleAnnotationItem *asRect( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == KadasRectangleAnnotationItem::itemTypeId() );
    return static_cast<KadasRectangleAnnotationItem *>( item );
  }
  inline const KadasRectangleAnnotationItem *asRect( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == KadasRectangleAnnotationItem::itemTypeId() );
    return static_cast<const KadasRectangleAnnotationItem *>( item );
  }
} // namespace


QString KadasRectangleAnnotationController::itemType() const
{
  return KadasRectangleAnnotationItem::itemTypeId();
}

QString KadasRectangleAnnotationController::itemName() const
{
  return QObject::tr( "Rectangle" );
}

QgsAnnotationItem *KadasRectangleAnnotationController::createItem() const
{
  auto *item = new KadasRectangleAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::Rectangle );
  return item;
}

QList<KadasNode> KadasRectangleAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const KadasRectangleAnnotationItem *rect = asRect( item );
  QList<KadasNode> result;
  for ( const QgsPointXY &c : rect->corners() )
    result.append( { toMapPos( c, ctx ) } );
  result.append( { toMapPos( rect->rotationHandle(), ctx ) } );
  return result;
}

bool KadasRectangleAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Center starts at the click; size is zero. Angle stays at 0 during draw.
  asRect( item )->setBox( QgsPointXY( ip.x(), ip.y() ), QSizeF( 0, 0 ), 0.0 );
  return true;
}

bool KadasRectangleAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasRectangleAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  // While drawing, treat the rubber-band point as the opposite corner of an
  // axis-aligned rectangle in item CRS. The center moves to the midpoint.
  KadasRectangleAnnotationItem *rect = asRect( item );
  const QgsPointXY anchor = rect->center(); // first-click position when size is (0,0)
  const QgsPointXY initialCorner( anchor.x(), anchor.y() );
  Q_UNUSED( initialCorner );

  const QgsPointXY cur = toItemPos( p, ctx );

  // Recover the original anchor: if size==(0,0) the anchor IS the center.
  // Once the size grows we still keep the original anchor tracked via the BL
  // corner of the rectangle in its (un-rotated) frame.
  QgsPointXY originAnchor = anchor;
  if ( !qgsDoubleNear( rect->size().width(), 0.0 ) || !qgsDoubleNear( rect->size().height(), 0.0 ) )
  {
    // Anchor = the corner opposite to the rubber-band corner. We assumed
    // angle=0 during draw, so corners()[0] is BL and corners()[2] is TR.
    const auto cs = rect->corners();
    // Pick the corner farthest from the cursor as the anchor.
    double bestSqr = -1.0;
    for ( const QgsPointXY &c : cs )
    {
      const double dx = c.x() - cur.x();
      const double dy = c.y() - cur.y();
      const double s = dx * dx + dy * dy;
      if ( s > bestSqr )
      {
        bestSqr = s;
        originAnchor = c;
      }
    }
  }

  const double w = std::abs( cur.x() - originAnchor.x() );
  const double h = std::abs( cur.y() - originAnchor.y() );
  const QgsPointXY newCenter( 0.5 * ( originAnchor.x() + cur.x() ), 0.5 * ( originAnchor.y() + cur.y() ) );
  rect->setBox( newCenter, QSizeF( w, h ), 0.0 );
}

void KadasRectangleAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasRectangleAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  // Two-click item.
  return false;
}

void KadasRectangleAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasRectangleAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasRectangleAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasRectangleAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasRectangleAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const KadasRectangleAnnotationItem *rect = asRect( item );
  const auto cs = rect->corners();
  for ( int i = 0; i < cs.size(); ++i )
  {
    const QgsPointXY mp = toMapPos( cs[i], ctx );
    if ( pos.sqrDist( mp ) < pickTolSqr( ctx ) )
      return KadasEditContext( QgsVertexId( 0, 0, i ), mp, drawAttribs() );
  }
  const QgsPointXY rotMap = toMapPos( rect->rotationHandle(), ctx );
  if ( pos.sqrDist( rotMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( 0, 0, RotationHandleVertex ), rotMap, drawAttribs(), Qt::CrossCursor );

  if ( toMapRect( rect->boundingBox(), ctx ).contains( pos ) )
  {
    const QgsPointXY centerMap = toMapPos( rect->center(), ctx );
    return KadasEditContext( QgsVertexId(), centerMap, KadasAttribDefs(), Qt::ArrowCursor );
  }
  return KadasEditContext();
}

void KadasRectangleAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  KadasRectangleAnnotationItem *rect = asRect( item );
  const QgsPointXY newIp = toItemPos( newPoint, ctx );

  const int v = editContext.vidx.vertex;

  if ( v == RotationHandleVertex )
  {
    // Compute new angle from center -> newPoint vector, with the local +Y
    // axis (top edge midpoint direction) being the reference. The rotation
    // handle is offset from the top, so its un-rotated direction is +Y.
    const double dx = newIp.x() - rect->center().x();
    const double dy = newIp.y() - rect->center().y();
    // atan2(dx, dy) measures the CCW angle of the vector from the +Y axis.
    const double angleRad = std::atan2( dx, dy );
    rect->setAngle( -angleRad * 180.0 / M_PI );
    return;
  }

  if ( v >= 0 && v < 4 )
  {
    // Resize: the opposite corner stays fixed; the dragged corner moves to
    // the cursor. We work in the rectangle's local (un-rotated) frame so
    // resizing remains correct under rotation.
    const auto cs = rect->corners();
    const QgsPointXY anchor = cs[( v + 2 ) % 4];

    const double a = rect->angle() * M_PI / 180.0;
    const double cosA = std::cos( a );
    const double sinA = std::sin( a );

    auto toLocal = [&]( const QgsPointXY &p ) {
      const double dx = p.x() - rect->center().x();
      const double dy = p.y() - rect->center().y();
      // Inverse rotation: rotate by -angle.
      return QPointF( cosA * dx + sinA * dy, -sinA * dx + cosA * dy );
    };

    const QPointF anchorLocal = toLocal( anchor );
    const QPointF cursorLocal = toLocal( QgsPointXY( newIp.x(), newIp.y() ) );

    const double localCx = 0.5 * ( anchorLocal.x() + cursorLocal.x() );
    const double localCy = 0.5 * ( anchorLocal.y() + cursorLocal.y() );

    // Map the new local center back to world coords.
    const double worldCx = rect->center().x() + cosA * localCx - sinA * localCy;
    const double worldCy = rect->center().y() + sinA * localCx + cosA * localCy;

    const QSizeF newSize( std::abs( cursorLocal.x() - anchorLocal.x() ), std::abs( cursorLocal.y() - anchorLocal.y() ) );
    rect->setBox( QgsPointXY( worldCx, worldCy ), newSize, rect->angle() );
    return;
  }

  // Whole-item move via map-space delta on the center.
  const QgsPointXY oldCenterMap = toMapPos( rect->center(), ctx );
  const double dxMap = newPoint.x() - oldCenterMap.x();
  const double dyMap = newPoint.y() - oldCenterMap.y();
  const QgsPointXY newCenterMap( oldCenterMap.x() + dxMap, oldCenterMap.y() + dyMap );
  const QgsPointXY newCenterIp = toItemPos( newCenterMap, ctx );
  rect->setCenter( QgsPointXY( newCenterIp.x(), newCenterIp.y() ) );
}

void KadasRectangleAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasRectangleAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasRectangleAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasRectangleAnnotationController::position( const QgsAnnotationItem *item ) const
{
  return asRect( item )->center();
}

void KadasRectangleAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  asRect( item )->setCenter( QgsPointXY( pos.x(), pos.y() ) );
}

void KadasRectangleAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  KadasRectangleAnnotationItem *rect = asRect( item );
  const QgsPointXY c = rect->center();
  rect->setCenter( QgsPointXY( c.x() + dx, c.y() + dy ) );
}

QString KadasRectangleAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const KadasRectangleAnnotationItem *rect = asRect( item );
  const auto cs = rect->corners();
  if ( cs.isEmpty() )
    return QString();

  QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<Polygon>\n<outerBoundaryIs><LinearRing>\n<coordinates>";
  for ( int i = 0; i < cs.size(); ++i )
  {
    QgsPointXY p = ct.transform( cs[i] );
    if ( i > 0 )
      outStream << " ";
    outStream << QString::number( p.x(), 'f', 10 ) << "," << QString::number( p.y(), 'f', 10 );
  }
  // Close the ring.
  QgsPointXY p0 = ct.transform( cs.first() );
  outStream << " " << QString::number( p0.x(), 'f', 10 ) << "," << QString::number( p0.y(), 'f', 10 );
  outStream << "</coordinates>\n</LinearRing></outerBoundaryIs>\n</Polygon>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
