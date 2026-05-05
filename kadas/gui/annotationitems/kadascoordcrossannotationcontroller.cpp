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
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>

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

void KadasCoordCrossAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  // When dragging the single vertex, snap the resulting position to a km grid.
  KadasMarkerAnnotationController::edit( item, editContext, newPoint, ctx );
  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
  {
    const QgsPointXY snapped = roundToKilometre( QgsPointXY( marker->geometry().x(), marker->geometry().y() ) );
    marker->setGeometry( QgsPoint( snapped.x(), snapped.y() ) );
  }
}

QList<QgsAnnotationItem *> KadasCoordCrossAnnotationController::generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  Q_UNUSED( ctx );
  // The Kadas coord-cross is screen-space (cross arms in pixels, label rendered
  // at canvas-pixel offsets relative to the cross). For a project-time shadow we
  // can only emit a map-space marker + a single coordinate label at the same
  // point. The four-quadrant labels and screen-pixel arm length cannot be
  // reproduced without canvas state, so the QGIS shadow is a single \"+\" marker
  // with one km-coordinate text label colocated with it.\n
  const auto *master = static_cast<const KadasCoordCrossAnnotationItem *>( item );
  const QgsPointXY pt = master->geometry();

  QList<QgsAnnotationItem *> shadows;

  // Cross-shaped marker shadow. The Kadas master draws an 80px-wide cross
  // (~21mm at 96 DPI); match that order of magnitude for the QGIS shadow
  // so the cross has comparable visual weight when rendered with the
  // mm-based simple-marker layer.
  auto *cross = new QgsAnnotationMarkerItem( QgsPoint( pt.x(), pt.y() ) );
  auto *layer = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Cross, 20.0 );
  layer->setStrokeColor( QColor( 0, 0, 0 ) );
  layer->setStrokeWidth( 0.4 );
  auto *symbol = new QgsMarkerSymbol( QgsSymbolLayerList() << layer );
  cross->setSymbol( symbol );
  cross->setZIndex( master->zIndex() );
  shadows.append( cross );

  // Coordinate label shadow.
  const QString label = QStringLiteral( "%1 / %2" ).arg( QString::number( pt.x() / 1000.0, 'f', 0 ) ).arg( QString::number( pt.y() / 1000.0, 'f', 0 ) );
  auto *text = new QgsAnnotationPointTextItem( label, pt );
  text->setZIndex( master->zIndex() );
  shadows.append( text );

  return shadows;
}

QStringList KadasCoordCrossAnnotationController::shadowIds( const QgsAnnotationItem *item ) const
{
  return static_cast<const KadasCoordCrossAnnotationItem *>( item )->shadowIds();
}

void KadasCoordCrossAnnotationController::setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const
{
  static_cast<KadasCoordCrossAnnotationItem *>( item )->setShadowIds( ids );
}
