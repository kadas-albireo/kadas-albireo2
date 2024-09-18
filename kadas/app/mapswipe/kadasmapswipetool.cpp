/***************************************************************************
    kadasmapswipetool.cpp
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


#include <qgsmessagebar.h>
#include <qgsmessagebaritem.h>
#include <qgsmapcanvas.h>
#include <qgsmapmouseevent.h>

#include "kadasmapswipetool.h"
#include "kadasmapswipecanvasitem.h"
#include "kadasapplication.h"
#include "kadasmainwindow.h"


KadasMapSwipeMapTool::KadasMapSwipeMapTool( QgsMapCanvas *mapCanvas )
  : QgsMapTool( mapCanvas )
  , mMapCanvasItem( new KadasMapSwipeCanvasItem(mapCanvas) )
{
  connect(QgsProject::instance(), qOverload<const QList< QgsMapLayer * > & >( &QgsProject::layersWillBeRemoved ), this, [=]( const QList<QgsMapLayer *> &layers ){
    for ( QgsMapLayer *layer : layers )
      mLayers.remove( layer );
    mMapCanvasItem->setLayers( mLayers );
    if ( mLayers.count() == 0 )
    {
      deactivate();
    }
  });
}

void KadasMapSwipeMapTool::addLayers( const QList<QgsMapLayer *> &layers )
{
  for ( QgsMapLayer *layer : layers )
    mLayers.insert( layer );
  mMapCanvasItem->setLayers( mLayers );

  QStringList layerNames;
  std::transform( mLayers.constBegin(), mLayers.constEnd(), std::back_inserter( layerNames ), []( const auto & layer ){ return layer->name(); } );

  QgsMessageBar *messageBar = KadasApplication::instance()->mainWindow()->messageBar();
  mMessageBarItem = messageBar->createMessage( tr("Swipe Tool" ), tr( "Comparing Layers %1" ).arg(layerNames.join( QStringLiteral( ", " ) ) ) );
  if ( mMessageBarItem )
    messageBar->popWidget( mMessageBarItem );
  messageBar->pushItem( mMessageBarItem );
}

bool KadasMapSwipeMapTool::isActive() const
{
  return mIsActive && !mLayers.isEmpty();
}

void KadasMapSwipeMapTool::activate()
{
  QgsMapTool::activate();
  canvas()->setCursor( QCursor( Qt::PointingHandCursor ) );
  mMapCanvasItem->enable();
  mIsSwiping = false;
  mIsActive = true;
}

void KadasMapSwipeMapTool::deactivate()
{
  mMapCanvasItem->disable( true );
  mLayers.clear();
  mIsActive = false;
  mIsSwiping = false;

  if ( mMessageBarItem )
    KadasApplication::instance()->mainWindow()->messageBar()->popWidget( mMessageBarItem );

  QgsMapTool::deactivate();
  emit deactivated();
}


void KadasMapSwipeMapTool::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::RightButton )
  {
    mIsActive = false;
    mMapCanvasItem->disable();
  }
  else
  {
    mIsActive = true;
    mIsSwiping = true;
    mFirstPoint = QPoint( e->x(), e->y() );
    mDirectionDefined = false;
  }
}

void KadasMapSwipeMapTool::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( mIsActive && mIsSwiping )
  {
    if ( !mDirectionDefined )
    {
      double dX = abs( e->x() - mFirstPoint.x() );
      double dY = abs( e->y() - mFirstPoint.y() );
      bool isVertical = dX > dY;
      mMapCanvasItem->setVertical( isVertical );
      canvas()->setCursor( isVertical ? mCursorH : mCursorV );
      if ( dX*dX + dY*dY  > 50 )
          mDirectionDefined = true;
    }
    mMapCanvasItem->setPixeLength( e->x(), e->y() );
  }
}


void KadasMapSwipeMapTool::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( mIsActive )
  {
    mIsSwiping = false;
    canvas()->setCursor( QCursor( Qt::PointingHandCursor ) );
  }
}
