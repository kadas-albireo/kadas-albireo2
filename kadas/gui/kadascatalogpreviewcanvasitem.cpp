/***************************************************************************
    kadascatalogpreviewcanvasitem.cpp
    ---------------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QPainter>

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadascatalogpreviewcanvasitem.h"

KadasCatalogPreviewCanvasItem::KadasCatalogPreviewCanvasItem( QgsMapCanvas *mapCanvas )
  : QgsMapCanvasItem( mapCanvas )
{
  // Above the map, below the annotation and tool items.
  setZValue( 50 );
}

void KadasCatalogPreviewCanvasItem::setImage( const QImage &image, const QgsRectangle &extent )
{
  mImage = image;
  // Anchoring the item to the extent the image was rendered for lets the base
  // class keep it in place while the canvas pans or zooms.
  setRect( extent );
  updateCanvas();
}

void KadasCatalogPreviewCanvasItem::clear()
{
  if ( mImage.isNull() )
  {
    return;
  }
  mImage = QImage();
  updateCanvas();
}

void KadasCatalogPreviewCanvasItem::paint( QPainter *painter )
{
  if ( mImage.isNull() )
  {
    return;
  }
  // Passing the source rect in image pixels leaves the device pixel ratio to
  // the painter, so a high dpi render is not drawn at double size.
  painter->setRenderHint( QPainter::SmoothPixmapTransform, true );
  painter->drawImage( boundingRect(), mImage, QRectF( mImage.rect() ) );
}
