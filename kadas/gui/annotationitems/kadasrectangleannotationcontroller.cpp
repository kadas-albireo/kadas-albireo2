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
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgsdistancearea.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
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
  mDrawAnchor = firstPoint;
  mDrawAnchorValid = true;
  asRect( item )->setBox( toItemPos( firstPoint, ctx ), QSizeF( 0, 0 ), 0.0, ctx.mapSettings().destinationCrs(), ctx.itemCrs() );
  return true;
}

bool KadasRectangleAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasRectangleAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  KadasRectangleAnnotationItem *rect = asRect( item );
  const QgsPointXY anchorMap = mDrawAnchorValid ? mDrawAnchor : toMapPos( rect->center(), ctx );
  const double w = std::abs( p.x() - anchorMap.x() );
  const double h = std::abs( p.y() - anchorMap.y() );
  const QgsPointXY centerMap( 0.5 * ( anchorMap.x() + p.x() ), 0.5 * ( anchorMap.y() + p.y() ) );
  rect->setBox( toItemPos( centerMap, ctx ), QSizeF( w, h ), 0.0, ctx.mapSettings().destinationCrs(), ctx.itemCrs() );
}

void KadasRectangleAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasRectangleAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasRectangleAnnotationController::endPart( QgsAnnotationItem * )
{
  mDrawAnchorValid = false;
}

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

  const auto csMap = [&]() {
    QVector<QgsPointXY> v;
    v.reserve( cs.size() );
    for ( const QgsPointXY &c : cs )
      v.append( toMapPos( c, ctx ) );
    return v;
  }();
  auto pointInQuad = [&]( const QgsPointXY &p ) {
    bool inside = false;
    const int n = csMap.size();
    for ( int i = 0, j = n - 1; i < n; j = i++ )
    {
      const double yi = csMap[i].y(), yj = csMap[j].y();
      if ( ( yi > p.y() ) != ( yj > p.y() ) )
      {
        const double xCross = csMap[j].x() + ( csMap[i].x() - csMap[j].x() ) * ( p.y() - yj ) / ( yi - yj );
        if ( p.x() < xCross )
          inside = !inside;
      }
    }
    return inside;
  };
  if ( pointInQuad( pos ) )
  {
    const QgsPointXY centerMap = toMapPos( rect->center(), ctx );
    return KadasEditContext( QgsVertexId(), centerMap, KadasAttribDefs(), Qt::ArrowCursor );
  }
  return KadasEditContext();
}

void KadasRectangleAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  KadasRectangleAnnotationItem *rect = asRect( item );

  const int v = editContext.vidx.vertex;

  const QgsCoordinateReferenceSystem mapCrs = ctx.mapSettings().destinationCrs();
  const QgsCoordinateReferenceSystem layerCrs = ctx.itemCrs();
  const QgsPointXY centerMap = toMapPos( rect->center(), ctx );

  if ( v == RotationHandleVertex )
  {
    // Handle offset along +Y, so atan2(dx, dy) gives the CCW rotation from +Y.
    const double dx = newPoint.x() - centerMap.x();
    const double dy = newPoint.y() - centerMap.y();
    const double angleRad = std::atan2( dx, dy );
    rect->setBox( rect->center(), rect->size(), -angleRad * 180.0 / M_PI, mapCrs, layerCrs );
    return;
  }

  if ( v >= 0 && v < 4 )
  {
    const auto cs = rect->corners();
    const QgsPointXY anchorMap = toMapPos( cs[( v + 2 ) % 4], ctx );

    const double a = rect->angle() * M_PI / 180.0;
    const double cosA = std::cos( a );
    const double sinA = std::sin( a );

    auto toLocal = [&]( const QgsPointXY &p ) {
      const double dx = p.x() - centerMap.x();
      const double dy = p.y() - centerMap.y();
      return QPointF( cosA * dx + sinA * dy, -sinA * dx + cosA * dy );
    };

    const QPointF anchorLocal = toLocal( anchorMap );
    const QPointF cursorLocal = toLocal( newPoint );

    const double localCx = 0.5 * ( anchorLocal.x() + cursorLocal.x() );
    const double localCy = 0.5 * ( anchorLocal.y() + cursorLocal.y() );

    const double newMapCx = centerMap.x() + cosA * localCx - sinA * localCy;
    const double newMapCy = centerMap.y() + sinA * localCx + cosA * localCy;
    const QgsPointXY newCenterLayer = toItemPos( QgsPointXY( newMapCx, newMapCy ), ctx );

    const QSizeF newSize( std::abs( cursorLocal.x() - anchorLocal.x() ), std::abs( cursorLocal.y() - anchorLocal.y() ) );
    rect->setBox( newCenterLayer, newSize, rect->angle(), mapCrs, layerCrs );
    return;
  }

  const double dxMap = newPoint.x() - centerMap.x();
  const double dyMap = newPoint.y() - centerMap.y();
  const QgsPointXY newCenterMap( centerMap.x() + dxMap, centerMap.y() + dyMap );
  rect->setBox( toItemPos( newCenterMap, ctx ), rect->size(), rect->angle(), mapCrs, layerCrs );
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
  QgsPointXY p0 = ct.transform( cs.first() );
  outStream << " " << QString::number( p0.x(), 'f', 10 ) << "," << QString::number( p0.y(), 'f', 10 );
  outStream << "</coordinates>\n</LinearRing></outerBoundaryIs>\n</Polygon>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}

KadasAnnotationStyleEditor *KadasRectangleAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasPolygonStyleEditor( parent );
}

QList<KadasAnnotationMeasurementLabel> KadasRectangleAnnotationController::measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasAnnotationMeasurementLabel> labels;
  const KadasRectangleAnnotationItem *rect = asRect( item );
  if ( rect->size().isEmpty() )
    return labels;

  const QVector<QgsPointXY> corners = rect->corners();
  if ( corners.size() != 4 )
    return labels;

  QgsDistanceArea da;
  da.setSourceCrs( ctx.itemCrs(), ctx.mapSettings().transformContext() );
  da.setEllipsoid( QgsProject::instance()->ellipsoid() );

  for ( int i = 0; i < 4; ++i )
  {
    const QgsPointXY &a = corners[i];
    const QgsPointXY &b = corners[( i + 1 ) % 4];
    const double seg = da.measureLine( a, b );
    const QgsPointXY midItem( 0.5 * ( a.x() + b.x() ), 0.5 * ( a.y() + b.y() ) );
    labels.append( { toMapPos( midItem, ctx ), formatLengthMeters( seg ), true } );
  }

  double areaM2 = 0.0;
  QgsPointXY centroidItem = rect->center();
  if ( const QgsAbstractGeometry *g = rect->geometry() )
  {
    const QgsGeometry geom( g->clone() );
    areaM2 = da.measureArea( geom );
    centroidItem = geom.centroid().asPoint();
  }
  labels.append( { toMapPos( centroidItem, ctx ), formatAreaSquareMeters( areaM2 ), true } );
  return labels;
}

QList<QgsAnnotationItem *> KadasRectangleAnnotationController::generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  Q_UNUSED( ctx );
  const auto *master = static_cast<const KadasRectangleAnnotationItem *>( item );
  if ( !master->geometry() || master->size().isEmpty() )
    return {};
  auto *shadow = new QgsAnnotationPolygonItem( master->geometry()->clone() );
  if ( master->symbol() )
    shadow->setSymbol( master->symbol()->clone() );
  shadow->setZIndex( master->zIndex() );
  return { shadow };
}

QStringList KadasRectangleAnnotationController::shadowIds( const QgsAnnotationItem *item ) const
{
  return static_cast<const KadasRectangleAnnotationItem *>( item )->shadowIds();
}

void KadasRectangleAnnotationController::setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const
{
  static_cast<KadasRectangleAnnotationItem *>( item )->setShadowIds( ids );
}
