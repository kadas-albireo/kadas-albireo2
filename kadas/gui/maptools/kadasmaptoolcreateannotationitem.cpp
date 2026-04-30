/***************************************************************************
    kadasmaptoolcreateannotationitem.cpp
    ------------------------------------
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

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/maptools/kadasmaptoolcreateannotationitem.h"


KadasMapToolCreateAnnotationItem::ToolState::~ToolState()
{
  delete itemClone;
}


KadasMapToolCreateAnnotationItem::KadasMapToolCreateAnnotationItem( QgsMapCanvas *canvas, KadasAnnotationItemController *controller, QgsAnnotationLayer *layer )
  : QgsMapTool( canvas )
  , mController( controller )
  , mLayer( layer )
{}

KadasMapToolCreateAnnotationItem::~KadasMapToolCreateAnnotationItem() = default;

void KadasMapToolCreateAnnotationItem::activate()
{
  QgsMapTool::activate();
  mStateHistory = new KadasStateHistory( this );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolCreateAnnotationItem::stateChanged );

  createItem();

  // Bottom bar with optional label + undo/redo + close.
  mBottomBar = new KadasBottomBar( mCanvas );
  QHBoxLayout *layout = new QHBoxLayout( mCanvas );
  layout->setContentsMargins( 8, 4, 8, 4 );
  if ( !mToolLabel.isEmpty() )
  {
    QLabel *label = new QLabel( mToolLabel );
    QFont font = label->font();
    font.setBold( true );
    label->setFont( font );
    layout->addWidget( label );
  }

  QPushButton *undoButton = new QPushButton();
  undoButton->setIcon( QIcon( ":/kadas/icons/undo" ) );
  undoButton->setToolTip( tr( "Undo" ) );
  undoButton->setEnabled( false );
  connect( undoButton, &QPushButton::clicked, this, [this] { mStateHistory->undo(); } );
  connect( mStateHistory, &KadasStateHistory::canUndoChanged, undoButton, &QPushButton::setEnabled );
  layout->addWidget( undoButton );

  QPushButton *redoButton = new QPushButton();
  redoButton->setIcon( QIcon( ":/kadas/icons/redo" ) );
  redoButton->setToolTip( tr( "Redo" ) );
  redoButton->setEnabled( false );
  connect( redoButton, &QPushButton::clicked, this, [this] { mStateHistory->redo(); } );
  connect( mStateHistory, &KadasStateHistory::canRedoChanged, redoButton, &QPushButton::setEnabled );
  layout->addWidget( redoButton );

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  layout->addWidget( closeButton );

  mBottomBar->setLayout( layout );
  mBottomBar->adjustSize();
  mBottomBar->show();
}

void KadasMapToolCreateAnnotationItem::deactivate()
{
  QgsMapTool::deactivate();

  // If the user closed the tool while no part was committed, drop the empty in-progress item.
  if ( mDrawState != DrawState::Finished )
    clearInProgress();
  mItem = nullptr;
  mItemId.clear();
  mDrawState = DrawState::Empty;

  delete mBottomBar;
  mBottomBar = nullptr;
  delete mStateHistory;
  mStateHistory = nullptr;
}

void KadasMapToolCreateAnnotationItem::createItem()
{
  if ( !mLayer || !mController )
    return;

  mItem = mController->createItem();
  mItemId = mLayer->addItem( mItem );
  mDrawState = DrawState::Empty;
  mLayer->triggerRepaint();
  pushState();
}

void KadasMapToolCreateAnnotationItem::clearInProgress()
{
  if ( mLayer && !mItemId.isEmpty() )
  {
    mLayer->removeItem( mItemId );
    mLayer->triggerRepaint();
  }
  mItem = nullptr;
  mItemId.clear();
}

void KadasMapToolCreateAnnotationItem::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( !mItem || !mLayer || !mController )
    return;

  if ( e->button() == Qt::LeftButton )
  {
    addPoint( KadasMapPos::fromPoint( e->mapPoint() ) );
  }
  else if ( e->button() == Qt::RightButton )
  {
    if ( mDrawState == DrawState::Drawing )
      finishPart();
    else
      canvas()->unsetMapTool( this );
  }
}

void KadasMapToolCreateAnnotationItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mItem || !mLayer || !mController )
    return;
  if ( mDrawState != DrawState::Drawing )
    return;

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );
  mController->setCurrentPoint( mItem, KadasMapPos::fromPoint( e->mapPoint() ), ctx );
  mLayer->triggerRepaint();
}

void KadasMapToolCreateAnnotationItem::keyPressEvent( QKeyEvent *e )
{
  if ( !mItem || !mLayer || !mController )
    return;

  if ( e->key() == Qt::Key_Escape )
  {
    if ( mDrawState == DrawState::Drawing )
    {
      // Discard the current part: rebuild a fresh empty item in place.
      clearInProgress();
      createItem();
      emit cleared();
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
  else if ( ( e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier ) || e->key() == Qt::Key_Backspace )
  {
    if ( mStateHistory )
      mStateHistory->undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier )
  {
    if ( mStateHistory )
      mStateHistory->redo();
  }
}

void KadasMapToolCreateAnnotationItem::addPoint( const KadasMapPos &pos )
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );

  switch ( mDrawState )
  {
    case DrawState::Empty:
      startPart( pos );
      break;

    case DrawState::Drawing:
      // Re-issue setCurrentPoint so the controller sees the click position
      // rather than a stale move position, then ask the controller whether
      // the item accepts further points.
      mController->setCurrentPoint( mItem, pos, ctx );
      if ( !mController->continuePart( mItem, ctx ) )
      {
        finishPart();
      }
      else
      {
        mLayer->triggerRepaint();
        pushState();
      }
      break;

    case DrawState::Finished:
      if ( !mMultipart )
      {
        // Single-part flow: drop the finished item from the layer (it's
        // been committed already) and start fresh in place.
        clearInProgress();
        createItem();
        emit cleared();
      }
      startPart( pos );
      break;
  }
}

void KadasMapToolCreateAnnotationItem::startPart( const KadasMapPos &pos )
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );
  if ( !mController->startPart( mItem, pos, ctx ) )
  {
    finishPart();
  }
  else
  {
    mDrawState = DrawState::Drawing;
    mLayer->triggerRepaint();
    pushState();
  }
}

void KadasMapToolCreateAnnotationItem::finishPart()
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );
  mController->endPart( mItem );
  mDrawState = DrawState::Finished;
  mLayer->triggerRepaint();
  pushState();
  emit partFinished();
}

void KadasMapToolCreateAnnotationItem::pushState()
{
  if ( !mStateHistory || !mItem )
    return;
  mStateHistory->push( new ToolState( mItem->clone(), mDrawState ) );
}

void KadasMapToolCreateAnnotationItem::stateChanged( KadasStateHistory::ChangeType, KadasStateHistory::State *state, KadasStateHistory::State * /*prevState*/ )
{
  if ( !mLayer || mItemId.isEmpty() )
    return;

  ToolState *ts = static_cast<ToolState *>( state );
  if ( !ts || !ts->itemClone )
    return;

  // Replace the in-place item with a fresh clone of the snapshot so the
  // history retains its own copy and the layer owns a separate one.
  QgsAnnotationItem *replacement = ts->itemClone->clone();
  mLayer->replaceItem( mItemId, replacement );
  mItem = mLayer->item( mItemId );
  mDrawState = ts->drawState;
  mLayer->triggerRepaint();
}
