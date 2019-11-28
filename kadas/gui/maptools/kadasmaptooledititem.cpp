/***************************************************************************
    kadasmaptooledititem.cpp
    ------------------------
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

#include <QMenu>
#include <QPushButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssnappingutils.h>

#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptooledititemgroup.h>


KadasMapToolEditItem::KadasMapToolEditItem( QgsMapCanvas *canvas, const KadasItemLayer::ItemId &itemId, KadasItemLayer *layer )
  : QgsMapTool( canvas )
  , mLayer( layer )
{
  mItem = layer->takeItem( itemId );
  layer->triggerRepaint();
  KadasMapCanvasItemManager::addItem( mItem );
}

KadasMapToolEditItem::KadasMapToolEditItem( QgsMapCanvas *canvas, KadasMapItem *item, KadasItemLayer *layer )
  : QgsMapTool( canvas )
  , mLayer( layer )
  , mItem( item )
{
  KadasMapCanvasItemManager::addItem( mItem );
}

void KadasMapToolEditItem::activate()
{
  QgsMapTool::activate();
  setCursor( Qt::ArrowCursor );
  mStateHistory = new KadasStateHistory( this );
  mStateHistory->push( mItem->constState()->clone() );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolEditItem::stateChanged );

  mSnapping = QgsSettings().value( "/kadas/snapping_enabled", false ).toBool();

  mBottomBar = new KadasBottomBar( canvas() );
  mBottomBar->setLayout( new QHBoxLayout() );
  mBottomBar->layout()->setContentsMargins( 8, 4, 8, 4 );
  KadasMapItemEditor::Factory factory = KadasMapItemEditor::registry()->value( mItem->editor() );
  if ( factory )
  {
    mEditor = factory( mItem, KadasMapItemEditor::CreateItemEditor );
    mEditor->syncItemToWidget();
    mBottomBar->layout()->addWidget( mEditor );
  }
  mItem->setSelected( true );

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

  mBottomBar->show();
}

void KadasMapToolEditItem::deactivate()
{
  QgsMapTool::deactivate();
  if ( mItem )
  {
    mLayer->addItem( mItem );
    mLayer->triggerRepaint();
    KadasMapCanvasItemManager::removeItemAfterRefresh( mItem, mCanvas );
    mItem->setSelected( false );
    mItem = nullptr;
  }
  delete mBottomBar;
  mBottomBar = nullptr;
  delete mStateHistory;
  mStateHistory = nullptr;
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolEditItem::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier )
  {
    KadasItemLayer::ItemId itemId = mLayer->pickItem( e->mapPoint(), mCanvas->mapSettings() );
    if ( itemId != KadasItemLayer::ITEM_ID_NULL )
    {
      KadasMapItem *otherItem = mLayer->takeItem( itemId );
      KadasMapCanvasItemManager::removeItem( mItem );
      KadasMapItem *item = mItem;
      mItem = nullptr;
      mLayer->triggerRepaint();
      mCanvas->setMapTool( new KadasMapToolEditItemGroup( mCanvas, QList<KadasMapItem *>() << item << otherItem, mLayer ) );
    }
  }
  if ( e->button() == Qt::RightButton )
  {
    KadasMapPos pos( e->mapPoint().x(), e->mapPoint().y() );
    mEditContext = mItem->getEditContext( pos, canvas()->mapSettings() );
    if ( mEditContext.isValid() )
    {
      QMenu menu;
      menu.addAction( mItem->itemName() )->setEnabled( false );
      menu.addSeparator();
      int count = menu.actions().size();
      mItem->populateContextMenu( &menu, mEditContext, pos, mCanvas->mapSettings() );
      if ( menu.actions().size() > count )
      {
        menu.addSeparator();
      }
      menu.addAction( QIcon( ":/images/themes/default/mActionEditCut.svg" ), tr( "Cut" ), this, &KadasMapToolEditItem::cutItem );
      menu.addAction( QIcon( ":/images/themes/default/mActionEditCopy.svg" ), tr( "Copy" ), this, &KadasMapToolEditItem::copyItem );
      menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, &KadasMapToolEditItem::deleteItem );
      QAction *clickedAction = menu.exec( e->globalPos() );

      if ( clickedAction )
      {
        if ( clickedAction->data() == KadasMapItem::EditSwitchToDrawingTool )
        {
          KadasMapCanvasItemManager::removeItem( mItem );
          KadasMapItem *item = mItem;
          mItem = nullptr;
          canvas()->setMapTool( new KadasMapToolCreateItem( canvas(), item, mLayer ) );
        }
      }
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}

void KadasMapToolEditItem::canvasDoubleClickEvent( QgsMapMouseEvent *e )
{
  mItem->onDoubleClick( mCanvas->mapSettings() );
}

void KadasMapToolEditItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  KadasMapPos pos = transformMousePoint( e->mapPoint() );

  if ( e->buttons() == Qt::LeftButton )
  {
    if ( mEditContext.isValid() )
    {
      mItem->edit( mEditContext, KadasMapPos( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() ), canvas()->mapSettings() );
    }
  }
  else
  {
    KadasMapItem::EditContext oldContext = mEditContext;
    mEditContext = mItem->getEditContext( pos, canvas()->mapSettings() );
    if ( !mEditContext.isValid() )
    {
      setCursor( Qt::ArrowCursor );
      clearNumericInput();
    }
    else
    {
      if ( mEditContext != oldContext )
      {
        setCursor( mEditContext.cursor );
        setupNumericInput();
      }
      mMoveOffset = QgsVector( pos.x() - mEditContext.pos.x(), pos.y() - mEditContext.pos.y() );
    }
  }
  if ( mInputWidget && mEditContext.isValid() )
  {
    mInputWidget->ensureFocus();

    KadasMapItem::AttribValues values = mItem->editAttribsFromPosition( mEditContext, KadasMapPos( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() ), canvas()->mapSettings() );
    for ( auto it = values.begin(), itEnd = values.end(); it != itEnd; ++it )
    {
      mInputWidget->inputField( it.key() )->setValue( it.value() );
    }

    mInputWidget->move( e->x(), e->y() + 20 );
    mInputWidget->show();
    if ( mInputWidget->focusedInputField() )
    {
      mInputWidget->focusedInputField()->setFocus();
      mInputWidget->focusedInputField()->selectAll();
    }
  }
}

void KadasMapToolEditItem::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton && mEditContext.isValid() )
  {
    mStateHistory->push( mItem->constState()->clone() );
  }
}

void KadasMapToolEditItem::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
  else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier )
  {
    mStateHistory->undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier )
  {
    mStateHistory->redo();
  }
  else if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copyItem();
  }
  else if ( e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier )
  {
    cutItem();
  }
  else if ( e->key() == Qt::Key_Delete )
  {
    deleteItem();
  }
}

void KadasMapToolEditItem::setupNumericInput()
{
  clearNumericInput();
  if ( QgsSettings().value( "/kadas/showNumericInput", false ).toBool() && mEditContext.isValid() && !mEditContext.attributes.isEmpty() )
  {
    mInputWidget = new KadasFloatingInputWidget( canvas() );

    const KadasMapItem::AttribDefs &attributes = mEditContext.attributes;
    for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it )
    {
      const KadasMapItem::NumericAttribute &attribute = it.value();
      KadasFloatingInputWidgetField *attrEdit = new KadasFloatingInputWidgetField( it.key(), attribute.decimals, attribute.min, attribute.max );
      connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolEditItem::inputChanged );
      mInputWidget->addInputField( attribute.name + ":", attrEdit );
    }
    if ( !attributes.isEmpty() )
    {
      mInputWidget->setFocusedInputField( mInputWidget->inputField( attributes.begin().key() ) );
    }
  }
}

void KadasMapToolEditItem::clearNumericInput()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolEditItem::stateChanged( KadasStateHistory::State *state )
{
  mItem->setState( static_cast<const KadasMapItem::State *>( state ) );
}

KadasMapItem::AttribValues KadasMapToolEditItem::collectAttributeValues() const
{
  KadasMapItem::AttribValues attributes;

  for ( const KadasFloatingInputWidgetField *field : mInputWidget->inputFields() )
  {
    attributes.insert( field->id(), field->text().toDouble() );
  }
  return attributes;
}

void KadasMapToolEditItem::inputChanged()
{
  if ( mEditContext.isValid() )
  {

    KadasMapItem::AttribValues values = collectAttributeValues();

    // Ignore the move event emitted by re-positioning the mouse cursor:
    // The widget mouse coordinates (stored in a integer QPoint) loses precision,
    // and mapping it back to map coordinates in the mouseMove event handler
    // results in a position different from geoPos, and hence the user-input
    // may get altered
    mIgnoreNextMoveEvent = true;

    KadasMapPos newPos = mItem->positionFromEditAttribs( mEditContext, values, mCanvas->mapSettings() );
    mInputWidget->adjustCursorAndExtent( newPos );

    mItem->edit( mEditContext, values, canvas()->mapSettings() );
    mStateHistory->push( mItem->constState()->clone() );
  }
}

void KadasMapToolEditItem::copyItem()
{
  KadasClipboard::instance()->setStoredMapItems( QList<KadasMapItem *>() << mItem );
}

void KadasMapToolEditItem::cutItem()
{
  KadasClipboard::instance()->setStoredMapItems( QList<KadasMapItem *>() << mItem );
  deleteItem();
}

void KadasMapToolEditItem::deleteItem()
{
  delete mEditor;
  mEditor = nullptr;

  delete mItem;
  mItem = nullptr;
  canvas()->unsetMapTool( this );
}

KadasMapPos KadasMapToolEditItem::transformMousePoint( QgsPointXY mapPos ) const
{
  if ( mSnapping )
  {
    QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap( mapPos );
    if ( m.isValid() )
    {
      mapPos = m.point();
    }
  }
  return KadasMapPos( mapPos.x(), mapPos.y() );
}
