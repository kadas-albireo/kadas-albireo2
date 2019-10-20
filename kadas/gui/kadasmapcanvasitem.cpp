/***************************************************************************
    kadasmapcanvasitem.h
    --------------------
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

#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasmapcanvasitem.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

KadasMapCanvasItem::KadasMapCanvasItem( const KadasMapItem *item, QgsMapCanvas *canvas )
  : QgsMapCanvasItem( canvas ), mItem( item )
{
  setZValue( mItem->zIndex() );
  connect( item, &KadasMapItem::changed, this, &KadasMapCanvasItem::updateRect );
  connect( item, &QObject::destroyed, this, &QObject::deleteLater );
  updateRect();
}

void KadasMapCanvasItem::paint( QPainter *painter )
{
  if ( mItem )
  {
    if ( mItem->associatedLayer() && !mMapCanvas->layers().contains( mItem->associatedLayer() ) )
    {
      return;
    }
    QgsRenderContext rc = QgsRenderContext::fromQPainter( painter );
    rc.setMapExtent( mMapCanvas->mapSettings().visibleExtent() );
    rc.setExtent( mMapCanvas->mapSettings().extent() );
    rc.setMapToPixel( mMapCanvas->mapSettings().mapToPixel() );
    rc.setTransformContext( mMapCanvas->mapSettings().transformContext() );
    rc.setFlag( QgsRenderContext::Antialiasing, true );
    rc.setCoordinateTransform( QgsCoordinateTransform( mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext() ) );

    rc.painter()->save();
    rc.painter()->translate( -pos() );
    rc.painter()->save();
    mItem->render( rc );
    rc.painter()->restore();

    if ( mItem->selected() )
    {
      for ( const KadasMapItem::Node &node : mItem->nodes( mMapCanvas->mapSettings() ) )
      {
        QPointF screenPoint = mMapCanvas->mapSettings().mapToPixel().transform( node.pos ).toQPointF();
        screenPoint.setX( qRound( screenPoint.x() ) );
        screenPoint.setY( qRound( screenPoint.y() ) );
        rc.painter()->save();
        node.render( rc.painter(), screenPoint, sHandleSize );
        rc.painter()->restore();
      }
    }
    rc.painter()->restore();
  }
}

void KadasMapCanvasItem::updateRect()
{
  QgsCoordinateTransform t( mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext() );
  QgsRectangle bbox = t.transformBoundingBox( mItem->boundingBox() );
  double mup = mMapCanvas->mapUnitsPerPixel();
  KadasMapItem::Margin margin = mItem->margin();
  bbox.setXMinimum( bbox.xMinimum() - ( margin.left + sHandleSize ) * mup );
  bbox.setXMaximum( bbox.xMaximum() + ( margin.right + sHandleSize ) * mup );
  bbox.setYMinimum( bbox.yMinimum() - ( margin.bottom + sHandleSize ) * mup );
  bbox.setYMaximum( bbox.yMaximum() + ( margin.top + sHandleSize ) * mup );
  setRect( bbox );
}
