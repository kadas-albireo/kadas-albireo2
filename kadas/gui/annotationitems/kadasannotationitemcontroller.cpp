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

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"


void KadasAnnotationItemController::populateContextMenu( QgsAnnotationItem *, QMenu *, const KadasMapItem::EditContext &, const KadasMapPos &, const QgsMapSettings & )
{}

void KadasAnnotationItemController::onDoubleClick( QgsAnnotationItem *, const QgsMapSettings & )
{}

bool KadasAnnotationItemController::hitTest( const QgsAnnotationItem *item, const KadasMapPos &pos, const QgsMapSettings &settings ) const
{
  if ( !item )
    return false;
  // Default: hit if the click falls within the bounding box (in map CRS).
  // Subclasses with cheap tighter tests (e.g. distance-to-line) should override.
  Q_UNUSED( settings )
  return item->boundingBox().contains( pos );
}

QPair<KadasMapPos, double> KadasAnnotationItemController::closestPoint( const QgsAnnotationItem *item, const KadasMapPos &pos, const QgsMapSettings &settings ) const
{
  Q_UNUSED( settings )
  if ( !item )
    return { pos, std::numeric_limits<double>::max() };
  // Fallback: report the bounding-box center. Subclasses should override.
  const QgsPointXY center = item->boundingBox().center();
  const KadasMapPos cp( center.x(), center.y() );
  return { cp, std::hypot( cp.x() - pos.x(), cp.y() - pos.y() ) };
}

bool KadasAnnotationItemController::intersects( const QgsAnnotationItem *item, const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const
{
  Q_UNUSED( settings )
  if ( !item )
    return false;
  return contains ? rect.contains( item->boundingBox() ) : rect.intersects( item->boundingBox() );
}
