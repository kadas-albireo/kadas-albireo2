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
#include <qgis/qgssettings.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfloatinginputwidget.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"


// ----- Persisted last-used styles, reapplied on freshly created items. ---

const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsMarkerShape
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-shape" ), sTreeAnnotation, static_cast<int>( Qgis::MarkerShape::Circle ), QStringLiteral( "Last-used marker shape." ) );
const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsMarkerSize
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-size" ), sTreeAnnotation, 3, QStringLiteral( "Last-used marker size (mm)." ) );
const QgsSettingsEntryDouble *KadasMapToolEditAnnotationItem::settingsMarkerStrokeWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "marker-stroke-width" ), sTreeAnnotation, 0.0, QStringLiteral( "Last-used marker outline width (mm)." ) );
const QgsSettingsEntryColor *KadasMapToolEditAnnotationItem::settingsMarkerFillColor
  = new QgsSettingsEntryColor( QStringLiteral( "marker-fill-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used marker fill color." ) );
const QgsSettingsEntryColor *KadasMapToolEditAnnotationItem::settingsMarkerStrokeColor
  = new QgsSettingsEntryColor( QStringLiteral( "marker-stroke-color" ), sTreeAnnotation, QColor( 0, 0, 0 ), QStringLiteral( "Last-used marker outline color." ) );
const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsMarkerStrokeStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-stroke-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidLine ), QStringLiteral( "Last-used marker outline style." ) );

const QgsSettingsEntryDouble *KadasMapToolEditAnnotationItem::settingsLineWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "line-width" ), sTreeAnnotation, 0.5, QStringLiteral( "Last-used line width (mm)." ) );
const QgsSettingsEntryColor *KadasMapToolEditAnnotationItem::settingsLineColor
  = new QgsSettingsEntryColor( QStringLiteral( "line-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used line color." ) );
const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsLineStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "line-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidLine ), QStringLiteral( "Last-used line style." ) );

const QgsSettingsEntryDouble *KadasMapToolEditAnnotationItem::settingsPolygonStrokeWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "polygon-stroke-width" ), sTreeAnnotation, 0.5, QStringLiteral( "Last-used polygon outline width (mm)." ) );
const QgsSettingsEntryColor *KadasMapToolEditAnnotationItem::settingsPolygonFillColor
  = new QgsSettingsEntryColor( QStringLiteral( "polygon-fill-color" ), sTreeAnnotation, QColor( 255, 0, 0, 80 ), QStringLiteral( "Last-used polygon fill color." ) );
const QgsSettingsEntryColor *KadasMapToolEditAnnotationItem::settingsPolygonStrokeColor
  = new QgsSettingsEntryColor( QStringLiteral( "polygon-stroke-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used polygon outline color." ) );
const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsPolygonStrokeStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "polygon-stroke-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidLine ), QStringLiteral( "Last-used polygon outline style." ) );
const QgsSettingsEntryInteger *KadasMapToolEditAnnotationItem::settingsPolygonBrushStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "polygon-brush-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidPattern ), QStringLiteral( "Last-used polygon fill style." ) );


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
  if ( !mStrokeColorBtn || !mItem || !mLayer )
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

  persistStyleToSettings();
}

void KadasMapToolEditAnnotationItem::persistStyleToSettings()
{
  if ( !mStrokeColorBtn || !mItem )
    return;
  // Only the three stock geometry types feed the persisted defaults so that
  // specialized items (rectangle / circle / coord-cross / pin / gpx-route
  // / picture / pointtext) keep their own controller defaults.
  const QString t = mItem->type();
  if ( t == QStringLiteral( "marker" ) )
  {
    settingsMarkerShape->setValue( mShapeCombo->currentData().toInt() );
    settingsMarkerSize->setValue( mSizeSpin->value() );
    settingsMarkerStrokeWidth->setValue( mStrokeWidthSpin->value() );
    settingsMarkerFillColor->setValue( mFillColorBtn->color() );
    settingsMarkerStrokeColor->setValue( mStrokeColorBtn->color() );
    settingsMarkerStrokeStyle->setValue( mStrokeStyleCombo->currentData().toInt() );
  }
  else if ( t == QStringLiteral( "linestring" ) )
  {
    settingsLineWidth->setValue( mStrokeWidthSpin->value() );
    settingsLineColor->setValue( mStrokeColorBtn->color() );
    settingsLineStyle->setValue( mStrokeStyleCombo->currentData().toInt() );
  }
  else if ( t == QStringLiteral( "polygon" ) )
  {
    settingsPolygonStrokeWidth->setValue( mStrokeWidthSpin->value() );
    settingsPolygonFillColor->setValue( mFillColorBtn->color() );
    settingsPolygonStrokeColor->setValue( mStrokeColorBtn->color() );
    settingsPolygonStrokeStyle->setValue( mStrokeStyleCombo->currentData().toInt() );
    settingsPolygonBrushStyle->setValue( mFillStyleCombo->currentData().toInt() );
  }
}

void KadasMapToolEditAnnotationItem::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  if ( !item )
    return;
  // Only the three stock geometry types pick up the persisted defaults so
  // that specialized items (rectangle / circle / coord-cross / pin / gpx
  // / picture / pointtext) keep their controller-supplied symbol unchanged.
  const QString t = item->type();
  if ( t == QStringLiteral( "marker" ) )
  {
    auto *marker = static_cast<QgsAnnotationMarkerItem *>( item );
    if ( !settingsMarkerStrokeColor->exists() )
      return; // never customized — keep controller defaults.
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
    sl->setShape( static_cast<Qgis::MarkerShape>( settingsMarkerShape->value() ) );
    sl->setSize( settingsMarkerSize->value() );
    sl->setStrokeWidth( settingsMarkerStrokeWidth->value() );
    sl->setColor( settingsMarkerFillColor->value() );
    sl->setStrokeColor( settingsMarkerStrokeColor->value() );
    sl->setStrokeStyle( static_cast<Qt::PenStyle>( settingsMarkerStrokeStyle->value() ) );
    marker->setSymbol( sym.release() );
  }
  else if ( t == QStringLiteral( "linestring" ) )
  {
    auto *line = static_cast<QgsAnnotationLineItem *>( item );
    if ( !settingsLineColor->exists() )
      return;
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
    sl->setWidth( settingsLineWidth->value() );
    sl->setColor( settingsLineColor->value() );
    sl->setPenStyle( static_cast<Qt::PenStyle>( settingsLineStyle->value() ) );
    line->setSymbol( sym.release() );
  }
  else if ( t == QStringLiteral( "polygon" ) )
  {
    auto *poly = static_cast<QgsAnnotationPolygonItem *>( item );
    if ( !settingsPolygonStrokeColor->exists() )
      return;
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
    sl->setStrokeWidth( settingsPolygonStrokeWidth->value() );
    sl->setColor( settingsPolygonFillColor->value() );
    sl->setStrokeColor( settingsPolygonStrokeColor->value() );
    sl->setStrokeStyle( static_cast<Qt::PenStyle>( settingsPolygonStrokeStyle->value() ) );
    sl->setBrushStyle( static_cast<Qt::BrushStyle>( settingsPolygonBrushStyle->value() ) );
    poly->setSymbol( sym.release() );
  }
}

void KadasMapToolEditAnnotationItem::createInitialItem()
{
  if ( !mLayer || !mController )
    return;
  QgsAnnotationItem *fresh = mItemFactory ? mItemFactory() : mController->createItem();
  if ( !fresh )
    return;
  applyPersistedStyle( fresh );
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
