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
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscolorbutton.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>

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
  auto *outer = new QVBoxLayout();
  outer->setContentsMargins( 8, 4, 8, 4 );
  mBottomBar->setLayout( outer );

  auto *topRow = new QHBoxLayout();
  outer->addLayout( topRow );

  QLabel *label = new QLabel( tr( "Edit %1" ).arg( mController->itemName() ) );
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

  if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem ) )
    setupMarkerStyleWidgets( marker, outer );

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
  // Style widgets were children of mBottomBar; their pointers are now dangling.
  mShapeCombo = nullptr;
  mSizeSpin = nullptr;
  mStrokeWidthSpin = nullptr;
  mFillColorBtn = nullptr;
  mStrokeColorBtn = nullptr;
  mStrokeStyleCombo = nullptr;
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
    canvas()->unsetMapTool( this );
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
  mEditContext = KadasEditContext();
  clearNumericInput();
  readMarkerStyleToWidgets();
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

void KadasMapToolEditAnnotationItem::setupMarkerStyleWidgets( QgsAnnotationMarkerItem *item, QBoxLayout *outer )
{
  if ( !item )
    return;

  auto *row = new QHBoxLayout();
  outer->addLayout( row );

  // Shape
  mShapeCombo = new QComboBox();
  for ( Qgis::MarkerShape shape : sShapeChoices )
    mShapeCombo->addItem( QgsSimpleMarkerSymbolLayerBase::encodeShape( shape ), QVariant::fromValue( static_cast<int>( shape ) ) );
  mShapeCombo->setToolTip( tr( "Marker shape" ) );
  row->addWidget( mShapeCombo );

  // Size
  mSizeSpin = new QSpinBox();
  mSizeSpin->setRange( 1, 100 );
  mSizeSpin->setSuffix( QStringLiteral( " mm" ) );
  mSizeSpin->setToolTip( tr( "Marker size" ) );
  row->addWidget( mSizeSpin );

  // Stroke width
  mStrokeWidthSpin = new QSpinBox();
  mStrokeWidthSpin->setRange( 0, 20 );
  mStrokeWidthSpin->setToolTip( tr( "Outline width" ) );
  row->addWidget( mStrokeWidthSpin );

  // Fill color
  mFillColorBtn = new QgsColorButton();
  mFillColorBtn->setAllowOpacity( true );
  mFillColorBtn->setShowNoColor( true );
  mFillColorBtn->setToolTip( tr( "Fill color" ) );
  row->addWidget( mFillColorBtn );

  // Stroke color
  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( tr( "Outline color" ) );
  row->addWidget( mStrokeColorBtn );

  // Stroke style
  mStrokeStyleCombo = new QComboBox();
  for ( Qt::PenStyle style : { Qt::NoPen, Qt::SolidLine, Qt::DashLine, Qt::DashDotLine, Qt::DotLine } )
    mStrokeStyleCombo->addItem( outlineStyleIcon( style ), QString(), QVariant::fromValue( static_cast<int>( style ) ) );
  mStrokeStyleCombo->setToolTip( tr( "Outline style" ) );
  row->addWidget( mStrokeStyleCombo );

  readMarkerStyleToWidgets();

  auto applyAndPush = [this] {
    applyMarkerStyleFromWidgets();
    if ( mLayer )
      mLayer->triggerRepaint();
    pushState();
  };
  connect( mShapeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, applyAndPush );
  connect( mSizeSpin, qOverload<int>( &QSpinBox::valueChanged ), this, applyAndPush );
  connect( mStrokeWidthSpin, qOverload<int>( &QSpinBox::valueChanged ), this, applyAndPush );
  connect( mFillColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, applyAndPush );
  connect( mStrokeStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, applyAndPush );
}

void KadasMapToolEditAnnotationItem::readMarkerStyleToWidgets()
{
  if ( !mShapeCombo )
    return; // styling row not built (item is not a marker)
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem );
  QgsSimpleMarkerSymbolLayer *sl = firstSimpleMarker( marker );
  if ( !sl )
    return;

  const QSignalBlocker b1( mShapeCombo );
  const QSignalBlocker b2( mSizeSpin );
  const QSignalBlocker b3( mStrokeWidthSpin );
  const QSignalBlocker b4( mFillColorBtn );
  const QSignalBlocker b5( mStrokeColorBtn );
  const QSignalBlocker b6( mStrokeStyleCombo );

  const int shapeIdx = mShapeCombo->findData( static_cast<int>( sl->shape() ) );
  if ( shapeIdx >= 0 )
    mShapeCombo->setCurrentIndex( shapeIdx );
  mSizeSpin->setValue( static_cast<int>( std::round( sl->size() ) ) );
  mStrokeWidthSpin->setValue( static_cast<int>( std::round( sl->strokeWidth() ) ) );
  mFillColorBtn->setColor( sl->color() );
  mStrokeColorBtn->setColor( sl->strokeColor() );
  const int styleIdx = mStrokeStyleCombo->findData( static_cast<int>( sl->strokeStyle() ) );
  if ( styleIdx >= 0 )
    mStrokeStyleCombo->setCurrentIndex( styleIdx );
}

void KadasMapToolEditAnnotationItem::applyMarkerStyleFromWidgets()
{
  if ( !mShapeCombo || !mItem || !mLayer )
    return;
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( mItem );
  if ( !marker )
    return;

  // Clone the existing symbol so we can replace its first simple-marker layer.
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

  const auto shape = static_cast<Qgis::MarkerShape>( mShapeCombo->currentData().toInt() );
  sl->setShape( shape );
  sl->setSize( mSizeSpin->value() );
  sl->setStrokeWidth( mStrokeWidthSpin->value() );
  sl->setColor( mFillColorBtn->color() );
  sl->setStrokeColor( mStrokeColorBtn->color() );
  sl->setStrokeStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );

  marker->setSymbol( sym.release() );
}
