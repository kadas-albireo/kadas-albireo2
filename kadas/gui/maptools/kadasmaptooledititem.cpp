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

#include <QPushButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>

KadasMapToolEditItem::KadasMapToolEditItem( QgsMapCanvas *canvas, const QString &itemId, KadasItemLayer *layer )
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

KadasMapToolEditItem::~KadasMapToolEditItem()
{

}

void KadasMapToolEditItem::activate()
{
  QgsMapTool::activate();
  setCursor( Qt::ArrowCursor );
  mStateHistory = new KadasStateHistory( this );
  mStateHistory->push( mItem->constState()->clone() );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolEditItem::stateChanged );

  mBottomBar = new KadasBottomBar( canvas() );
  mBottomBar->setLayout( new QHBoxLayout() );
  if ( mItem->getEditorFactory() )
  {
    KadasMapItemEditor *editor = mItem->getEditorFactory()( mItem );
    editor->syncItemToWidget();
    mBottomBar->layout()->addWidget( editor );
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
    KadasMapItem *item = mItem;
    QObject *scope = new QObject;
    connect( mCanvas, &QgsMapCanvas::mapCanvasRefreshed, scope, [item, scope] { KadasMapCanvasItemManager::removeItem( item ); scope->deleteLater(); } );
    mItem->setSelected( false );
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
  if ( e->button() == Qt::RightButton )
  {
    if ( mEditContext.isValid() )
    {
      // Context menu
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}

void KadasMapToolEditItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  QgsCoordinateTransform crst( canvas()->mapSettings().destinationCrs(), mItem->crs(), QgsProject::instance() );
  QgsPointXY pos = crst.transform( e->mapPoint() );

  if ( e->buttons() == Qt::LeftButton )
  {
    if ( mEditContext.isValid() )
    {
      mItem->edit( mEditContext, pos - mMoveOffset, &canvas()->mapSettings() );
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
      mMoveOffset = pos - mEditContext.pos;
    }
  }
  if ( mInputWidget && mEditContext.isValid() )
  {
    mInputWidget->ensureFocus();
    KadasMapItem::AttribValues values = mItem->editAttribsFromPosition( mEditContext, pos - mMoveOffset );
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
}

void KadasMapToolEditItem::setupNumericInput()
{
  clearNumericInput();
  if ( QSettings().value( "/kadas/showNumericInput", false ).toBool() && mEditContext.isValid() && !mEditContext.attributes.isEmpty() )
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

    QgsPointXY newPos = mItem->positionFromEditAttribs( mEditContext, values, mCanvas->mapSettings() );
    QgsCoordinateTransform crst( mItem->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
    mInputWidget->adjustCursorAndExtent( crst.transform( newPos ) );

    mItem->edit( mEditContext, values );
    mStateHistory->push( mItem->constState()->clone() );
  }
}
