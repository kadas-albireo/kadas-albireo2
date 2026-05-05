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
#include <QVBoxLayout>
#include <memory>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfloatinginputwidget.h"
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
  mDrawState = DrawState::Finished;
  mAllowCreate = false;
}

KadasMapToolEditAnnotationItem::KadasMapToolEditAnnotationItem( QgsMapCanvas *canvas, KadasAnnotationItemController *controller, QgsAnnotationLayer *layer )
  : QgsMapTool( canvas )
  , mController( controller )
  , mLayer( layer )
{
  mDrawState = DrawState::Empty;
  mAllowCreate = true;
}

void KadasMapToolEditAnnotationItem::activate()
{
  QgsMapTool::activate();
  setCursor( Qt::ArrowCursor );
  if ( !mController || !mLayer )
  {
    canvas()->unsetMapTool( this );
    return;
  }

  if ( mAllowCreate && !mItem )
  {
    createInitialItem();
    if ( !mItem )
    {
      canvas()->unsetMapTool( this );
      return;
    }
  }
  else if ( !mItem )
  {
    canvas()->unsetMapTool( this );
    return;
  }

  mStateHistory = new KadasStateHistory( this );
  mStateHistory->push( new ToolState( mItem->clone(), mDrawState ) );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolEditAnnotationItem::stateChanged );

  mBottomBar = new KadasBottomBar( canvas() );
  auto *outer = new QVBoxLayout();
  outer->setContentsMargins( 8, 4, 8, 4 );
  mBottomBar->setLayout( outer );

  auto *topRow = new QHBoxLayout();
  outer->addLayout( topRow );

  QLabel *label = new QLabel( ( mAllowCreate ? tr( "Draw %1" ) : tr( "Edit %1" ) ).arg( mController->itemName() ) );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );
  topRow->addWidget( label );

  QPushButton *undoButton = new QPushButton();
  undoButton->setIcon( QIcon( ":/kadas/icons/undo" ) );
  undoButton->setToolTip( tr( "Undo" ) );
  undoButton->setEnabled( false );
  connect( undoButton, &QPushButton::clicked, this, [this] { mStateHistory->undo(); } );
  connect( mStateHistory, &KadasStateHistory::canUndoChanged, undoButton, &QPushButton::setEnabled );
  topRow->addWidget( undoButton );

  QPushButton *redoButton = new QPushButton();
  redoButton->setIcon( QIcon( ":/kadas/icons/redo" ) );
  redoButton->setToolTip( tr( "Redo" ) );
  redoButton->setEnabled( false );
  connect( redoButton, &QPushButton::clicked, this, [this] { mStateHistory->redo(); } );
  connect( mStateHistory, &KadasStateHistory::canRedoChanged, redoButton, &QPushButton::setEnabled );
  topRow->addWidget( redoButton );

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  topRow->addWidget( closeButton );

  setupStyleEditor( outer );

  mBottomBar->adjustSize();
  mBottomBar->show();
}

void KadasMapToolEditAnnotationItem::deactivate()
{
  QgsMapTool::deactivate();
  // Drop the uncommitted in-progress item if the user closed the tool mid-draw.
  if ( mAllowCreate && mDrawState != DrawState::Finished )
    clearInProgressItem();
  if ( mLayer )
    mLayer->triggerRepaint();
  delete mBottomBar;
  mBottomBar = nullptr;
  // Style editor was a child of mBottomBar; its pointer is now dangling.
  mStyleEditor = nullptr;
  delete mStateHistory;
  mStateHistory = nullptr;
  clearNumericInput();
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
    // Right-click during digitizing finalizes the part; otherwise close.
    if ( mAllowCreate && mDrawState == DrawState::InProgress )
      finishPart();
    else
      canvas()->unsetMapTool( this );
    return;
  }

  if ( e->button() != Qt::LeftButton )
    return;

  // Click on a vertex/handle is handled by canvasMoveEvent (drag) +
  // canvasReleaseEvent (pushState). Nothing to do here.
  if ( mEditContext.isValid() )
    return;

  // Outside of a digitizing part, a click on a different item on the same
  // layer switches the tool to editing that item, instead of starting a
  // brand new one (or doing nothing in pure edit mode).
  if ( mDrawState != DrawState::InProgress )
  {
    const QString hit = pickItemAt( e->mapPoint() );
    if ( !hit.isEmpty() && hit != mItemId )
    {
      switchToItem( hit );
      return;
    }
  }

  if ( !mAllowCreate )
    return;

  addPoint( e->mapPoint() );
}

void KadasMapToolEditAnnotationItem::addPoint( const QgsPointXY &pos )
{
  if ( !mAllowCreate || !mItem || !mController || !mLayer )
    return;

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );

  switch ( mDrawState )
  {
    case DrawState::Empty:
      startPart( pos );
      break;

    case DrawState::InProgress:
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
        // Detach from the just-finalized item (it stays on the layer) and
        // create a fresh one in its place.
        mItem = nullptr;
        mItemId.clear();
        createInitialItem();
        if ( !mItem )
          return;
        if ( mStyleEditor )
          mStyleEditor->loadFromItem( mItem );
        emit cleared();
      }
      startPart( pos );
      break;
  }
}

void KadasMapToolEditAnnotationItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mItem || !mController || !mLayer )
    return;
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  const QgsPointXY pos = e->mapPoint();

  // While actively digitizing a part, skip vertex hit-testing entirely:
  // the just-placed vertex sits at (or very near) the cursor and would
  // make mEditContext flip-flop, causing flicker and unnecessary work.
  if ( mAllowCreate && mDrawState == DrawState::InProgress )
  {
    if ( mEditContext.isValid() )
    {
      mEditContext = KadasEditContext();
      setCursor( Qt::ArrowCursor );
      clearNumericInput();
    }
    mController->setCurrentPoint( mItem, pos, ctx );
    mLayer->triggerRepaint();
    return;
  }

  if ( e->buttons() == Qt::LeftButton )
  {
    if ( mEditContext.isValid() )
    {
      const QgsPointXY adjusted( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() );
      mController->edit( mItem, mEditContext, adjusted, ctx );
      mLayer->triggerRepaint();
    }
  }
  else
  {
    KadasEditContext oldContext = mEditContext;
    mEditContext = mController->getEditContext( mItem, pos, ctx );
    if ( !mEditContext.isValid() )
    {
      setCursor( Qt::ArrowCursor );
      clearNumericInput();
    }
    else if ( mEditContext != oldContext )
    {
      setCursor( mEditContext.cursor );
      mMoveOffset = QgsVector( pos.x() - mEditContext.pos.x(), pos.y() - mEditContext.pos.y() );
      setupNumericInput();
    }
    else
    {
      mMoveOffset = QgsVector( pos.x() - mEditContext.pos.x(), pos.y() - mEditContext.pos.y() );
    }
  }

  if ( mInputWidget && mEditContext.isValid() )
  {
    mInputWidget->ensureFocus();
    const QgsPointXY adjusted( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() );
    KadasAttribValues values = mController->editAttribsFromPosition( mItem, mEditContext, adjusted, ctx );
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
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  mController->onDoubleClick( mItem, ctx );
  mLayer->triggerRepaint();
}

void KadasMapToolEditAnnotationItem::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mAllowCreate && mDrawState == DrawState::InProgress )
    {
      // Discard the current part and start fresh.
      clearInProgressItem();
      createInitialItem();
      if ( mItem )
      {
        if ( mStyleEditor )
          mStyleEditor->loadFromItem( mItem );
        emit cleared();
      }
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
  else if ( ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter ) && mAllowCreate && mDrawState == DrawState::InProgress )
  {
    finishPart();
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
    mStateHistory->push( new ToolState( mItem->clone(), mDrawState ) );
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
  mDrawState = ts->drawState;
  mLayer->triggerRepaint();
  mEditContext = KadasEditContext();
  clearNumericInput();
  if ( mStyleEditor )
    mStyleEditor->loadFromItem( mItem );
}

void KadasMapToolEditAnnotationItem::setupNumericInput()
{
  clearNumericInput();
  if ( !QgsSettings().value( "/kadas/showNumericInput", false ).toBool() )
    return;
  if ( !mEditContext.isValid() || mEditContext.attributes.isEmpty() )
    return;

  mInputWidget = new KadasFloatingInputWidget( canvas() );
  const KadasAttribDefs &attributes = mEditContext.attributes;
  for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it )
  {
    const KadasNumericAttribute &attribute = it.value();
    KadasFloatingInputWidgetField *attrEdit = new KadasFloatingInputWidgetField( it.key(), attribute.precision( mCanvas->mapSettings() ), attribute.min, attribute.max );
    connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolEditAnnotationItem::inputChanged );
    mInputWidget->addInputField( attribute.name + ":", attrEdit, attribute.suffix( mCanvas->mapSettings() ) );
  }
  mInputWidget->setFocusedInputField( mInputWidget->inputField( attributes.begin().key() ) );
}

void KadasMapToolEditAnnotationItem::clearNumericInput()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

KadasAttribValues KadasMapToolEditAnnotationItem::collectAttributeValues() const
{
  KadasAttribValues values;
  if ( !mInputWidget )
    return values;
  for ( const KadasFloatingInputWidgetField *field : mInputWidget->inputFields() )
    values.insert( field->id(), field->text().toDouble() );
  return values;
}

void KadasMapToolEditAnnotationItem::inputChanged()
{
  if ( !mItem || !mController || !mLayer || !mInputWidget )
    return;
  if ( !mEditContext.isValid() )
    return;

  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  const KadasAttribValues values = collectAttributeValues();

  // Suppress the spurious move event triggered by adjustCursorAndExtent.
  mIgnoreNextMoveEvent = true;
  const QgsPointXY newPos = mController->positionFromEditAttribs( mItem, mEditContext, values, ctx );
  mInputWidget->adjustCursorAndExtent( newPos );

  mController->edit( mItem, mEditContext, values, ctx );
  mLayer->triggerRepaint();
  pushState();
}

void KadasMapToolEditAnnotationItem::setupStyleEditor( QBoxLayout *outer )
{
  if ( !mController )
    return;
  mStyleEditor = mController->createStyleEditor( mBottomBar );
  if ( !mStyleEditor )
    return;
  mStyleEditor->loadFromItem( mItem );
  outer->addWidget( mStyleEditor );

  // Live preview: apply state to the item and repaint, but don't push history.
  connect( mStyleEditor, &KadasAnnotationStyleEditor::previewChanged, this, [this] {
    if ( !mItem || !mLayer )
      return;
    mStyleEditor->applyToItem( mItem );
    mLayer->triggerRepaint();
  } );

  // Committed: apply, repaint, persist as defaults, and push a history entry.
  connect( mStyleEditor, &KadasAnnotationStyleEditor::committed, this, [this] {
    if ( !mItem || !mLayer )
      return;
    mStyleEditor->applyToItem( mItem );
    mLayer->triggerRepaint();
    if ( mController )
      mController->persistStyle( mItem );
    pushState();
  } );
}

void KadasMapToolEditAnnotationItem::createInitialItem()
{
  if ( !mLayer || !mController )
    return;
  QgsAnnotationItem *fresh = mItemFactory ? mItemFactory() : mController->createItem();
  if ( !fresh )
    return;
  mController->applyPersistedStyle( fresh );
  mItemId = mLayer->addItem( fresh );
  mItem = mLayer->item( mItemId );
  mDrawState = DrawState::Empty;
  mLayer->triggerRepaint();
}

void KadasMapToolEditAnnotationItem::clearInProgressItem()
{
  if ( mLayer && !mItemId.isEmpty() )
  {
    mLayer->removeItem( mItemId );
    mLayer->triggerRepaint();
  }
  mItem = nullptr;
  mItemId.clear();
  mDrawState = DrawState::Empty;
}

void KadasMapToolEditAnnotationItem::startPart( const QgsPointXY &pos )
{
  if ( !mItem || !mController || !mLayer )
    return;
  KadasAnnotationItemContext ctx( mLayer->crs(), canvas()->mapSettings(), mLayer );
  if ( !mController->startPart( mItem, pos, ctx ) )
  {
    finishPart();
  }
  else
  {
    mDrawState = DrawState::InProgress;
    mLayer->triggerRepaint();
    pushState();
  }
}

void KadasMapToolEditAnnotationItem::finishPart()
{
  if ( !mItem || !mController || !mLayer )
    return;
  mController->endPart( mItem );
  mDrawState = DrawState::Finished;
  mLayer->triggerRepaint();
  pushState();
  emit partFinished();
}

QString KadasMapToolEditAnnotationItem::pickItemAt( const QgsPointXY &mapPos ) const
{
  if ( !mLayer || !canvas() )
    return QString();
  QgsRenderContext rc = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
  double radiusmm = QgsSettings().value( QStringLiteral( "/Map/searchRadiusMM" ), Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  if ( radiusmm <= 0 )
    radiusmm = Qgis::DEFAULT_SEARCH_RADIUS_MM;
  const double radiusmu = radiusmm * rc.scaleFactor() * rc.mapToPixel().mapUnitsPerPixel();
  const QgsRectangle mapBounds( mapPos.x() - radiusmu, mapPos.y() - radiusmu, mapPos.x() + radiusmu, mapPos.y() + radiusmu );
  const QgsRectangle layerBounds = canvas()->mapSettings().mapToLayerCoordinates( mLayer.data(), mapBounds );
  QgsFeedback feedback;
  const QStringList hits = mLayer->itemsInBounds( layerBounds, rc, &feedback );
  return hits.isEmpty() ? QString() : hits.first();
}

void KadasMapToolEditAnnotationItem::switchToItem( const QString &itemId )
{
  if ( !canvas() || !mLayer || itemId.isEmpty() )
    return;
  // Replace the active tool with a fresh edit-mode tool bound to the
  // clicked item. The current tool is destroyed by setMapTool().
  canvas()->setMapTool( new KadasMapToolEditAnnotationItem( canvas(), mLayer.data(), itemId ) );
}
