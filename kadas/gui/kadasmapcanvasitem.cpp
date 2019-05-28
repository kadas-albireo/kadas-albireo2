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

#include <kadas/core/mapitems/kadasmapitem.h>
#include <kadas/gui/kadasmapcanvasitem.h>

KadasMapCanvasItem::KadasMapCanvasItem(KadasMapItem* item, QgsMapCanvas* canvas)
  : QgsMapCanvasItem(canvas), mItem(item)
{
  connect(item, &KadasMapItem::changed, this, &KadasMapCanvasItem::updateRect);
  connect(item, &QObject::destroyed, this, &QObject::deleteLater);
}

void KadasMapCanvasItem::paint(QPainter *painter)
{
  if ( mItem ) {
    QgsRenderContext rc = QgsRenderContext::fromQPainter( painter );
    rc.setMapToPixel( mMapCanvas->mapSettings().mapToPixel() );
    rc.setTransformContext( mMapCanvas->mapSettings().transformContext() );
    rc.setFlag( QgsRenderContext::Antialiasing, true );
    rc.setCoordinateTransform(QgsCoordinateTransform(mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext()));

    rc.painter()->save();
    rc.painter()->translate( -pos() );
    mItem->render( rc );

    if(/*isSelected()*/true) {
      double handleSize = 8;
      rc.painter()->setPen( QPen(Qt::red, 2) );
      rc.painter()->setBrush( Qt::white );
      QgsCoordinateTransform crst(mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext());

      for(const QgsPointXY& point : mItem->nodes()) {
        QgsPointXY screenPoint = mMapCanvas->mapSettings().mapToPixel().transform(crst.transform(point));
        rc.painter()->drawRect( QRectF( screenPoint.x() - 0.5 * handleSize, screenPoint.y() - 0.5 * handleSize, handleSize, handleSize ) );
      }
    }
    rc.painter()->restore();
  }
}

void KadasMapCanvasItem::updateRect()
{
  QgsCoordinateTransform t(mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext());
  QgsRectangle bbox = t.transform(mItem->boundingBox());
  double scale = mMapCanvas->mapUnitsPerPixel();
  setRect(bbox.buffered(mItem->margin() * scale));
}
