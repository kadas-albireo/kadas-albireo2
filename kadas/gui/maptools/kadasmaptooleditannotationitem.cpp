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


/**
 * \brief Lightweight canvas overlay that paints the controller-supplied
 *        edit handles (vertices) for the item being created/edited.
 *
 * Lives only while the tool is active; the tool calls update() after any
 * change to the item to keep the handles in sync.
 */
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

    void updateRect()
    {
      // Cover the visible map extent so handles outside the item bbox are
      // still painted (e.g. a rotation handle offset above the rect).
      setRect( mMapCanvas->mapSettings().visibleExtent() );
    }

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

    // Paints the on-canvas measurement labels (segment lengths, polygon area)
    // emitted by the controller. Restored from Kadas 2.3 KadasGeometryItem.
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
      constexpr int offsetBelow = 20; // pixels, when label is anchored to a vertex (e.g. line total)
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
    // Embed the integration-supplied widget (e.g. MilX symbol picker
    // button) into the editor row so it is visually part of the editor,
    // not a free-floating popup over the map.
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
  // Any layer repaint (i.e. any item mutation by this tool) refreshes handles.
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

  // Style the band from the item's actual symbol so the preview is drawn
  // with the right colors / stroke width. Re-applied on every update so
  // live style-editor changes are reflected immediately. Falls back to
  // QgsMapToolModifyAnnotation's generic gray band for items without a
  // line/fill symbol (e.g. the bounding-box preview of markers).
  const double minWidth = canvas()->fontMetrics().xHeight() * .2;
  const double mmToPx = canvas()->logicalDpiX() / 25.4;
  QColor strokeColor( 50, 50, 50, 200 );
  QColor fillColor( Qt::transparent );
  QColor secondaryColor( 255, 255, 255, 100 );
  double widthPx = minWidth;
  if ( const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( mItem ) )
  {
    if ( const QgsLineSymbol *sym = line->symbol() )
    {
      strokeColor = sym->color();
      widthPx = std::max( minWidth, sym->width() * mmToPx );
      secondaryColor = QColor();
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
      }
    }
  }
  mTempRubberBand->setStrokeColor( strokeColor );
  mTempRubberBand->setFillColor( fillColor );
  mTempRubberBand->setSecondaryStrokeColor( secondaryColor );
  mTempRubberBand->setWidth( widthPx );

  // setToGeometry() resets the band to the geometry's type and transforms
  // from the layer CRS to the map CRS.
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
  // Drop the uncommitted in-progress item if the user closed the tool mid-draw.
  if ( mAllowCreate && mDrawState != DrawState::Finished )
    clearInProgressItem();
  clearTempRubberBand();
  if ( mLayer )
    mLayer->triggerRepaint();
  delete mBottomBar;
  mBottomBar = nullptr;
  // Style editor was a child of mBottomBar; its pointer is now dangling.
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
    // Right-click during digitizing finalizes the part.
    if ( mAllowCreate && mDrawState == DrawState::InProgress )
    {
      finishPart();
      return;
    }
    // Otherwise, if the click hits an annotation, show a z-order menu;
    // a click on empty canvas closes the tool.
    const PickedItem hit = pickItemAt( e->mapPoint() );
    if ( !hit.isEmpty() )
      showContextMenu( hit.layer, hit.itemId, e->globalPosition().toPoint() );
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

  // Outside of a digitizing part, a click on a different item switches
  // the tool to editing that item, instead of starting a brand new one
  // (or doing nothing in pure edit mode). The picker considers items on
  // any visible annotation layer, not only the currently active one.
  if ( mDrawState != DrawState::InProgress )
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
        // Vertex committed: the item is disabled in the layer while
        // digitizing, so only the preview band needs updating.
        updateTempRubberBand();
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

  KadasAnnotationItemContext ctx( mLayer, canvas()->mapSettings() );
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
    // No layer repaint per mouse move: the preview band (drawn above all
    // layers) provides the live feedback; the layer is re-rendered only
    // when a vertex is committed.
    updateTempRubberBand();
    refreshHandles();
    return;
  }

  // In single-part create mode, after a part is finalized the next click
  // must place a new item — not drag the just-placed one. Suppress the
  // edit context so canvasPressEvent never enters the drag branch.
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
      // During the drag the layer keeps showing the pre-drag rendering;
      // the band shows the live outline above all layers (QGIS behavior).
      // The layer is re-rendered once on release.
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
    // Commit the drag: clear the preview band and render the edited item.
    if ( mTempRubberBand )
    {
      clearTempRubberBand();
      if ( mLayer )
        mLayer->triggerRepaint();
    }
    pushState();
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
  // History clones carry the enabled flag of when they were pushed; keep
  // it consistent with the restored draw state (disabled only while
  // digitizing, when the band is the preview).
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
    if ( mTempRubberBand )
      updateTempRubberBand();
  } );

  // Committed: apply, repaint, persist as defaults, and push a history entry.
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
    // While digitizing, the item must not be rendered by the layer: any
    // intermediate repaint (e.g. a style change) would leave a stale
    // "shadow" of the in-progress geometry behind the live band. The
    // band is the only preview until the part is finished.
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

  // Walk all visible annotation layers so a click can pick items from
  // any of them (Pictures / Routes / Pins / Symbols / Mss / Redlining
  // ...). QgsAnnotationLayer::itemsInBounds is a bounding-box test, so
  // we refine each candidate with the controller's getEditContext()
  // (vertex / handle / edge / actual body containment). Candidates are
  // ranked by KadasFeaturePicker::rankAnnotationCandidates: precise
  // hits (vertex / handle / stroke edge) outrank loose body / bbox
  // hits regardless of z, with z-then-area as tiebreaker inside each
  // precision tier. A controller that reports no hit means "genuinely
  // missed" — skip such candidates entirely (otherwise a right-click in
  // empty space inside an item's AABB would falsely show the context
  // menu). Only items WITHOUT a registered controller go into the
  // bbox-only fallback so identify-style flows still resolve something
  // for unknown item types.
  QList<KadasFeaturePicker::AnnotationPickCandidate> precise;
  QList<KadasFeaturePicker::AnnotationPickCandidate> fallback;

  const auto layers = canvas()->layers();
  for ( QgsMapLayer *ml : layers )
  {
    QgsAnnotationLayer *al = qobject_cast<QgsAnnotationLayer *>( ml );
    if ( !al )
      continue;
    // Parametric layers (bullseye, guide grid, ...) are edited atomically
    // via their dedicated config tool, never item by item.
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
        // No controller for this item type: bbox-only fallback.
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
  // Replace the active tool with a fresh edit-mode tool bound to the
  // clicked item (on its own layer, which may differ from the layer the
  // current tool was created with). The current tool is destroyed by
  // setMapTool().
  canvas()->setMapTool( new KadasMapToolEditAnnotationItem( canvas(), layer, itemId ) );
}

void KadasMapToolEditAnnotationItem::showContextMenu( QgsAnnotationLayer *layer, const QString &itemId, const QPoint &globalPos )
{
  if ( !layer )
    return;
  QgsAnnotationItem *target = layer->item( itemId );
  if ( !target )
    return;

  // Collect z-indices of all items on this layer to compute neighbor swaps
  // and front/back extremes.
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

  // Find next higher and next lower distinct z values.
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
  // If the right-clicked item is the one currently being edited, keep
  // history in sync.
  if ( layer == mLayer.data() && itemId == mItemId )
    pushState();
}
