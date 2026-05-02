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
#include <qgis/qgssettings.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfloatinginputwidget.h"
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
  setupNumericInputWidget();

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
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolCreateAnnotationItem::createItem()
{
  if ( !mLayer || !mController )
    return;

  mItem = mItemFactory ? mItemFactory() : mController->createItem();
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
    addPoint( e->mapPoint() );
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
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  if ( mDrawState != DrawState::Drawing )
    return;

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  const QgsPointXY pos = e->mapPoint();
  mController->setCurrentPoint( mItem, pos, ctx );
  mLayer->triggerRepaint();

  if ( mInputWidget )
  {
    mInputWidget->ensureFocus();
    KadasAttribValues values = mController->drawAttribsFromPosition( mItem, pos, ctx );
    for ( auto it = values.begin(), itEnd = values.end(); it != itEnd; ++it )
      mInputWidget->inputField( it.key() )->setValue( it.value() );
    mInputWidget->move( e->position().x(), e->position().y() + 20 );
    mInputWidget->show();
    if ( mInputWidget->focusedInputField() )
    {
      mInputWidget->focusedInputField()->setFocus();
      mInputWidget->focusedInputField()->selectAll();
    }
  }
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

void KadasMapToolCreateAnnotationItem::addPoint( const QgsPointXY &pos )
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );

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

void KadasMapToolCreateAnnotationItem::startPart( const QgsPointXY &pos )
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
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

void KadasMapToolCreateAnnotationItem::startPart( const KadasAttribValues &values )
{
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  if ( !mController->startPart( mItem, values, ctx ) )
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
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
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

void KadasMapToolCreateAnnotationItem::setupNumericInputWidget()
{
  delete mInputWidget;
  mInputWidget = nullptr;
  if ( !QgsSettings().value( "/kadas/showNumericInput", false ).toBool() )
    return;
  if ( !mController )
    return;

  const KadasAttribDefs attribs = mController->drawAttribs();
  if ( attribs.isEmpty() )
    return;

  mInputWidget = new KadasFloatingInputWidget( canvas() );
  for ( auto it = attribs.begin(), itEnd = attribs.end(); it != itEnd; ++it )
  {
    const KadasNumericAttribute &attribute = it.value();
    KadasFloatingInputWidgetField *attrEdit = new KadasFloatingInputWidgetField( it.key(), attribute.precision( mCanvas->mapSettings() ), attribute.min, attribute.max );
    connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolCreateAnnotationItem::inputChanged );
    connect( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolCreateAnnotationItem::acceptInput );
    mInputWidget->addInputField( attribute.name + ":", attrEdit, attribute.suffix( mCanvas->mapSettings() ) );
  }
  mInputWidget->setFocusedInputField( mInputWidget->inputField( attribs.begin().key() ) );
}

KadasAttribValues KadasMapToolCreateAnnotationItem::collectAttributeValues() const
{
  KadasAttribValues attributes;
  if ( !mInputWidget )
    return attributes;
  for ( const KadasFloatingInputWidgetField *field : mInputWidget->inputFields() )
    attributes.insert( field->id(), field->text().toDouble() );
  return attributes;
}

void KadasMapToolCreateAnnotationItem::inputChanged()
{
  if ( !mItem || !mController || !mLayer || !mInputWidget )
    return;
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  const KadasAttribValues values = collectAttributeValues();

  // Suppress the move event triggered by the cursor reposition below so the
  // mouse-move handler does not overwrite the user's typed values via
  // setCurrentPoint.
  mIgnoreNextMoveEvent = true;
  const QgsPointXY newPos = mController->positionFromDrawAttribs( mItem, values, ctx );
  mInputWidget->adjustCursorAndExtent( newPos );

  if ( mDrawState == DrawState::Drawing )
  {
    mController->setCurrentAttributes( mItem, values, ctx );
    mLayer->triggerRepaint();
  }
}

void KadasMapToolCreateAnnotationItem::acceptInput()
{
  if ( !mItem || !mController || !mLayer )
    return;
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );

  switch ( mDrawState )
  {
    case DrawState::Empty:
      startPart( collectAttributeValues() );
      break;
    case DrawState::Drawing:
      if ( !mController->continuePart( mItem, ctx ) )
        finishPart();
      else
      {
        mLayer->triggerRepaint();
        pushState();
      }
      break;
    case DrawState::Finished:
      if ( !mMultipart )
      {
        clearInProgress();
        createItem();
        emit cleared();
      }
      startPart( collectAttributeValues() );
      break;
  }
}
