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
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsexception.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsproject.h>

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

  // Snaps an item-CRS position to the round-km grid of the labelling CRS (degrees would round to 0/0).
  QgsPointXY snapToKmGrid( const QgsPointXY &itemPos, const QgsCoordinateReferenceSystem &layerCrs )
  {
    const QgsCoordinateReferenceSystem crossCrs = KadasCoordCrossAnnotationItem::labelCrs( layerCrs );
    if ( !layerCrs.isValid() || crossCrs == layerCrs )
      return roundToKilometre( itemPos );
    const QgsCoordinateTransform ct( layerCrs, crossCrs, QgsProject::instance() );
    try
    {
      return ct.transform( roundToKilometre( ct.transform( itemPos ) ), Qgis::TransformDirection::Reverse );
    }
    catch ( QgsCsException & )
    {
      return itemPos;
    }
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
  const QgsPointXY itemPos = snapToKmGrid( toItemPos( firstPoint, ctx ), ctx.itemCrs() );
  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
    marker->setGeometry( QgsPoint( itemPos.x(), itemPos.y() ) );
  return false;
}

void KadasCoordCrossAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  KadasMarkerAnnotationController::setPosition( item, pos );
}

void KadasCoordCrossAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  KadasMarkerAnnotationController::edit( item, editContext, newPoint, ctx );
  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
  {
    const QgsPointXY snapped = snapToKmGrid( QgsPointXY( marker->geometry().x(), marker->geometry().y() ), ctx.itemCrs() );
    marker->setGeometry( QgsPoint( snapped.x(), snapped.y() ) );
  }
}

QList<QgsAnnotationItem *> KadasCoordCrossAnnotationController::generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const auto *master = static_cast<const KadasCoordCrossAnnotationItem *>( item );
  const QgsPointXY pt = master->geometry();

  QList<QgsAnnotationItem *> shadows;

  auto *cross = new QgsAnnotationMarkerItem( QgsPoint( pt.x(), pt.y() ) );
  auto *layer = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Cross, 20.0 );
  layer->setStrokeColor( QColor( 0, 0, 0 ) );
  layer->setStrokeWidth( 0.4 );
  auto *symbol = new QgsMarkerSymbol( QgsSymbolLayerList() << layer );
  cross->setSymbol( symbol );
  cross->setZIndex( master->zIndex() );
  shadows.append( cross );

  QgsPointXY labelPt = pt;
  const QgsCoordinateReferenceSystem layerCrs = ctx.itemCrs();
  const QgsCoordinateReferenceSystem crossCrs = KadasCoordCrossAnnotationItem::labelCrs( layerCrs );
  if ( layerCrs.isValid() && crossCrs != layerCrs )
  {
    try
    {
      labelPt = QgsCoordinateTransform( layerCrs, crossCrs, QgsProject::instance() ).transform( pt );
    }
    catch ( QgsCsException & )
    {}
  }
  const QString label = QStringLiteral( "%1 / %2" ).arg( QString::number( labelPt.x() / 1000.0, 'f', 0 ) ).arg( QString::number( labelPt.y() / 1000.0, 'f', 0 ) );
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
