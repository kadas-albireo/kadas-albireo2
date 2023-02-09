/***************************************************************************
    kadasmaptoolcreateitem.cpp
    --------------------------
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
#include <qgis/qgssettings.h>
#include <qgis/qgssnappingutils.h>


#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasgeometryitem.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>


KadasMapToolCreateItem::KadasMapToolCreateItem( QgsMapCanvas *canvas, ItemFactory itemFactory, KadasItemLayer *layer )
  : QgsMapTool( canvas )
  , mItemFactory( itemFactory )
  , mLayer( layer )
{
}

KadasMapToolCreateItem::KadasMapToolCreateItem( QgsMapCanvas *canvas, KadasMapItem *item, KadasItemLayer *layer )
  : QgsMapTool( canvas )
  , mItem( item )
  , mLayer( layer )
{

}

KadasMapToolCreateItem::~KadasMapToolCreateItem()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolCreateItem::activate()
{
  mStateHistory = new KadasStateHistory( this );
  mCurrentItemData = QSharedPointer<ItemData>( new ItemData );
  connect( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolCreateItem::stateChanged );
  createItem();
  setupNumericInputWidget();
  mSnapping = QgsSettings().value( "/kadas/snapping_enabled", false ).toBool();
  mBottomBar = new KadasBottomBar( canvas() );
  mBottomBar->setLayout( new QHBoxLayout() );
  mBottomBar->layout()->setContentsMargins( 8, 4, 8, 4 );
  if ( !mToolLabel.isEmpty() )
  {
    QLabel *label = new QLabel( mToolLabel );
    QFont font = label->font();
    font.setBold( true );
    label->setFont( font );
    mBottomBar->layout()->addWidget( label );
  }
  if ( mBottomBarExtra )
  {
    mBottomBar->layout()->addWidget( mBottomBarExtra );
  }
  if ( mShowLayerSelection )
  {
    KadasLayerSelectionWidget *layerSelection = new KadasLayerSelectionWidget( mCanvas, mLayerTreeView, mLayerSelectionFilter, mLayerCreator );
    layerSelection->setSelectedLayer( mLayer );
    connect( layerSelection, &KadasLayerSelectionWidget::selectedLayerChanged, this, &KadasMapToolCreateItem::setTargetLayer );
    mBottomBar->layout()->addWidget( layerSelection );
  }
  KadasMapItemEditor::Factory factory = KadasMapItemEditor::registry()->value( mItem->editor() );
  if ( factory )
  {
    mEditor = factory( mItem, KadasMapItemEditor::CreateItemEditor );
    mEditor->syncWidgetToItem();
    mBottomBar->layout()->addWidget( mEditor );
  }

  if ( mUndoRedoVisible )
  {
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
  }

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  mBottomBar->layout()->addWidget( closeButton );

  mBottomBar->show();
  QgsMapTool::activate();
}

void KadasMapToolCreateItem::deactivate()
{
  QgsMapTool::deactivate();
  delete mBottomBar;
  mBottomBar = nullptr;
  mEditor = nullptr;
  if ( mItem )
  {
    commitItem();
  }
  delete mStateHistory;
  mStateHistory = nullptr;
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolCreateItem::setExtraBottomBarContents( QWidget *widget )
{
  mBottomBarExtra = widget;
}

void KadasMapToolCreateItem::setItemFactory( ItemFactory itemFactory )
{
  mItemFactory = itemFactory;
  clear();
  setupNumericInputWidget();
}

void KadasMapToolCreateItem::showLayerSelection( bool enabled, QgsLayerTreeView *layerTreeView, KadasLayerSelectionWidget::LayerFilter filter, KadasLayerSelectionWidget::LayerCreator creator )
{
  mLayerTreeView = layerTreeView;
  mShowLayerSelection = enabled;
  mLayerSelectionFilter = filter;
  mLayerCreator = creator;
}

void KadasMapToolCreateItem::clear()
{
  commitItem();
  mCurrentItemData = QSharedPointer<ItemData>( new ItemData );
  createItem();
  if ( mEditor )
  {
    mEditor->setItem( mItem );
    mEditor->reset();
    mEditor->syncWidgetToItem();
  }
  emit cleared();
}

void KadasMapToolCreateItem::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    KadasMapPos pos = transformMousePoint( e->mapPoint() );
    addPoint( pos );
  }
  else if ( e->button() == Qt::RightButton )
  {
    if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
    {
      finishPart();
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }

}

void KadasMapToolCreateItem::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  KadasMapPos pos = transformMousePoint( e->mapPoint() );

  if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
  {
    mItem->setCurrentPoint( pos, canvas()->mapSettings() );
  }
  if ( mInputWidget )
  {
    mInputWidget->ensureFocus();
    KadasMapItem::AttribValues values = mItem->drawAttribsFromPosition( pos, canvas()->mapSettings() );
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

void KadasMapToolCreateItem::canvasReleaseEvent( QgsMapMouseEvent *e )
{
}

void KadasMapToolCreateItem::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
    {
      mItem->clear();
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
  else if ( ( e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier ) || e->key() == Qt::Key_Backspace )
  {
    mStateHistory->undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier )
  {
    mStateHistory->redo();
  }
}

KadasMapPos KadasMapToolCreateItem::transformMousePoint( QgsPointXY mapPos ) const
{
  if ( mSnapping )
  {
    QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap( mapPos );
    if ( m.isValid() )
    {
      mapPos = m.point();
    }
    else
    {
      double minDist = std::numeric_limits<double>::max();
      QgsPointXY minPos;
      double snapTol = QgsSettings().value( "/kadas/snapping_radius", 10 ).toInt();
      for ( QgsMapLayer *layer : mCanvas->layers() )
      {
        KadasItemLayer *itemLayer = qobject_cast<KadasItemLayer *>( layer );
        if ( !itemLayer )
        {
          continue;
        }
        QPair<QgsPointXY, double> result = itemLayer->snapToVertex( mapPos, mCanvas->mapSettings(), snapTol );
        if ( result.second < minDist )
        {
          minPos = result.first;
          minDist = result.second;
        }
      }
      if ( minDist < snapTol )
      {
        mapPos = minPos;
      }
    }
  }
  return KadasMapPos( mapPos.x(), mapPos.y() );
}

KadasMapItem *KadasMapToolCreateItem::takeItem()
{
  KadasMapItem *item = mItem;
  KadasMapCanvasItemManager::removeItem( item );
  mItem = nullptr;
  clear();
  return item;
}

void KadasMapToolCreateItem::addPoint( const KadasMapPos &pos )
{
  if ( mItem->constState()->drawStatus == KadasMapItem::State::Empty )
  {
    startPart( pos );
  }
  else if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
  {
    // Add point, stop drawing if item does not accept further points
    if ( !mItem->continuePart( canvas()->mapSettings() ) )
    {
      finishPart();
    }
    else
    {
      mStateHistory->push( new ToolState( mItem->constState()->clone(), mCurrentItemData ) );
    }
  }
  else if ( mItem->constState()->drawStatus == KadasMapItem::State::Finished )
  {
    if ( !mMultipart )
    {
      clear();
    }
    startPart( pos );
  }
}

void KadasMapToolCreateItem::createItem()
{
  if ( !mItem && mItemFactory )
  {
    mItem = mItemFactory();
  }
  else
  {
    KadasMapItem::State *state = mItem->constState()->clone();
    state->drawStatus = KadasMapItem::State::Drawing;
    mItem->setState( state );
    delete state;
  }
  mItem->setSelected( mSelectItems );
  mCurrentItemData->props = mItem->getProps();
  connect( mItem, &KadasMapItem::propertyChanged, this, &KadasMapToolCreateItem::storeItemProps );
  KadasMapCanvasItemManager::addItem( mItem );
}

void KadasMapToolCreateItem::startPart( const KadasMapPos &pos )
{
  if ( !mItem->startPart( pos, canvas()->mapSettings() ) )
  {
    finishPart();
  }
  else
  {
    mStateHistory->push( new ToolState( mItem->constState()->clone(), mCurrentItemData ) );
  }
}

void KadasMapToolCreateItem::startPart( const KadasMapItem::AttribValues &attributes )
{
  if ( !mItem->startPart( attributes, canvas()->mapSettings() ) )
  {
    finishPart();
  }
  else
  {
    mStateHistory->push( new ToolState( mItem->constState()->clone(), mCurrentItemData ) );
  }
}

void KadasMapToolCreateItem::finishPart()
{
  mItem->endPart();
  mStateHistory->push( new ToolState( mItem->constState()->clone(), mCurrentItemData ) );
  emit partFinished();
  if ( !mItemFactory )
  {
    KadasMapItem *item = mItem;
    KadasMapCanvasItemManager::removeItem( mItem ); // Edit tool adds item again
    mItem = nullptr;
    if ( mEditor )
    {
      mEditor->setItem( nullptr );
    }
    canvas()->setMapTool( new KadasMapToolEditItem( canvas(), item, mLayer ) );
  }
}

void KadasMapToolCreateItem::addPartFromGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs )
{
  if ( dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    if ( crs != mItem->crs() )
    {
      QgsAbstractGeometry *transformedGeom = geom.clone();
      transformedGeom->transform( QgsCoordinateTransform( crs, mItem->crs(), QgsProject::instance() ) );
      static_cast<KadasGeometryItem *>( mItem )->addPartFromGeometry( *transformedGeom );
      delete transformedGeom;
    }
    else
    {
      static_cast<KadasGeometryItem *>( mItem )->addPartFromGeometry( geom );
    }
    mStateHistory->push( new ToolState( mItem->constState()->clone(), mCurrentItemData ) );
    emit partFinished();
  }
}

void KadasMapToolCreateItem::commitItem()
{
  if ( !mItem )
  {
    return;
  }
  mItem->setSelected( false );
  disconnect( mItem, &KadasMapItem::propertyChanged, this, &KadasMapToolCreateItem::storeItemProps );
  if ( mEditor )
  {
    mEditor->setItem( nullptr );
  }
  if ( mLayer && mItem->constState()->drawStatus == KadasMapItem::State::Finished )
  {
    mCurrentItemData->itemId = mLayer->addItem( mItem );
    mLayer->triggerRepaint();
    KadasMapCanvasItemManager::removeItemAfterRefresh( mItem, mCanvas );
  }
  else
  {
    KadasMapCanvasItemManager::removeItem( mItem );
    delete mItem;
  }
  mItem = nullptr;
}

KadasMapItem::AttribValues KadasMapToolCreateItem::collectAttributeValues() const
{
  KadasMapItem::AttribValues attributes;
  for ( const KadasFloatingInputWidgetField *field : mInputWidget->inputFields() )
  {
    attributes.insert( field->id(), field->text().toDouble() );
  }
  return attributes;
}

void KadasMapToolCreateItem::inputChanged()
{
  KadasMapItem::AttribValues values = collectAttributeValues();

  // Ignore the move event emitted by re-positioning the mouse cursor:
  // The widget mouse coordinates (stored in a integer QPoint) loses precision,
  // and mapping it back to map coordinates in the mouseMove event handler
  // results in a position different from geoPos, and hence the user-input
  // may get altered
  mIgnoreNextMoveEvent = true;

  KadasMapPos newPos = mItem->positionFromDrawAttribs( values, mCanvas->mapSettings() );
  mInputWidget->adjustCursorAndExtent( newPos );

  if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
  {
    mItem->setCurrentAttributes( values, canvas()->mapSettings() );
  }
}

void KadasMapToolCreateItem::acceptInput()
{
  if ( mItem->constState()->drawStatus == KadasMapItem::State::Empty )
  {
    startPart( collectAttributeValues() );
  }
  else if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing )
  {
    if ( !mItem->continuePart( canvas()->mapSettings() ) )
    {
      finishPart();
    }
    else
    {
      mStateHistory->push( mItem->constState()->clone() );
    }
  }
  else if ( mItem->constState()->drawStatus == KadasMapItem::State::Finished )
  {
    if ( !mMultipart )
    {
      clear();
    }
    startPart( collectAttributeValues() );
  }
}

void KadasMapToolCreateItem::stateChanged( KadasStateHistory::ChangeType changeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState )
{
  ToolState *newToolState = static_cast<ToolState *>( state );
  ToolState *prevToolState = static_cast<ToolState *>( prevState );
  if ( newToolState->itemData->itemId != prevToolState->itemData->itemId )
  {
    if ( changeType == KadasStateHistory::ChangeType::Undo )
    {
      if ( mEditor )
      {
        mEditor->setItem( nullptr );
      }
      KadasMapCanvasItemManager::removeItem( mItem );
      delete mItem;
      mItem = nullptr;
      clear();
      delete mLayer->takeItem( newToolState->itemData->itemId );
      mLayer->triggerRepaint( true );
    }
    else if ( changeType == KadasStateHistory::ChangeType::Redo )
    {
      clear();
    }
    mItem->setProps( newToolState->itemData->props );
    mEditor->syncItemToWidget();
    mCurrentItemData = newToolState->itemData;
  }
  mItem->setState( static_cast<const KadasMapItem::State *>( newToolState->itemState ) );
}

void KadasMapToolCreateItem::setTargetLayer( QgsMapLayer *layer )
{
  mLayer = dynamic_cast<KadasItemLayer *>( layer );
  emit targetLayerChanged( layer );
}

void KadasMapToolCreateItem::storeItemProps()
{
  mCurrentItemData->props = mItem->getProps();
}

void KadasMapToolCreateItem::setupNumericInputWidget()
{
  delete mInputWidget;
  mInputWidget = nullptr;
  if ( QgsSettings().value( "/kadas/showNumericInput", false ).toBool() )
  {
    mInputWidget = new KadasFloatingInputWidget( canvas() );

    mDrawAttribs = mItem->drawAttribs();
    for ( auto it = mDrawAttribs.begin(), itEnd = mDrawAttribs.end(); it != itEnd; ++it )
    {
      const KadasMapItem::NumericAttribute &attribute = it.value();
      KadasFloatingInputWidgetField *attrEdit = new KadasFloatingInputWidgetField( it.key(), attribute.precision( mCanvas->mapSettings() ), attribute.min, attribute.max );
      connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolCreateItem::inputChanged );
      connect( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolCreateItem::acceptInput );
      mInputWidget->addInputField( attribute.name + ":", attrEdit, attribute.suffix( mCanvas->mapSettings() ) );
    }
    if ( !mDrawAttribs.isEmpty() )
    {
      mInputWidget->setFocusedInputField( mInputWidget->inputField( mDrawAttribs.begin().key() ) );
    }
  }
}
