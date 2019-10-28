/***************************************************************************
    kadasmaptooledititemgroup.cpp
    -----------------------------
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

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/gui/mapitems/kadasselectionrectitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptooledititemgroup.h>

KadasMapToolEditItemGroup::KadasMapToolEditItemGroup( QgsMapCanvas *canvas, const QList<KadasMapItem *> &items, KadasItemLayer *layer )
  : QgsMapTool( canvas ), mItems( items ), mLayer( layer )
{
}

void KadasMapToolEditItemGroup::activate()
{
  for ( KadasMapItem *item : mItems )
  {
    item->setSelected( true );
    KadasMapCanvasItemManager::addItem( item );
  }

  mSelectionRect = new KadasSelectionRectItem( canvas()->mapSettings().destinationCrs() );
  KadasMapCanvasItemManager::addItem( mSelectionRect );

  mBottomBar = new KadasBottomBar( canvas() );
  mBottomBar->setLayout( new QHBoxLayout() );
  mBottomBar->layout()->setContentsMargins( 8, 4, 8, 4 );
  mStatusLabel = new QLabel();
  mBottomBar->layout()->addWidget( mStatusLabel );
  updateSelection();

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  mBottomBar->layout()->addWidget( closeButton );

  mBottomBar->show();
}

void KadasMapToolEditItemGroup::deactivate()
{
  QgsMapTool::deactivate();
  while ( !mItems.isEmpty() )
  {
    deselectItem( mItems.front(), false );
  }
  mLayer->triggerRepaint();

  KadasMapCanvasItemManager::removeItem( mSelectionRect );
  delete mSelectionRect;
  mSelectionRect = nullptr;

  delete mBottomBar;
  mBottomBar = nullptr;
  mStatusLabel = nullptr;
}

void KadasMapToolEditItemGroup::canvasPressEvent( QgsMapMouseEvent *e )
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( mCanvas->mapSettings() );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  KadasMapRect filterRect;
  filterRect.setXMinimum( e->mapPoint().x() - radiusmu );
  filterRect.setXMaximum( e->mapPoint().x() + radiusmu );
  filterRect.setYMinimum( e->mapPoint().y() - radiusmu );
  filterRect.setYMaximum( e->mapPoint().y() + radiusmu );

  if ( e->button() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier )
  {
    // First, test selected items
    for ( KadasMapItem *item : mItems )
    {
      if ( item->intersects( filterRect, mCanvas->mapSettings() ) )
      {
        deselectItem( item );
        updateSelection();

        if ( mItems.size() == 1 )
        {
          KadasMapItem *item = mItems.front();
          KadasMapCanvasItemManager::removeItem( item );
          mItems.clear();
          canvas()->setMapTool( new KadasMapToolEditItem( mCanvas, item, mLayer ) );
        }

        return;
      }
    }
    // Then, test layer for new items to select
    QString itemId = mLayer->pickItem( filterRect, mCanvas->mapSettings() );
    if ( !itemId.isEmpty() )
    {
      KadasMapItem *item = mLayer->takeItem( itemId );
      item->setSelected( true );
      mItems.append( item );
      updateSelection();
      KadasMapCanvasItemManager::addItem( item );
      mLayer->triggerRepaint();
    }
  }
  else if ( e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier )
  {
    mMoveRefPos = e->mapPoint();
    for ( KadasMapItem *item : mItems )
    {
      QgsCoordinateTransform crst( item->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
      mItemRefPos.append( crst.transform( item->position() ) );
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    if ( mSelectionRect->intersects( filterRect, mCanvas->mapSettings() ) )
    {
      QMenu menu;
      menu.addAction( tr( "Cut Selection" ), this, &KadasMapToolEditItemGroup::cutItems );
      menu.addAction( tr( "Copy Selection" ), this, &KadasMapToolEditItemGroup::copyItems );
      menu.addAction( tr( "Delete Selection" ), this, &KadasMapToolEditItemGroup::deleteItems );
      menu.exec( e->globalPos() );
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}

void KadasMapToolEditItemGroup::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( e->buttons() == Qt::LeftButton && e->modifiers() == Qt::NoModifier )
  {
    QgsVector delta = e->mapPoint() - mMoveRefPos;
    for ( int i = 0, n = mItems.size(); i < n; ++i )
    {
      QgsCoordinateTransform crst( mCanvas->mapSettings().destinationCrs(), mItems[i]->crs(), QgsProject::instance() );
      QgsPointXY newPos = crst.transform( mItemRefPos[i] + delta, QgsCoordinateTransform::ReverseTransform );
      mItems[i]->setPosition( KadasItemPos::fromPoint( newPos ) );
    }
    mSelectionRect->update();
  }
}

void KadasMapToolEditItemGroup::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier )
  {
    mItemRefPos.clear();
  }
}

void KadasMapToolEditItemGroup::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
  else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier )
  {
//    mStateHistory->undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier )
  {
//    mStateHistory->redo();
  }
  else if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copyItems();
  }
  else if ( e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier )
  {
    cutItems();
  }
  else if ( e->key() == Qt::Key_Delete )
  {
    deleteItems();
  }
}

void KadasMapToolEditItemGroup::copyItems()
{
  KadasClipboard::instance()->setStoredMapItems( mItems );
}

void KadasMapToolEditItemGroup::cutItems()
{
  KadasClipboard::instance()->setStoredMapItems( mItems );
  deleteItems();
}

void KadasMapToolEditItemGroup::deleteItems()
{
  qDeleteAll( mItems );
  mItems.clear();
  canvas()->unsetMapTool( this );
}

void KadasMapToolEditItemGroup::deselectItem( KadasMapItem *item, bool triggerRepaint )
{
  item->setSelected( false );
  mItems.removeAll( item );
  mLayer->addItem( item );
  if ( triggerRepaint )
  {
    mLayer->triggerRepaint();
  }
  KadasMapCanvasItemManager::removeItemAfterRefresh( item, mCanvas );
}

void KadasMapToolEditItemGroup::updateSelection()
{
  mStatusLabel->setText( tr( "%1 item(s) selected on layer %2" ).arg( mItems.size() ).arg( mLayer->name() ) );
  mSelectionRect->setSelectedItems( mItems );
}
