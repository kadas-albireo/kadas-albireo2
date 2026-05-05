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

#include <QComboBox>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgscolorbutton.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfloatinginputwidget.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"


namespace
{
  //! Standard marker shapes exposed in the styling row, in display order.
  static const QList<Qgis::MarkerShape> sShapeChoices = {
    Qgis::MarkerShape::Circle,
    Qgis::MarkerShape::Square,
    Qgis::MarkerShape::Triangle,
    Qgis::MarkerShape::Diamond,
    Qgis::MarkerShape::Pentagon,
    Qgis::MarkerShape::Star,
    Qgis::MarkerShape::Cross,
    Qgis::MarkerShape::Cross2,
  };

  QIcon outlineStyleIcon( Qt::PenStyle style )
  {
    QPixmap pix( 24, 16 );
    pix.fill( Qt::transparent );
    QPainter p( &pix );
    p.setRenderHint( QPainter::Antialiasing );
    QPen pen( Qt::black );
    pen.setStyle( style );
    pen.setWidth( 2 );
    p.setPen( pen );
    p.drawLine( 0, 8, 24, 8 );
    return pix;
  }

  QgsSimpleMarkerSymbolLayer *firstSimpleMarker( QgsAnnotationMarkerItem *item )
  {
    if ( !item )
      return nullptr;
    QgsMarkerSymbol *sym = const_cast<QgsMarkerSymbol *>( item->symbol() );
    if ( !sym || sym->symbolLayerCount() == 0 )
      return nullptr;
    return dynamic_cast<QgsSimpleMarkerSymbolLayer *>( sym->symbolLayer( 0 ) );
  }

  QgsSimpleLineSymbolLayer *firstSimpleLine( QgsAnnotationLineItem *item )
  {
    if ( !item )
      return nullptr;
    QgsLineSymbol *sym = const_cast<QgsLineSymbol *>( item->symbol() );
    if ( !sym || sym->symbolLayerCount() == 0 )
      return nullptr;
    return dynamic_cast<QgsSimpleLineSymbolLayer *>( sym->symbolLayer( 0 ) );
  }

  QgsSimpleFillSymbolLayer *firstSimpleFill( QgsAnnotationPolygonItem *item )
  {
    if ( !item )
      return nullptr;
    QgsFillSymbol *sym = const_cast<QgsFillSymbol *>( item->symbol() );
    if ( !sym || sym->symbolLayerCount() == 0 )
      return nullptr;
    return dynamic_cast<QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) );
  }
} // namespace


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

  setupStyleWidgets( outer );

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
  // Style widgets were children of mBottomBar; their pointers are now dangling.
  mShapeCombo = nullptr;
  mSizeSpin = nullptr;
  mStrokeWidthSpin = nullptr;
  mFillColorBtn = nullptr;
  mStrokeColorBtn = nullptr;
  mStrokeStyleCombo = nullptr;
  mFillStyleCombo = nullptr;
  mTextEdit = nullptr;
  mTextFontCombo = nullptr;
  mTextSizeSpin = nullptr;
  mTextColorBtn = nullptr;
  mTextBufferColorBtn = nullptr;
  mTextBufferWidthSpin = nullptr;
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
        readStyleToWidgets();
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
        readStyleToWidgets();
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
  readStyleToWidgets();
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

void KadasMapToolEditAnnotationItem::setupStyleWidgets( QBoxLayout *outer )
{
  const bool isMarker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem ) != nullptr;
  const bool isLine = dynamic_cast<QgsAnnotationLineItem *>( mItem ) != nullptr;
  const bool isPolygon = dynamic_cast<QgsAnnotationPolygonItem *>( mItem ) != nullptr;
  const bool isPointText = dynamic_cast<QgsAnnotationPointTextItem *>( mItem ) != nullptr;

  // Text input row (point-text only).
  if ( isPointText )
  {
    auto *textRow = new QHBoxLayout();
    outer->addLayout( textRow );
    textRow->addWidget( new QLabel( tr( "Text:" ) ) );
    mTextEdit = new QLineEdit();
    mTextEdit->setPlaceholderText( tr( "Enter text" ) );
    textRow->addWidget( mTextEdit, 1 );
    connect( mTextEdit, &QLineEdit::textChanged, this, [this]( const QString &t ) {
      auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( mItem );
      if ( !pt )
        return;
      pt->setText( t );
      if ( mLayer )
        mLayer->triggerRepaint();
    } );
    connect( mTextEdit, &QLineEdit::editingFinished, this, [this]() { pushState(); } );

    // Styling row: font, size, color, border (buffer) color + width.
    auto *styleRow = new QHBoxLayout();
    outer->addLayout( styleRow );

    mTextFontCombo = new QFontComboBox();
    mTextFontCombo->setToolTip( tr( "Font family" ) );
    styleRow->addWidget( mTextFontCombo, 1 );

    mTextSizeSpin = new QDoubleSpinBox();
    mTextSizeSpin->setRange( 1.0, 200.0 );
    mTextSizeSpin->setDecimals( 1 );
    mTextSizeSpin->setSingleStep( 0.5 );
    mTextSizeSpin->setSuffix( QStringLiteral( " pt" ) );
    mTextSizeSpin->setToolTip( tr( "Font size" ) );
    styleRow->addWidget( mTextSizeSpin );

    mTextColorBtn = new QgsColorButton();
    mTextColorBtn->setAllowOpacity( true );
    mTextColorBtn->setToolTip( tr( "Text color" ) );
    styleRow->addWidget( mTextColorBtn );

    styleRow->addWidget( new QLabel( tr( "Border:" ) ) );
    mTextBufferColorBtn = new QgsColorButton();
    mTextBufferColorBtn->setAllowOpacity( true );
    mTextBufferColorBtn->setShowNoColor( true );
    mTextBufferColorBtn->setToolTip( tr( "Text border color" ) );
    styleRow->addWidget( mTextBufferColorBtn );

    mTextBufferWidthSpin = new QDoubleSpinBox();
    mTextBufferWidthSpin->setRange( 0.0, 10.0 );
    mTextBufferWidthSpin->setDecimals( 1 );
    mTextBufferWidthSpin->setSingleStep( 0.1 );
    mTextBufferWidthSpin->setSuffix( QStringLiteral( " mm" ) );
    mTextBufferWidthSpin->setToolTip( tr( "Text border width" ) );
    styleRow->addWidget( mTextBufferWidthSpin );

    auto applyAndPush = [this]() {
      applyStyleFromWidgets();
      pushState();
    };
    connect( mTextFontCombo, &QFontComboBox::currentFontChanged, this, applyAndPush );
    connect( mTextSizeSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, applyAndPush );
    connect( mTextColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
    connect( mTextBufferColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
    connect( mTextBufferWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, applyAndPush );
    return;
  }

  if ( !isMarker && !isLine && !isPolygon )
    return;

  auto *row = new QHBoxLayout();
  outer->addLayout( row );

  // Shape (marker only)
  if ( isMarker )
  {
    mShapeCombo = new QComboBox();
    for ( Qgis::MarkerShape shape : sShapeChoices )
      mShapeCombo->addItem( QgsSimpleMarkerSymbolLayerBase::encodeShape( shape ), QVariant::fromValue( static_cast<int>( shape ) ) );
    mShapeCombo->setToolTip( tr( "Marker shape" ) );
    row->addWidget( mShapeCombo );

    mSizeSpin = new QSpinBox();
    mSizeSpin->setRange( 1, 100 );
    mSizeSpin->setSuffix( QStringLiteral( " mm" ) );
    mSizeSpin->setToolTip( tr( "Marker size" ) );
    row->addWidget( mSizeSpin );
  }

  // Stroke width (all)
  mStrokeWidthSpin = new QDoubleSpinBox();
  mStrokeWidthSpin->setRange( 0.0, 20.0 );
  mStrokeWidthSpin->setDecimals( 1 );
  mStrokeWidthSpin->setSingleStep( 0.1 );
  mStrokeWidthSpin->setToolTip( isMarker ? tr( "Outline width" ) : tr( "Line width" ) );
  row->addWidget( mStrokeWidthSpin );

  // Fill color (marker + polygon)
  if ( isMarker || isPolygon )
  {
    mFillColorBtn = new QgsColorButton();
    mFillColorBtn->setAllowOpacity( true );
    mFillColorBtn->setShowNoColor( true );
    mFillColorBtn->setToolTip( tr( "Fill color" ) );
    row->addWidget( mFillColorBtn );
  }

  // Stroke color (all)
  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( isMarker || isPolygon ? tr( "Outline color" ) : tr( "Line color" ) );
  row->addWidget( mStrokeColorBtn );

  // Stroke style (all)
  mStrokeStyleCombo = new QComboBox();
  for ( Qt::PenStyle style : { Qt::NoPen, Qt::SolidLine, Qt::DashLine, Qt::DashDotLine, Qt::DotLine } )
    mStrokeStyleCombo->addItem( outlineStyleIcon( style ), QString(), QVariant::fromValue( static_cast<int>( style ) ) );
  mStrokeStyleCombo->setToolTip( isMarker || isPolygon ? tr( "Outline style" ) : tr( "Line style" ) );
  row->addWidget( mStrokeStyleCombo );

  // Fill style (polygon only)
  if ( isPolygon )
  {
    mFillStyleCombo = new QComboBox();
    for ( Qt::BrushStyle style : { Qt::NoBrush, Qt::SolidPattern, Qt::HorPattern, Qt::VerPattern, Qt::BDiagPattern, Qt::FDiagPattern, Qt::CrossPattern, Qt::DiagCrossPattern } )
      mFillStyleCombo->addItem( QgsSymbolLayerUtils::encodeBrushStyle( style ), QVariant::fromValue( static_cast<int>( style ) ) );
    mFillStyleCombo->setToolTip( tr( "Fill style" ) );
    row->addWidget( mFillStyleCombo );
  }

  readStyleToWidgets();

  auto applyAndPush = [this] {
    applyStyleFromWidgets();
    if ( mLayer )
      mLayer->triggerRepaint();
    pushState();
  };
  if ( mShapeCombo )
    connect( mShapeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, applyAndPush );
  if ( mSizeSpin )
    connect( mSizeSpin, qOverload<int>( &QSpinBox::valueChanged ), this, applyAndPush );
  connect( mStrokeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, applyAndPush );
  if ( mFillColorBtn )
    connect( mFillColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
  connect( mStrokeStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, applyAndPush );
  if ( mFillStyleCombo )
    connect( mFillStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, applyAndPush );
}

void KadasMapToolEditAnnotationItem::readStyleToWidgets()
{
  if ( mTextEdit )
  {
    if ( auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( mItem ) )
    {
      const QSignalBlocker b( mTextEdit );
      mTextEdit->setText( pt->text() );
      mTextEdit->setFocus();
      mTextEdit->selectAll();
    }
  }

  if ( mTextFontCombo )
  {
    if ( auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( mItem ) )
    {
      const QgsTextFormat fmt = pt->format();
      const QSignalBlocker b1( mTextFontCombo ), b2( mTextSizeSpin ), b3( mTextColorBtn ), b4( mTextBufferColorBtn ), b5( mTextBufferWidthSpin );
      mTextFontCombo->setCurrentFont( fmt.font() );
      mTextSizeSpin->setValue( fmt.size() );
      mTextColorBtn->setColor( fmt.color() );
      const QgsTextBufferSettings buf = fmt.buffer();
      mTextBufferColorBtn->setColor( buf.enabled() ? buf.color() : QColor( 0, 0, 0, 0 ) );
      mTextBufferWidthSpin->setValue( buf.enabled() ? buf.size() : 0.0 );
    }
  }

  if ( !mStrokeColorBtn )
    return; // styling row not built

  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem ) )
  {
    QgsSimpleMarkerSymbolLayer *sl = firstSimpleMarker( marker );
    if ( !sl )
      return;
    const QSignalBlocker b1( mShapeCombo ), b2( mSizeSpin ), b3( mStrokeWidthSpin ), b4( mFillColorBtn ), b5( mStrokeColorBtn ), b6( mStrokeStyleCombo );
    const int shapeIdx = mShapeCombo->findData( static_cast<int>( sl->shape() ) );
    if ( shapeIdx >= 0 )
      mShapeCombo->setCurrentIndex( shapeIdx );
    mSizeSpin->setValue( static_cast<int>( std::round( sl->size() ) ) );
    mStrokeWidthSpin->setValue( sl->strokeWidth() );
    mFillColorBtn->setColor( sl->color() );
    mStrokeColorBtn->setColor( sl->strokeColor() );
    const int styleIdx = mStrokeStyleCombo->findData( static_cast<int>( sl->strokeStyle() ) );
    if ( styleIdx >= 0 )
      mStrokeStyleCombo->setCurrentIndex( styleIdx );
  }
  else if ( auto *line = dynamic_cast<QgsAnnotationLineItem *>( mItem ) )
  {
    QgsSimpleLineSymbolLayer *sl = firstSimpleLine( line );
    if ( !sl )
      return;
    const QSignalBlocker b1( mStrokeWidthSpin ), b2( mStrokeColorBtn ), b3( mStrokeStyleCombo );
    mStrokeWidthSpin->setValue( sl->width() );
    mStrokeColorBtn->setColor( sl->color() );
    const int styleIdx = mStrokeStyleCombo->findData( static_cast<int>( sl->penStyle() ) );
    if ( styleIdx >= 0 )
      mStrokeStyleCombo->setCurrentIndex( styleIdx );
  }
  else if ( auto *poly = dynamic_cast<QgsAnnotationPolygonItem *>( mItem ) )
  {
    QgsSimpleFillSymbolLayer *sl = firstSimpleFill( poly );
    if ( !sl )
      return;
    const QSignalBlocker b1( mStrokeWidthSpin ), b2( mFillColorBtn ), b3( mStrokeColorBtn ), b4( mStrokeStyleCombo ), b5( mFillStyleCombo );
    mStrokeWidthSpin->setValue( sl->strokeWidth() );
    mFillColorBtn->setColor( sl->color() );
    mStrokeColorBtn->setColor( sl->strokeColor() );
    const int styleIdx = mStrokeStyleCombo->findData( static_cast<int>( sl->strokeStyle() ) );
    if ( styleIdx >= 0 )
      mStrokeStyleCombo->setCurrentIndex( styleIdx );
    const int fillIdx = mFillStyleCombo->findData( static_cast<int>( sl->brushStyle() ) );
    if ( fillIdx >= 0 )
      mFillStyleCombo->setCurrentIndex( fillIdx );
  }
}

void KadasMapToolEditAnnotationItem::applyStyleFromWidgets()
{
  if ( !mItem || !mLayer )
    return;

  if ( mTextFontCombo )
  {
    if ( auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( mItem ) )
    {
      QgsTextFormat fmt = pt->format();
      QFont f = mTextFontCombo->currentFont();
      fmt.setFont( f );
      fmt.setSize( mTextSizeSpin->value() );
      fmt.setSizeUnit( Qgis::RenderUnit::Points );
      fmt.setColor( mTextColorBtn->color() );
      QgsTextBufferSettings buf = fmt.buffer();
      const double bw = mTextBufferWidthSpin->value();
      const QColor bc = mTextBufferColorBtn->color();
      buf.setEnabled( bw > 0.0 && bc.alpha() > 0 );
      buf.setColor( bc );
      buf.setSize( bw );
      buf.setSizeUnit( Qgis::RenderUnit::Millimeters );
      fmt.setBuffer( buf );
      pt->setFormat( fmt );
      mLayer->triggerRepaint();
      if ( mController )
        mController->persistStyle( mItem );
      return;
    }
  }

  if ( !mStrokeColorBtn )
    return;

  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem ) )
  {
    std::unique_ptr<QgsMarkerSymbol> sym( marker->symbol() ? marker->symbol()->clone() : new QgsMarkerSymbol() );
    if ( sym->symbolLayerCount() == 0 )
      sym->appendSymbolLayer( new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle ) );
    auto *sl = dynamic_cast<QgsSimpleMarkerSymbolLayer *>( sym->symbolLayer( 0 ) );
    if ( !sl )
    {
      auto *replacement = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle );
      sym->changeSymbolLayer( 0, replacement );
      sl = replacement;
    }
    sl->setShape( static_cast<Qgis::MarkerShape>( mShapeCombo->currentData().toInt() ) );
    sl->setSize( mSizeSpin->value() );
    sl->setStrokeWidth( mStrokeWidthSpin->value() );
    sl->setColor( mFillColorBtn->color() );
    sl->setStrokeColor( mStrokeColorBtn->color() );
    sl->setStrokeStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );
    marker->setSymbol( sym.release() );
  }
  else if ( auto *line = dynamic_cast<QgsAnnotationLineItem *>( mItem ) )
  {
    std::unique_ptr<QgsLineSymbol> sym( line->symbol() ? line->symbol()->clone() : new QgsLineSymbol() );
    if ( sym->symbolLayerCount() == 0 )
      sym->appendSymbolLayer( new QgsSimpleLineSymbolLayer() );
    auto *sl = dynamic_cast<QgsSimpleLineSymbolLayer *>( sym->symbolLayer( 0 ) );
    if ( !sl )
    {
      auto *replacement = new QgsSimpleLineSymbolLayer();
      sym->changeSymbolLayer( 0, replacement );
      sl = replacement;
    }
    sl->setWidth( mStrokeWidthSpin->value() );
    sl->setColor( mStrokeColorBtn->color() );
    sl->setPenStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );
    line->setSymbol( sym.release() );
  }
  else if ( auto *poly = dynamic_cast<QgsAnnotationPolygonItem *>( mItem ) )
  {
    std::unique_ptr<QgsFillSymbol> sym( poly->symbol() ? poly->symbol()->clone() : new QgsFillSymbol() );
    if ( sym->symbolLayerCount() == 0 )
      sym->appendSymbolLayer( new QgsSimpleFillSymbolLayer() );
    auto *sl = dynamic_cast<QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) );
    if ( !sl )
    {
      auto *replacement = new QgsSimpleFillSymbolLayer();
      sym->changeSymbolLayer( 0, replacement );
      sl = replacement;
    }
    sl->setStrokeWidth( mStrokeWidthSpin->value() );
    sl->setColor( mFillColorBtn->color() );
    sl->setStrokeColor( mStrokeColorBtn->color() );
    sl->setStrokeStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );
    sl->setBrushStyle( static_cast<Qt::BrushStyle>( mFillStyleCombo->currentData().toInt() ) );
    poly->setSymbol( sym.release() );
  }

  if ( mController )
    mController->persistStyle( mItem );
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
