/***************************************************************************
    kadasannotationstyleeditor.cpp
    ------------------------------
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
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSpinBox>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscolorbutton.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"

namespace
{
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

  static const QList<Qt::PenStyle> sPenStyleChoices = { Qt::NoPen, Qt::SolidLine, Qt::DashLine, Qt::DashDotLine, Qt::DotLine };

  static const QList<Qt::BrushStyle> sBrushStyleChoices = { Qt::NoBrush, Qt::SolidPattern, Qt::HorPattern, Qt::VerPattern, Qt::BDiagPattern, Qt::FDiagPattern, Qt::CrossPattern, Qt::DiagCrossPattern };

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

  void populatePenStyleCombo( QComboBox *combo )
  {
    for ( Qt::PenStyle style : sPenStyleChoices )
      combo->addItem( outlineStyleIcon( style ), QString(), QVariant::fromValue( static_cast<int>( style ) ) );
  }

  void selectByData( QComboBox *combo, int data )
  {
    const int idx = combo->findData( data );
    if ( idx >= 0 )
      combo->setCurrentIndex( idx );
  }
} // namespace


// ----- Marker -----------------------------------------------------------

KadasMarkerStyleEditor::KadasMarkerStyleEditor( QWidget *parent )
  : KadasAnnotationStyleEditor( parent )
{
  auto *row = new QHBoxLayout( this );
  row->setContentsMargins( 0, 0, 0, 0 );

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

  mStrokeWidthSpin = new QDoubleSpinBox();
  mStrokeWidthSpin->setRange( 0.0, 20.0 );
  mStrokeWidthSpin->setDecimals( 1 );
  mStrokeWidthSpin->setSingleStep( 0.1 );
  mStrokeWidthSpin->setToolTip( tr( "Outline width" ) );
  row->addWidget( mStrokeWidthSpin );

  mFillColorBtn = new QgsColorButton();
  mFillColorBtn->setAllowOpacity( true );
  mFillColorBtn->setShowNoColor( true );
  mFillColorBtn->setToolTip( tr( "Fill color" ) );
  row->addWidget( mFillColorBtn );

  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( tr( "Outline color" ) );
  row->addWidget( mStrokeColorBtn );

  mStrokeStyleCombo = new QComboBox();
  populatePenStyleCombo( mStrokeStyleCombo );
  mStrokeStyleCombo->setToolTip( tr( "Outline style" ) );
  row->addWidget( mStrokeStyleCombo );

  connect( mShapeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mSizeSpin, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mFillColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasAnnotationStyleEditor::committed );
}

void KadasMarkerStyleEditor::loadFromItem( const QgsAnnotationItem *item )
{
  const auto *marker = dynamic_cast<const QgsAnnotationMarkerItem *>( item );
  if ( !marker || !marker->symbol() || marker->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  const QSignalBlocker b1( mShapeCombo ), b2( mSizeSpin ), b3( mStrokeWidthSpin );
  const QSignalBlocker b4( mFillColorBtn ), b5( mStrokeColorBtn ), b6( mStrokeStyleCombo );
  selectByData( mShapeCombo, static_cast<int>( sl->shape() ) );
  mSizeSpin->setValue( static_cast<int>( std::round( sl->size() ) ) );
  mStrokeWidthSpin->setValue( sl->strokeWidth() );
  mFillColorBtn->setColor( sl->color() );
  mStrokeColorBtn->setColor( sl->strokeColor() );
  selectByData( mStrokeStyleCombo, static_cast<int>( sl->strokeStyle() ) );
}

void KadasMarkerStyleEditor::applyToItem( QgsAnnotationItem *item ) const
{
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item );
  if ( !marker )
    return;
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


// ----- Line -------------------------------------------------------------

KadasLineStyleEditor::KadasLineStyleEditor( QWidget *parent )
  : KadasAnnotationStyleEditor( parent )
{
  auto *row = new QHBoxLayout( this );
  row->setContentsMargins( 0, 0, 0, 0 );

  mStrokeWidthSpin = new QDoubleSpinBox();
  mStrokeWidthSpin->setRange( 0.0, 20.0 );
  mStrokeWidthSpin->setDecimals( 1 );
  mStrokeWidthSpin->setSingleStep( 0.1 );
  mStrokeWidthSpin->setToolTip( tr( "Line width" ) );
  row->addWidget( mStrokeWidthSpin );

  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( tr( "Line color" ) );
  row->addWidget( mStrokeColorBtn );

  mStrokeStyleCombo = new QComboBox();
  populatePenStyleCombo( mStrokeStyleCombo );
  mStrokeStyleCombo->setToolTip( tr( "Line style" ) );
  row->addWidget( mStrokeStyleCombo );

  connect( mStrokeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasAnnotationStyleEditor::committed );
}

void KadasLineStyleEditor::loadFromItem( const QgsAnnotationItem *item )
{
  const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( item );
  if ( !line || !line->symbol() || line->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSimpleLineSymbolLayer *>( line->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  const QSignalBlocker b1( mStrokeWidthSpin ), b2( mStrokeColorBtn ), b3( mStrokeStyleCombo );
  mStrokeWidthSpin->setValue( sl->width() );
  mStrokeColorBtn->setColor( sl->color() );
  selectByData( mStrokeStyleCombo, static_cast<int>( sl->penStyle() ) );
}

void KadasLineStyleEditor::applyToItem( QgsAnnotationItem *item ) const
{
  auto *line = dynamic_cast<QgsAnnotationLineItem *>( item );
  if ( !line )
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
  sl->setWidth( mStrokeWidthSpin->value() );
  sl->setColor( mStrokeColorBtn->color() );
  sl->setPenStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );
  line->setSymbol( sym.release() );
}


// ----- Polygon ----------------------------------------------------------

KadasPolygonStyleEditor::KadasPolygonStyleEditor( QWidget *parent )
  : KadasAnnotationStyleEditor( parent )
{
  auto *row = new QHBoxLayout( this );
  row->setContentsMargins( 0, 0, 0, 0 );

  mStrokeWidthSpin = new QDoubleSpinBox();
  mStrokeWidthSpin->setRange( 0.0, 20.0 );
  mStrokeWidthSpin->setDecimals( 1 );
  mStrokeWidthSpin->setSingleStep( 0.1 );
  mStrokeWidthSpin->setToolTip( tr( "Outline width" ) );
  row->addWidget( mStrokeWidthSpin );

  mFillColorBtn = new QgsColorButton();
  mFillColorBtn->setAllowOpacity( true );
  mFillColorBtn->setShowNoColor( true );
  mFillColorBtn->setToolTip( tr( "Fill color" ) );
  row->addWidget( mFillColorBtn );

  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( tr( "Outline color" ) );
  row->addWidget( mStrokeColorBtn );

  mStrokeStyleCombo = new QComboBox();
  populatePenStyleCombo( mStrokeStyleCombo );
  mStrokeStyleCombo->setToolTip( tr( "Outline style" ) );
  row->addWidget( mStrokeStyleCombo );

  mFillStyleCombo = new QComboBox();
  for ( Qt::BrushStyle style : sBrushStyleChoices )
    mFillStyleCombo->addItem( QgsSymbolLayerUtils::encodeBrushStyle( style ), QVariant::fromValue( static_cast<int>( style ) ) );
  mFillStyleCombo->setToolTip( tr( "Fill style" ) );
  row->addWidget( mFillStyleCombo );

  connect( mStrokeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mFillColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mFillStyleCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasAnnotationStyleEditor::committed );
}

void KadasPolygonStyleEditor::loadFromItem( const QgsAnnotationItem *item )
{
  const auto *poly = dynamic_cast<const QgsAnnotationPolygonItem *>( item );
  if ( !poly || !poly->symbol() || poly->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSimpleFillSymbolLayer *>( poly->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  const QSignalBlocker b1( mStrokeWidthSpin ), b2( mFillColorBtn ), b3( mStrokeColorBtn );
  const QSignalBlocker b4( mStrokeStyleCombo ), b5( mFillStyleCombo );
  mStrokeWidthSpin->setValue( sl->strokeWidth() );
  mFillColorBtn->setColor( sl->color() );
  mStrokeColorBtn->setColor( sl->strokeColor() );
  selectByData( mStrokeStyleCombo, static_cast<int>( sl->strokeStyle() ) );
  selectByData( mFillStyleCombo, static_cast<int>( sl->brushStyle() ) );
}

void KadasPolygonStyleEditor::applyToItem( QgsAnnotationItem *item ) const
{
  auto *poly = dynamic_cast<QgsAnnotationPolygonItem *>( item );
  if ( !poly )
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
  sl->setStrokeWidth( mStrokeWidthSpin->value() );
  sl->setColor( mFillColorBtn->color() );
  sl->setStrokeColor( mStrokeColorBtn->color() );
  sl->setStrokeStyle( static_cast<Qt::PenStyle>( mStrokeStyleCombo->currentData().toInt() ) );
  sl->setBrushStyle( static_cast<Qt::BrushStyle>( mFillStyleCombo->currentData().toInt() ) );
  poly->setSymbol( sym.release() );
}


// ----- Point text -------------------------------------------------------

KadasPointTextStyleEditor::KadasPointTextStyleEditor( QWidget *parent )
  : KadasAnnotationStyleEditor( parent )
{
  auto *outer = new QVBoxLayout( this );
  outer->setContentsMargins( 0, 0, 0, 0 );

  auto *textRow = new QHBoxLayout();
  outer->addLayout( textRow );
  textRow->addWidget( new QLabel( tr( "Text:" ) ) );
  mTextEdit = new QLineEdit();
  mTextEdit->setPlaceholderText( tr( "Enter text" ) );
  textRow->addWidget( mTextEdit, 1 );

  auto *styleRow = new QHBoxLayout();
  outer->addLayout( styleRow );

  mFontCombo = new QFontComboBox();
  mFontCombo->setToolTip( tr( "Font family" ) );
  styleRow->addWidget( mFontCombo, 1 );

  mSizeSpin = new QDoubleSpinBox();
  mSizeSpin->setRange( 1.0, 200.0 );
  mSizeSpin->setDecimals( 1 );
  mSizeSpin->setSingleStep( 0.5 );
  mSizeSpin->setSuffix( QStringLiteral( " pt" ) );
  mSizeSpin->setToolTip( tr( "Font size" ) );
  styleRow->addWidget( mSizeSpin );

  mColorBtn = new QgsColorButton();
  mColorBtn->setAllowOpacity( true );
  mColorBtn->setToolTip( tr( "Text color" ) );
  styleRow->addWidget( mColorBtn );

  styleRow->addWidget( new QLabel( tr( "Border:" ) ) );
  mBufferColorBtn = new QgsColorButton();
  mBufferColorBtn->setAllowOpacity( true );
  mBufferColorBtn->setShowNoColor( true );
  mBufferColorBtn->setToolTip( tr( "Text border color" ) );
  styleRow->addWidget( mBufferColorBtn );

  mBufferWidthSpin = new QDoubleSpinBox();
  mBufferWidthSpin->setRange( 0.0, 10.0 );
  mBufferWidthSpin->setDecimals( 1 );
  mBufferWidthSpin->setSingleStep( 0.1 );
  mBufferWidthSpin->setSuffix( QStringLiteral( " mm" ) );
  mBufferWidthSpin->setToolTip( tr( "Text border width" ) );
  styleRow->addWidget( mBufferWidthSpin );

  // Live preview while typing; commit on focus-out / Enter.
  connect( mTextEdit, &QLineEdit::textChanged, this, &KadasAnnotationStyleEditor::previewChanged );
  connect( mTextEdit, &QLineEdit::editingFinished, this, &KadasAnnotationStyleEditor::committed );

  connect( mFontCombo, &QFontComboBox::currentFontChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mSizeSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mBufferColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mBufferWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
}

void KadasPointTextStyleEditor::loadFromItem( const QgsAnnotationItem *item )
{
  const auto *pt = dynamic_cast<const QgsAnnotationPointTextItem *>( item );
  if ( !pt )
    return;
  const QgsTextFormat fmt = pt->format();
  const QSignalBlocker b0( mTextEdit );
  const QSignalBlocker b1( mFontCombo ), b2( mSizeSpin ), b3( mColorBtn );
  const QSignalBlocker b4( mBufferColorBtn ), b5( mBufferWidthSpin );
  mTextEdit->setText( pt->text() );
  mTextEdit->setFocus();
  mTextEdit->selectAll();
  mFontCombo->setCurrentFont( fmt.font() );
  mSizeSpin->setValue( fmt.size() );
  mColorBtn->setColor( fmt.color() );
  const QgsTextBufferSettings buf = fmt.buffer();
  mBufferColorBtn->setColor( buf.enabled() ? buf.color() : QColor( 0, 0, 0, 0 ) );
  mBufferWidthSpin->setValue( buf.enabled() ? buf.size() : 0.0 );
}

void KadasPointTextStyleEditor::applyToItem( QgsAnnotationItem *item ) const
{
  auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( item );
  if ( !pt )
    return;
  pt->setText( mTextEdit->text() );
  QgsTextFormat fmt = pt->format();
  fmt.setFont( mFontCombo->currentFont() );
  fmt.setSize( mSizeSpin->value() );
  fmt.setSizeUnit( Qgis::RenderUnit::Points );
  fmt.setColor( mColorBtn->color() );
  QgsTextBufferSettings buf = fmt.buffer();
  const double bw = mBufferWidthSpin->value();
  const QColor bc = mBufferColorBtn->color();
  buf.setEnabled( bw > 0.0 && bc.alpha() > 0 );
  buf.setColor( bc );
  buf.setSize( bw );
  buf.setSizeUnit( Qgis::RenderUnit::Millimeters );
  fmt.setBuffer( buf );
  pt->setFormat( fmt );
}
