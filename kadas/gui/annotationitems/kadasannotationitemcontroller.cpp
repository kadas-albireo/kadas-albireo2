/***************************************************************************
    kadasannotationitemcontroller.cpp
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

#include <cmath>
#include <limits>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"


void KadasAnnotationItemController::populateContextMenu( QgsAnnotationItem *, QMenu *, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext & )
{}

void KadasAnnotationItemController::onDoubleClick( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{}

bool KadasAnnotationItemController::hitTest( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  if ( !item )
    return false;
  // Default: hit if the click falls within the item's bounding box (in map space).
  // Subclasses with cheap tighter tests should override.
  return toMapRect( item->boundingBox(), ctx ).contains( pos );
}

QPair<QgsPointXY, double> KadasAnnotationItemController::closestPoint( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  if ( !item )
    return { pos, std::numeric_limits<double>::max() };
  // Fallback: report the bounding-box center (in map space). Subclasses should override.
  const QgsPointXY centerItem = item->boundingBox().center();
  const QgsPointXY centerMap = toMapPos( centerItem, ctx );
  const QgsPointXY cp( centerMap.x(), centerMap.y() );
  return { cp, std::hypot( cp.x() - pos.x(), cp.y() - pos.y() ) };
}

bool KadasAnnotationItemController::intersects( const QgsAnnotationItem *item, const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx, bool contains ) const
{
  if ( !item )
    return false;
  const QgsRectangle itemBounds = item->boundingBox();
  const QgsRectangle itemRect = toItemRect( mapRect, ctx );
  return contains ? itemRect.contains( itemBounds ) : itemRect.intersects( itemBounds );
}

// ----- Transform helpers ---------------------------------------------------

QgsPointXY KadasAnnotationItemController::toMapPos( const QgsPointXY &itemPos, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() ).transform( itemPos );
}

QgsPointXY KadasAnnotationItemController::toItemPos( const QgsPointXY &mapPos, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY p = QgsCoordinateTransform( ctx.mapSettings().destinationCrs(), ctx.itemCrs(), ctx.mapSettings().transformContext() ).transform( mapPos );
  return QgsPointXY( p.x(), p.y() );
}

QgsRectangle KadasAnnotationItemController::toItemRect( const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.mapSettings().destinationCrs(), ctx.itemCrs(), ctx.mapSettings().transformContext() ).transform( mapRect );
}

QgsRectangle KadasAnnotationItemController::toMapRect( const QgsRectangle &itemRect, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() ).transform( itemRect );
}

double KadasAnnotationItemController::pickTolSqr( const KadasAnnotationItemContext &ctx )
{
  const double mupp = ctx.mapSettings().mapUnitsPerPixel();
  return 25 * mupp * mupp;
}
