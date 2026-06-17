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
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <functional>
#include <limits>
#include <memory>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapcanvasitem.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/kadasfloatinginputwidget.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"


class KadasMapToolEditAnnotationItem::HandlesOverlay : public QgsMapCanvasItem
{
  public:
    using NodesProvider = std::function<QList<KadasNode>()>;
    using LabelsProvider = std::function<QList<KadasAnnotationMeasurementLabel>()>;

    HandlesOverlay( QgsMapCanvas *canvas, NodesProvider nodesProvider, LabelsProvider labelsProvider )
      : QgsMapCanvasItem( canvas )
      , mNodesProvider( std::move( nodesProvider ) )
      , mLabelsProvider( std::move( labelsProvider ) )
    {
      setZValue( std::numeric_limits<double>::max() );
      updateRect();
    }

    void updateRect() { setRect( mMapCanvas->mapSettings().visibleExtent() ); }

    void paint( QPainter *painter ) override
    {
      paintMeasurementLabels( painter );
      paintHandles( painter );
    }

  private:
    void paintHandles( QPainter *painter )
    {
      if ( !mNodesProvider )
        return;
      const QList<KadasNode> nodes = mNodesProvider();
      painter->save();
      painter->setRenderHint( QPainter::Antialiasing, false );
      painter->setBrush( QBrush( Qt::white ) );
      painter->setPen( QPen( Qt::black, 1 ) );
      constexpr int sz = 8;
      for ( const KadasNode &n : nodes )
      {
        const QPointF screen = toCanvasCoordinates( n.pos );
        if ( n.render )
        {
          painter->save();
          n.render( painter, screen, sz );
          painter->restore();
        }
        else
        {
          painter->drawRect( QRectF( screen.x() - 0.5 * sz, screen.y() - 0.5 * sz, sz, sz ) );
        }
      }
      painter->restore();
    }

    void paintMeasurementLabels( QPainter *painter )
    {
      if ( !mLabelsProvider )
        return;
      const QList<KadasAnnotationMeasurementLabel> labels = mLabelsProvider();
      if ( labels.isEmpty() )
        return;

      const QgsSettings settings;
      const int red = settings.value( QStringLiteral( "/Qgis/default_measure_color_red" ), 255 ).toInt();
      const int green = settings.value( QStringLiteral( "/Qgis/default_measure_color_green" ), 0 ).toInt();
      const int blue = settings.value( QStringLiteral( "/Qgis/default_measure_color_blue" ), 0 ).toInt();
      const QColor textColor( red, green, blue );
      const QColor backgroundColor( 255, 255, 255, 192 );
      constexpr int offsetBelow = 20; // px
      constexpr int padding = 3;

      QFont font = painter->font();
      font.setPointSizeF( 9.0 );
      font.setBold( true );

      painter->save();
      painter->setFont( font );
      const QFontMetrics metrics( font );

      for ( const KadasAnnotationMeasurementLabel &label : labels )
      {
        const QStringList lines = label.text.split( '\n' );
        int width = 0;
        for ( const QString &l : lines )
          width = std::max( width, metrics.horizontalAdvance( l ) );
        const int height = metrics.height() * lines.size();
        const QPointF screen = toCanvasCoordinates( label.mapPos );
        QRectF rect( screen.x() - 0.5 * ( width + 2 * padding ), screen.y() + ( label.centered ? 0 : offsetBelow ) - 0.5 * ( height + 2 * padding ), width + 2 * padding, height + 2 * padding );
        painter->fillRect( rect, backgroundColor );
        painter->setPen( textColor );
        painter->drawText( rect, Qt::AlignCenter, label.text );
      }
      painter->restore();
    }

    NodesProvider mNodesProvider;
    LabelsProvider mLabelsProvider;
};


struct KadasMapToolEditAnnotationItem::ToolState : KadasStateHistory::State
{
    ToolState( QgsAnnotationItem *clone, DrawState ds )
      : itemClone( clone )
      , drawState( ds )
    {}
    std::unique_ptr<QgsAnnotationItem> itemClone;
    DrawState drawState = DrawState::Finished;
};


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

  if ( mExtraTopWidget )
  {
    topRow->insertWidget( 1, mExtraTopWidget );
  }

  setupStyleEditor( outer );

  mHandles = new HandlesOverlay(
    canvas(),
    [this]() -> QList<KadasNode> {
      if ( !mItem || !mController || !mLayer )
        return {};
      KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
      return mController->nodes( mItem, ctx );
    },
    [this]() -> QList<KadasAnnotationMeasurementLabel> {
      if ( !mItem || !mController || !mLayer )
        return {};
      KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
      return mController->measurementLabels( mItem, ctx );
    }
  );
  connect( canvas(), &QgsMapCanvas::extentsChanged, this, [this] {
    if ( mHandles )
    {
      mHandles->updateRect();
      mHandles->update();
    }
  } );
  connect( mLayer.data(), &QgsMapLayer::repaintRequested, this, &KadasMapToolEditAnnotationItem::refreshHandles );

  mBottomBar->adjustSize();
  mBottomBar->show();
}

void KadasMapToolEditAnnotationItem::refreshHandles()
{
  if ( mHandles )
    mHandles->update();
}

void KadasMapToolEditAnnotationItem::updateTempRubberBand()
{
  if ( !mItem || !mController || !mLayer || !canvas() )
  {
    clearTempRubberBand();
    return;
  }
  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
  const QgsGeometry geom = mController->representativeGeometry( mItem, ctx );
  if ( geom.isEmpty() )
  {
    clearTempRubberBand();
    return;
  }
  if ( !mTempRubberBand )
  {
    mTempRubberBand = new QgsRubberBand( canvas(), geom.type() );
  }

  const double minWidth = canvas()->fontMetrics().xHeight() * .2;
  const double mmToPx = canvas()->logicalDpiX() / 25.4;
  QColor strokeColor( 50, 50, 50, 200 );
  QColor fillColor( Qt::transparent );
  QColor secondaryColor( 255, 255, 255, 100 );
  Qt::BrushStyle brushStyle = Qt::SolidPattern;
  Qt::PenStyle lineStyle = Qt::SolidLine;
  double widthPx = minWidth;
  if ( const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( mItem ) )
  {
    if ( const QgsLineSymbol *sym = line->symbol() )
    {
      strokeColor = sym->color();
      widthPx = std::max( minWidth, sym->width() * mmToPx );
      secondaryColor = QColor();
      if ( const auto *sl = dynamic_cast<const QgsSimpleLineSymbolLayer *>( sym->symbolLayer( 0 ) ) )
      {
        // Mirror the dash pattern so a dashed line does not preview as solid.
        lineStyle = sl->penStyle();
      }
    }
  }
  else if ( const auto *poly = dynamic_cast<const QgsAnnotationPolygonItem *>( mItem ) )
  {
    if ( const QgsFillSymbol *sym = poly->symbol() )
    {
      fillColor = sym->color();
      secondaryColor = QColor();
      if ( const auto *sl = dynamic_cast<const QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) ) )
      {
        strokeColor = sl->strokeColor();
        widthPx = std::max( minWidth, sl->strokeWidth() * mmToPx );
        // Mirror the fill pattern so a hatched polygon does not preview as a solid block.
        brushStyle = sl->brushStyle();
        // Mirror the dash pattern so a dashed outline does not preview as solid.
        lineStyle = sl->strokeStyle();
      }
    }
  }
  mTempRubberBand->setStrokeColor( strokeColor );
  mTempRubberBand->setFillColor( fillColor );
  mTempRubberBand->setBrushStyle( brushStyle );
  mTempRubberBand->setLineStyle( lineStyle );
  mTempRubberBand->setSecondaryStrokeColor( secondaryColor );
  mTempRubberBand->setWidth( widthPx );

  mTempRubberBand->setToGeometry( geom, mLayer->crs() );
}

void KadasMapToolEditAnnotationItem::clearTempRubberBand()
{
  delete mTempRubberBand;
  mTempRubberBand = nullptr;
}

void KadasMapToolEditAnnotationItem::deactivate()
{
  QgsMapTool::deactivate();
  if ( mEditItemHidden && mItem )
  {
    mEditItemHidden = false;
    mItem->setEnabled( true );
  }
  if ( mAllowCreate && mDrawState != DrawState::Finished )
    clearInProgressItem();
  clearTempRubberBand();
  if ( mLayer )
    mLayer->triggerRepaint();
  delete mBottomBar;
  mBottomBar = nullptr;
  mStyleEditor = nullptr;
  if ( mHandles )
  {
    canvas()->scene()->removeItem( mHandles );
    delete mHandles;
    mHandles = nullptr;
  }
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
    if ( mAllowCreate && mDrawState == DrawState::InProgress )
    {
      finishPart();
      return;
    }
    const PickedItem hit = pickItemAt( e->mapPoint() );
    if ( !hit.isEmpty() )
      showContextMenu( hit.layer, hit.itemId, e->globalPosition().toPoint() );
    else
      canvas()->unsetMapTool( this );
    return;
  }

  if ( e->button() != Qt::LeftButton )
    return;

  if ( mEditContext.isValid() )
    return;

  if ( !mAllowCreate && mDrawState != DrawState::InProgress )
  {
    const PickedItem hit = pickItemAt( e->mapPoint() );
    if ( !hit.isEmpty() && !( hit.layer == mLayer.data() && hit.itemId == mItemId ) )
    {
      switchToItem( hit.layer, hit.itemId );
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

  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );

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
        updateTempRubberBand();
        pushState();
      }
      break;

    case DrawState::Finished:
      if ( !mMultipart )
      {
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

  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
  const QgsPointXY pos = e->mapPoint();

  if ( mAllowCreate && mDrawState == DrawState::InProgress )
  {
    if ( mEditContext.isValid() )
    {
      mEditContext = KadasEditContext();
      setCursor( Qt::ArrowCursor );
      clearNumericInput();
    }
    mController->setCurrentPoint( mItem, pos, ctx );
    updateTempRubberBand();
    refreshHandles();
    return;
  }

  if ( mAllowCreate && !mMultipart && mDrawState == DrawState::Finished )
  {
    if ( mEditContext.isValid() )
    {
      mEditContext = KadasEditContext();
      setCursor( Qt::ArrowCursor );
      clearNumericInput();
    }
    return;
  }

  if ( e->buttons() == Qt::LeftButton )
  {
    if ( mEditContext.isValid() )
    {
      const QgsPointXY adjusted( pos.x() - mMoveOffset.x(), pos.y() - mMoveOffset.y() );
      mController->edit( mItem, mEditContext, adjusted, ctx );
      if ( mController->liveRepaintOnEdit() )
      {
        mLayer->triggerRepaint();
      }
      else if ( !mEditItemHidden )
      {
        // Hide the original so the rubber band is the only preview (no ghost/duplicate).
        mEditItemHidden = true;
        mItem->setEnabled( false );
        mLayer->triggerRepaint();
      }
      updateTempRubberBand();
      refreshHandles();
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
  {
    if ( mEditItemHidden )
    {
      mEditItemHidden = false;
      mItem->setEnabled( true );
    }
    clearTempRubberBand();
    if ( mLayer )
      mLayer->triggerRepaint();
    pushState();
    if ( mStyleEditor )
      mStyleEditor->loadFromItem( mItem );
  }
}

void KadasMapToolEditAnnotationItem::canvasDoubleClickEvent( QgsMapMouseEvent * )
{
  if ( !mItem || !mController || !mLayer )
    return;
  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
  mController->onDoubleClick( mItem, ctx );
  mLayer->triggerRepaint();
}

void KadasMapToolEditAnnotationItem::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mAllowCreate && mDrawState == DrawState::InProgress )
    {
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
  clearTempRubberBand();
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
  if ( mItem )
    mItem->setEnabled( mDrawState != DrawState::InProgress );
  mLayer->triggerRepaint();
  if ( mDrawState == DrawState::InProgress )
    updateTempRubberBand();
  else
    clearTempRubberBand();
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

  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
  const KadasAttribValues values = collectAttributeValues();

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

  connect( mStyleEditor, &KadasAnnotationStyleEditor::previewChanged, this, [this] {
    if ( !mItem || !mLayer )
      return;
    mStyleEditor->applyToItem( mItem );
    mLayer->triggerRepaint();
    if ( mTempRubberBand )
      updateTempRubberBand();
  } );

  connect( mStyleEditor, &KadasAnnotationStyleEditor::committed, this, [this] {
    if ( !mItem || !mLayer )
      return;
    mStyleEditor->applyToItem( mItem );
    mLayer->triggerRepaint();
    if ( mTempRubberBand )
      updateTempRubberBand();
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
  clearTempRubberBand();
  mItem = nullptr;
  mItemId.clear();
  mDrawState = DrawState::Empty;
}

void KadasMapToolEditAnnotationItem::startPart( const QgsPointXY &pos )
{
  if ( !mItem || !mController || !mLayer )
    return;
  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
  if ( !mController->startPart( mItem, pos, ctx ) )
  {
    finishPart();
  }
  else
  {
    mDrawState = DrawState::InProgress;
    mItem->setEnabled( false );
    mLayer->triggerRepaint();
    updateTempRubberBand();
    pushState();
  }
}

void KadasMapToolEditAnnotationItem::finishPart()
{
  if ( !mItem || !mController || !mLayer )
    return;
  mController->endPart( mItem );
  mDrawState = DrawState::Finished;
  mItem->setEnabled( true );
  clearTempRubberBand();
  mLayer->triggerRepaint();
  pushState();
  emit partFinished();
}

KadasMapToolEditAnnotationItem::PickedItem KadasMapToolEditAnnotationItem::pickItemAt( const QgsPointXY &mapPos ) const
{
  PickedItem result;
  if ( !canvas() )
    return result;
  QgsRenderContext rc = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
  double radiusmm = QgsSettings().value( QStringLiteral( "/Map/searchRadiusMM" ), Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  if ( radiusmm <= 0 )
    radiusmm = Qgis::DEFAULT_SEARCH_RADIUS_MM;
  const double radiusmu = radiusmm * rc.scaleFactor() * rc.mapToPixel().mapUnitsPerPixel();
  const QgsRectangle mapBounds( mapPos.x() - radiusmu, mapPos.y() - radiusmu, mapPos.x() + radiusmu, mapPos.y() + radiusmu );

  QList<KadasFeaturePicker::AnnotationPickCandidate> precise;
  QList<KadasFeaturePicker::AnnotationPickCandidate> fallback;

  const auto layers = canvas()->layers();
  for ( QgsMapLayer *ml : layers )
  {
    QgsAnnotationLayer *al = qobject_cast<QgsAnnotationLayer *>( ml );
    if ( !al )
      continue;
    if ( KadasAnnotationLayerHelpers::isParametricLayer( al ) )
      continue;
    const QgsRectangle layerBounds = canvas()->mapSettings().mapToLayerCoordinates( al, mapBounds );
    QgsFeedback feedback;
    const QStringList hits = al->itemsInBounds( layerBounds, rc, &feedback );
    if ( hits.isEmpty() )
      continue;
    KadasAnnotationItemContext ctx( al, canvas()->mapSettings() );
    for ( const QString &id : hits )
    {
      QgsAnnotationItem *cand = al->item( id );
      if ( !cand )
        continue;
      const QgsRectangle bb = cand->boundingBox();
      KadasFeaturePicker::AnnotationPickCandidate c;
      c.layer = al;
      c.itemId = id;
      c.zIndex = cand->zIndex();
      c.bboxArea = bb.width() * bb.height();

      KadasAnnotationItemController *cc = KadasAnnotationControllerRegistry::instance()->controllerFor( cand->type() );
      if ( cc )
      {
        const KadasEditContext ec = cc->getEditContext( cand, mapPos, ctx );
        if ( !ec.isValid() )
          continue;
        c.precision = ec.precision;
        precise.append( c );
      }
      else
      {
        c.precision = KadasEditContext::HitPrecision::Body;
        fallback.append( c );
      }
    }
  }

  const QList<KadasFeaturePicker::AnnotationPickCandidate> &list = !precise.isEmpty() ? precise : fallback;
  const int bestIdx = KadasFeaturePicker::rankAnnotationCandidates( list );
  if ( bestIdx >= 0 )
  {
    result.layer = list[bestIdx].layer;
    result.itemId = list[bestIdx].itemId;
  }
  return result;
}

void KadasMapToolEditAnnotationItem::switchToItem( QgsAnnotationLayer *layer, const QString &itemId )
{
  if ( !canvas() || !layer || itemId.isEmpty() )
    return;
  canvas()->setMapTool( new KadasMapToolEditAnnotationItem( canvas(), layer, itemId ) );
}

void KadasMapToolEditAnnotationItem::showContextMenu( QgsAnnotationLayer *layer, const QString &itemId, const QPoint &globalPos )
{
  if ( !layer )
    return;
  QgsAnnotationItem *target = layer->item( itemId );
  if ( !target )
    return;

  const QMap<QString, QgsAnnotationItem *> all = layer->items();
  QList<int> zs;
  zs.reserve( all.size() );
  for ( auto it = all.constBegin(), itEnd = all.constEnd(); it != itEnd; ++it )
    zs.append( it.value()->zIndex() );
  if ( zs.isEmpty() )
    return;
  std::sort( zs.begin(), zs.end() );
  const int curZ = target->zIndex();
  const int minZ = zs.first();
  const int maxZ = zs.last();

  int nextHigher = curZ;
  bool hasHigher = false;
  for ( int z : zs )
  {
    if ( z > curZ )
    {
      nextHigher = z;
      hasHigher = true;
      break;
    }
  }
  int nextLower = curZ;
  bool hasLower = false;
  for ( int i = zs.size() - 1; i >= 0; --i )
  {
    if ( zs[i] < curZ )
    {
      nextLower = zs[i];
      hasLower = true;
      break;
    }
  }

  QMenu menu;
  QAction *toFront = menu.addAction( tr( "Bring to Front" ) );
  QAction *forward = menu.addAction( tr( "Bring Forward" ) );
  QAction *backward = menu.addAction( tr( "Send Backward" ) );
  QAction *toBack = menu.addAction( tr( "Send to Back" ) );
  toFront->setEnabled( curZ < maxZ );
  forward->setEnabled( hasHigher );
  backward->setEnabled( hasLower );
  toBack->setEnabled( curZ > minZ );

  QAction *chosen = menu.exec( globalPos );
  if ( !chosen )
    return;

  int newZ = curZ;
  if ( chosen == toFront )
    newZ = maxZ + 1;
  else if ( chosen == toBack )
    newZ = minZ - 1;
  else if ( chosen == forward )
    newZ = nextHigher + 1;
  else if ( chosen == backward )
    newZ = nextLower - 1;
  if ( newZ == curZ )
    return;

  target->setZIndex( newZ );
  layer->triggerRepaint();
  if ( layer == mLayer.data() && itemId == mItemId )
    pushState();
}
