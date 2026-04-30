/***************************************************************************
    kadasmaptooleditannotationitem.cpp
    ----------------------------------
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

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"


KadasMapToolEditAnnotationItem::ToolState::~ToolState()
{
  delete itemClone;
}


KadasMapToolEditAnnotationItem::KadasMapToolEditAnnotationItem( QgsMapCanvas *canvas, QgsAnnotationLayer *layer, const QString &itemId )
  : QgsMapTool( canvas )
  , mLayer( layer )
  , mItemId( itemId )
{
  if ( layer )
  {
    mItem = layer->item( itemId );
    if ( mItem )
      mController = KadasAnnotationControllerRegistry::instance()->controllerFor( mItem->type() );
  }
}

void KadasMapToolEditAnnotationItem::activate()
{
  QgsMapTool::activate();
  setCursor( Qt::ArrowCursor );
  if ( !mItem || !mController || !mLayer )
  {
    canvas()->unsetMapTool( this );
    return;
  }

  mStateHistory = new KadasStateHistory( this );
  mStateHistory->push( new ToolState( mItem->clone() ) );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolEditAnnotationItem::stateChanged );

  mBottomBar = new KadasBottomBar( canvas() );
  mBottomBar->setLayout( new QHBoxLayout() );
  mBottomBar->layout()->setContentsMargins( 8, 4, 8, 4 );

  QLabel *label = new QLabel( tr( "Edit %1" ).arg( mController->itemName() ) );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );
  mBottomBar->layout()->addWidget( label );

  QPushButton *undoButton = new QPushButton();
  undoButton->setIcon( QIcon( ":/kadas/icons/undo" ) );
  undoButton->setToolTip( tr( "Undo" ) );
  undoButton->setEnabled( false );
  connect( undoButton, &QPushButton::clicked, this, [this] { mStateHistory->undo(); } );
  connect( mStateHistory, &KadasStateHistory::canUndoChanged, undoButton, &QPushButton::setEnabled );
  mBottomBar->layout()->addWidget( undoButton );

  QPushButton *redoButton = new QPushButton();
  redoButton->setIcon( QIcon( ":/kadas/icons/redo" ) );
  redoButton->setToolTip( tr( "Redo" ) );
  redoButton->setEnabled( false );
  connect( redoButton, &QPushButton::clicked, this, [this] { mStateHistory->redo(); } );
  connect( mStateHistory, &KadasStateHistory::canRedoChanged, redoButton, &QPushButton::setEnabled );
  mBottomBar->layout()->addWidget( redoButton );

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  mBottomBar->layout()->addWidget( closeButton );

  mBottomBar->adjustSize();
  mBottomBar->show();
}

void KadasMapToolEditAnnotationItem::deactivate()
{
  QgsMapTool::deactivate();
  if ( mLayer )
    mLayer->triggerRepaint();
  delete mBottomBar;
  mBottomBar = nullptr;
  delete mStateHistory;
  mStateHistory = nullptr;
}

void KadasMapToolEditAnnotationItem::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( !mItem || !mController || !mLayer )
    return;

  if ( mPressedButton != Qt::NoButton )
    return;
  mPressedButton = e->button();

  if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void KadasMapToolEditAnnotationItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mItem || !mController || !mLayer )
    return;

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );
  const KadasMapPos pos = KadasMapPos::fromPoint( e->mapPoint() );

  if ( e->buttons() == Qt::LeftButton )
  {
    if ( mEditContext.isValid() )
    {
      const KadasMapPos adjusted( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() );
      mController->edit( mItem, mEditContext, adjusted, ctx );
      mLayer->triggerRepaint();
    }
  }
  else
  {
    KadasMapItem::EditContext oldContext = mEditContext;
    mEditContext = mController->getEditContext( mItem, pos, ctx );
    if ( !mEditContext.isValid() )
    {
      setCursor( Qt::ArrowCursor );
    }
    else if ( mEditContext != oldContext )
    {
      setCursor( mEditContext.cursor );
      mMoveOffset = QgsVector( pos.x() - mEditContext.pos.x(), pos.y() - mEditContext.pos.y() );
    }
    else
    {
      mMoveOffset = QgsVector( pos.x() - mEditContext.pos.x(), pos.y() - mEditContext.pos.y() );
    }
  }
}

void KadasMapToolEditAnnotationItem::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  mPressedButton = Qt::NoButton;
  if ( e->button() == Qt::LeftButton && mEditContext.isValid() )
    pushState();
}

void KadasMapToolEditAnnotationItem::canvasDoubleClickEvent( QgsMapMouseEvent * )
{
  if ( !mItem || !mController || !mLayer )
    return;
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings() );
  mController->onDoubleClick( mItem, ctx );
  mLayer->triggerRepaint();
}

void KadasMapToolEditAnnotationItem::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
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
  else if ( e->key() == Qt::Key_Delete )
  {
    deleteItem();
  }
}

void KadasMapToolEditAnnotationItem::pushState()
{
  if ( mStateHistory && mItem )
    mStateHistory->push( new ToolState( mItem->clone() ) );
}

void KadasMapToolEditAnnotationItem::deleteItem()
{
  if ( !mLayer || mItemId.isEmpty() )
    return;
  mLayer->removeItem( mItemId );
  mLayer->triggerRepaint();
  mItem = nullptr;
  mItemId.clear();
  canvas()->unsetMapTool( this );
}

void KadasMapToolEditAnnotationItem::stateChanged( KadasStateHistory::ChangeType, KadasStateHistory::State *state, KadasStateHistory::State * /*prevState*/ )
{
  if ( !mLayer || mItemId.isEmpty() )
    return;
  ToolState *ts = static_cast<ToolState *>( state );
  if ( !ts || !ts->itemClone )
    return;
  QgsAnnotationItem *replacement = ts->itemClone->clone();
  mLayer->replaceItem( mItemId, replacement );
  mItem = mLayer->item( mItemId );
  mLayer->triggerRepaint();
  mEditContext = KadasMapItem::EditContext();
}
