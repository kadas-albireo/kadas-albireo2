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

KadasMapCanvasItem::KadasMapCanvasItem(const KadasMapItem *item, QgsMapCanvas* canvas)
  : QgsMapCanvasItem(canvas), mItem(item)
{
  setZValue(mItem->zIndex());
  connect(item, &KadasMapItem::changed, this, &KadasMapCanvasItem::updateRect);
  connect(item, &QObject::destroyed, this, &QObject::deleteLater);
  updateRect();
}

void KadasMapCanvasItem::paint(QPainter *painter)
{
  if ( mItem ) {
    if(mItem->associatedLayer() && !mMapCanvas->layers().contains(mItem->associatedLayer())) {
      return;
    }
    QgsRenderContext rc = QgsRenderContext::fromQPainter( painter );
    rc.setMapToPixel( mMapCanvas->mapSettings().mapToPixel() );
    rc.setTransformContext( mMapCanvas->mapSettings().transformContext() );
    rc.setFlag( QgsRenderContext::Antialiasing, true );
    rc.setCoordinateTransform(QgsCoordinateTransform(mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext()));

    rc.painter()->save();
    rc.painter()->translate( -pos() );
    mItem->render( rc );

    if(mItem->selected()) {
      QgsCoordinateTransform crst(mItem->crs(), mMapCanvas->mapSettings().destinationCrs(), mMapCanvas->mapSettings().transformContext());
      for(const KadasMapItem::Node& node : mItem->nodes(mMapCanvas->mapSettings())) {
        QgsPointXY screenPoint = mMapCanvas->mapSettings().mapToPixel().transform(crst.transform(node.pos));
        screenPoint.setX(qRound(screenPoint.x()));
        screenPoint.setY(qRound(screenPoint.y()));
        node.render(rc.painter(), screenPoint, sHandleSize);
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
  QRect margin = mItem->margin();
  bbox.setXMinimum(bbox.xMinimum() - margin.left() * scale - 0.5 * sHandleSize);
  bbox.setXMaximum(bbox.xMaximum() + margin.right() * scale + 0.5 * sHandleSize);
  bbox.setYMinimum(bbox.yMinimum() - margin.top() * scale  - 0.5 * sHandleSize);
  bbox.setYMaximum(bbox.yMaximum() + margin.bottom() * scale + 0.5 * sHandleSize);
  setRect(bbox);
}
