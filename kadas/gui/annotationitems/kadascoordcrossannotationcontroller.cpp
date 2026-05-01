/***************************************************************************
    kadascoordcrossannotationcontroller.cpp
    ---------------------------------------
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

#include <qgis/qgsannotationmarkeritem.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationcontroller.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"


namespace
{
  inline QgsPointXY roundToKilometre( const QgsPointXY &p )
  {
    return QgsPointXY( std::round( p.x() / 1000.0 ) * 1000.0, std::round( p.y() / 1000.0 ) * 1000.0 );
  }
} // namespace


QString KadasCoordCrossAnnotationController::itemType() const
{
  return KadasCoordCrossAnnotationItem::itemTypeId();
}

QString KadasCoordCrossAnnotationController::itemName() const
{
  return QObject::tr( "Coordinate cross" );
}

QgsAnnotationItem *KadasCoordCrossAnnotationController::createItem() const
{
  auto *item = new KadasCoordCrossAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::CoordCross );
  return item;
}

bool KadasCoordCrossAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY itemPos = roundToKilometre( toItemPos( firstPoint, ctx ) );
  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
    marker->setGeometry( QgsPoint( itemPos.x(), itemPos.y() ) );
  // CoordCross is finalized on the very first click.
  return false;
}

void KadasCoordCrossAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  KadasMarkerAnnotationController::setPosition( item, roundToKilometre( pos ) );
}

void KadasCoordCrossAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  // When dragging the single vertex, snap the resulting position to a km grid.
  KadasMarkerAnnotationController::edit( item, editContext, newPoint, ctx );
  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
  {
    const QgsPointXY snapped = roundToKilometre( QgsPointXY( marker->geometry().x(), marker->geometry().y() ) );
    marker->setGeometry( QgsPoint( snapped.x(), snapped.y() ) );
  }
}
