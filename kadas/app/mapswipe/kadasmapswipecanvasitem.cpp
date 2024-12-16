/***************************************************************************
    kadasmapswipecanvasitem.cpp
    -----------------
    copyright            : (C) 2024 Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <qgsmapcanvas.h>
#include <qgsmaprendererparalleljob.h>

#include "kadasmapswipecanvasitem.h"


KadasMapSwipeCanvasItem::KadasMapSwipeCanvasItem( QgsMapCanvas *mapCanvas )
  : QgsMapCanvasItem( mapCanvas )
{
  QObject::connect( mapCanvas, &QgsMapCanvas::extentsChanged, [=]() { refreshMap(); } );
  QObject::connect( mapCanvas, &QgsMapCanvas::layersChanged, [=]() { refreshMap(); } );
}

void KadasMapSwipeCanvasItem::enable()
{
  mIsVertical = true;
  mPixelLength = int( boundingRect().width() / 2 );
  updateCanvas();
}

void KadasMapSwipeCanvasItem::disable( bool clearLayers )
{
  if ( clearLayers )
    mRemovedLayers.clear();

  mPixelLength = -1;

  refreshMap();
  updateCanvas();
}

void KadasMapSwipeCanvasItem::setLayers( const QSet<QgsMapLayer *> &layers )
{
  mRemovedLayers = layers;
  refreshMap();
}

void KadasMapSwipeCanvasItem::setPixelPosition( int x, int y )
{
  if ( mIsVertical )
  {
    mPixelLength = x;
  }
  else
  {
    mPixelLength = boundingRect().height() - y;
  }
  refreshMap();
}

int KadasMapSwipeCanvasItem::pixelLength() const
{
  if ( mIsVertical )
    return mPixelLength;
  else
    return boundingRect().height() - mPixelLength;
}

void KadasMapSwipeCanvasItem::setVertical( bool vertical )
{
  mIsVertical = vertical;
  refreshMap();
}

void KadasMapSwipeCanvasItem::refreshMap()
{
  QgsMapSettings settings( mMapCanvas->mapSettings() );
  QList<QgsMapLayer *> mapLayers = settings.layers();
  for ( QgsMapLayer *layer : std::as_const( mRemovedLayers ) )
    mapLayers.removeOne( layer );

  settings.setLayers( mapLayers );
  settings.setBackgroundColor( QColor( Qt::GlobalColor::white ) );

  setRect( mMapCanvas->extent() );
  QgsMapRendererParallelJob job( settings );
  job.start();
  QObject::connect( &job, &QgsMapRendererParallelJob::finished, [this, &job]() {
    mRenderedMapImage = job.renderedImage();
  } );
  job.waitForFinished();
}


void KadasMapSwipeCanvasItem::paint( QPainter *painter )
{
  if ( mPixelLength < 0 || mRemovedLayers.isEmpty() )
    return;

  QLine line;
  QRect rect;
  if ( mIsVertical )
  {
    int h = boundingRect().height() - 2;
    int w = mPixelLength;
    line = QLine( w - 1, 0, w - 1, h - 1 );
    rect = QRect( w, 0, mRenderedMapImage.width() - w, mRenderedMapImage.height() );
  }
  else
  {
    int h = boundingRect().height() - mPixelLength;
    int w = boundingRect().width() - 2;
    line = QLine( 0, h - 1, w - 1, h - 1 );
    rect = QRect( 0, h, mRenderedMapImage.width(), mRenderedMapImage.height() - h );
  }

  painter->drawImage(
    rect,
    mRenderedMapImage.scaled( mRenderedMapImage.width() / mRenderedMapImage.devicePixelRatioF(), mRenderedMapImage.height() / mRenderedMapImage.devicePixelRatioF() ),
    rect
  );

  QPen pen( Qt::black, 3, Qt::SolidLine );
  painter->setPen( pen );
  painter->drawLine( line );
}
