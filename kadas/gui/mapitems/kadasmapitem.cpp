/***************************************************************************
    kadasmapitem.cpp
    ----------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgis/qgsmaplayer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadasmapitem.h>


KadasMapItem::KadasMapItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : QObject( parent ), mCrs( crs )
{
}

KadasMapItem::~KadasMapItem()
{
  emit aboutToBeDestroyed();
  if ( mAssociatedLayer )
  {
    setParent( nullptr );
    QgsProject::instance()->removeMapLayer( mAssociatedLayer->id() );
  }
}

KadasMapItem *KadasMapItem::clone() const
{
  KadasMapItem *item = _clone();
  item->setState( constState()->clone() );
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    prop.write( item, prop.read( this ) );
  }
  return item;
}

void KadasMapItem::associateToLayer( QgsMapLayer *layer )
{
  mAssociatedLayer = layer;
  setParent( layer );
}

void KadasMapItem::setSelected( bool selected )
{
  mSelected = selected;
  update();
}

void KadasMapItem::setZIndex( int zIndex )
{
  mZIndex = zIndex;
  update();
}

void KadasMapItem::setState( const State *state )
{
  mState->assign( state );
  update();
}

void KadasMapItem::clear()
{
  delete mState;
  mState = createEmptyState();
  update();
}

void KadasMapItem::update()
{
  emit changed();
}

KadasMapPos KadasMapItem::toMapPos( const KadasItemPos &itemPos, const QgsMapSettings &settings ) const
{
  QgsPointXY pos = QgsCoordinateTransform( mCrs, settings.destinationCrs(), QgsProject::instance()->transformContext() ).transform( itemPos );
  return KadasMapPos( pos.x(), pos.y() );
}

KadasItemPos KadasMapItem::toItemPos( const KadasMapPos &mapPos, const QgsMapSettings &settings ) const
{
  QgsPointXY pos = QgsCoordinateTransform( settings.destinationCrs(), mCrs, QgsProject::instance()->transformContext() ).transform( mapPos );
  return KadasItemPos( pos.x(), pos.y() );
}

KadasMapRect KadasMapItem::toMapRect( const KadasItemRect &itemRect, const QgsMapSettings &settings ) const
{
  QgsRectangle rect = QgsCoordinateTransform( mCrs, settings.destinationCrs(), QgsProject::instance()->transformContext() ).transform( itemRect );
  return KadasMapRect( rect.xMinimum(), rect.yMinimum(), rect.xMaximum(), rect.yMaximum() );
}

KadasItemRect KadasMapItem::toItemRect( const KadasMapRect &itemRect, const QgsMapSettings &settings ) const
{
  QgsRectangle rect = QgsCoordinateTransform( settings.destinationCrs(), mCrs, QgsProject::instance()->transformContext() ).transform( itemRect );
  return KadasItemRect( rect.xMinimum(), rect.yMinimum(), rect.xMaximum(), rect.yMaximum() );
}

double KadasMapItem::pickTolSqr( const QgsMapSettings &settings ) const
{
  return 25 * settings.mapUnitsPerPixel() * settings.mapUnitsPerPixel();
}

double KadasMapItem::pickTol( const QgsMapSettings &settings ) const
{
  return 5 * settings.mapUnitsPerPixel();
}

void KadasMapItem::defaultNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::red, 2 ) );
  painter->setBrush( Qt::white );
  painter->drawRect( QRectF( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize ) );
}

void KadasMapItem::anchorNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::red );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}
