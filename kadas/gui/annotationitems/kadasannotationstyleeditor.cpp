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
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QUrl>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscallout.h>
#include <qgis/qgscolorbutton.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsproject.h>
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


// ----- Picture (with balloon callout) -----------------------------------

namespace
{
  Qgis::PictureFormat pictureFormatFromPath( const QString &path )
  {
    const QString ext = QFileInfo( path ).suffix().toLower();
    if ( ext == QLatin1String( "svg" ) )
      return Qgis::PictureFormat::SVG;
    return Qgis::PictureFormat::Raster;
  }
} // namespace

KadasPictureStyleEditor::KadasPictureStyleEditor( QWidget *parent )
  : KadasAnnotationStyleEditor( parent )
{
  auto *row = new QHBoxLayout( this );
  row->setContentsMargins( 0, 0, 0, 0 );

  mChangeImageBtn = new QToolButton();
  mChangeImageBtn->setText( tr( "Change image…" ) );
  mChangeImageBtn->setToolTip( tr( "Pick a new picture from a file or fetch one from a URL." ) );
  // Tool-button + dropdown menu so the user can pick between local file
  // or a remote URL. InstantPopup means the entire button surface opens
  // the menu (rather than splitting click vs arrow), which is the
  // cleanest UX when both choices are first-class.
  mChangeImageBtn->setPopupMode( QToolButton::InstantPopup );
  mChangeImageBtn->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  auto *menu = new QMenu( mChangeImageBtn );
  QAction *fromFileAction = menu->addAction( tr( "From file…" ) );
  QAction *fromUrlAction = menu->addAction( tr( "From URL…" ) );
  mChangeImageBtn->setMenu( menu );
  row->addWidget( mChangeImageBtn );

  row->addWidget( new QLabel( tr( "Size:" ) ) );
  mWidthSpin = new QSpinBox();
  mWidthSpin->setRange( 16, 2000 );
  mWidthSpin->setSuffix( QStringLiteral( " px" ) );
  mWidthSpin->setToolTip( tr( "Picture width in pixels" ) );
  row->addWidget( mWidthSpin );
  mHeightSpin = new QSpinBox();
  mHeightSpin->setRange( 16, 2000 );
  mHeightSpin->setSuffix( QStringLiteral( " px" ) );
  mHeightSpin->setToolTip( tr( "Picture height in pixels" ) );
  row->addWidget( mHeightSpin );

  row->addWidget( new QLabel( tr( "Frame:" ) ) );
  mFillColorBtn = new QgsColorButton();
  mFillColorBtn->setAllowOpacity( true );
  mFillColorBtn->setShowNoColor( true );
  mFillColorBtn->setToolTip( tr( "Balloon fill color" ) );
  row->addWidget( mFillColorBtn );

  mStrokeColorBtn = new QgsColorButton();
  mStrokeColorBtn->setAllowOpacity( true );
  mStrokeColorBtn->setShowNoColor( true );
  mStrokeColorBtn->setToolTip( tr( "Balloon outline color" ) );
  row->addWidget( mStrokeColorBtn );

  mStrokeWidthSpin = new QDoubleSpinBox();
  mStrokeWidthSpin->setRange( 0.0, 10.0 );
  mStrokeWidthSpin->setDecimals( 1 );
  mStrokeWidthSpin->setSingleStep( 0.5 );
  mStrokeWidthSpin->setSuffix( QStringLiteral( " px" ) );
  mStrokeWidthSpin->setToolTip( tr( "Balloon outline width" ) );
  row->addWidget( mStrokeWidthSpin );

  row->addWidget( new QLabel( tr( "Wedge:" ) ) );
  mWedgeWidthSpin = new QDoubleSpinBox();
  mWedgeWidthSpin->setRange( 1.0, 60.0 );
  mWedgeWidthSpin->setDecimals( 1 );
  mWedgeWidthSpin->setSingleStep( 1.0 );
  mWedgeWidthSpin->setSuffix( QStringLiteral( " px" ) );
  mWedgeWidthSpin->setToolTip( tr( "Width of the balloon wedge base" ) );
  row->addWidget( mWedgeWidthSpin );

  connect( fromFileAction, &QAction::triggered, this, [this]() {
    const QString chosen
      = QFileDialog::getOpenFileName( this, tr( "Select picture" ), mPath.isEmpty() ? QDir::homePath() : QFileInfo( mPath ).absolutePath(), tr( "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff *.svg)" ) );
    if ( chosen.isEmpty() )
      return;
    mPath = chosen;
    emit committed();
  } );
  connect( fromUrlAction, &QAction::triggered, this, [this]() {
    bool ok = false;
    const QString urlStr = QInputDialog::getText( this, tr( "Picture URL" ), tr( "Enter the URL of an image:" ), QLineEdit::Normal, QString(), &ok );
    if ( !ok || urlStr.trimmed().isEmpty() )
      return;
    const QUrl url( urlStr.trimmed() );
    if ( !url.isValid() || !( url.scheme().compare( QLatin1String( "http" ), Qt::CaseInsensitive ) == 0 || url.scheme().compare( QLatin1String( "https" ), Qt::CaseInsensitive ) == 0 ) )
    {
      QMessageBox::warning( this, tr( "Picture URL" ), tr( "Please enter a valid http:// or https:// URL." ) );
      return;
    }
    // Blocking GET keeps the wiring simple: the user already paused the
    // edit flow on the menu, an extra modal wait is acceptable here.
    QNetworkRequest req( url );
    QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( req );
    if ( content.error() != QNetworkReply::NoError || content.content().isEmpty() )
    {
      QMessageBox::warning( this, tr( "Picture URL" ), tr( "Failed to download image: %1" ).arg( content.errorString() ) );
      return;
    }
    // Save into the project archive so the picture survives a project
    // save/reload — same lifecycle as a file picked from disk via
    // KadasApplication::addImageItem. Picks the basename from the URL
    // path, falling back to a generic name if the URL has no file part.
    QString baseName = QFileInfo( url.path() ).fileName();
    if ( baseName.isEmpty() )
      baseName = QStringLiteral( "downloaded_image" );
    const QString attached = QgsProject::instance()->createAttachedFile( baseName );
    QFile out( attached );
    if ( !out.open( QIODevice::WriteOnly ) )
    {
      QMessageBox::warning( this, tr( "Picture URL" ), tr( "Failed to write image to project archive." ) );
      return;
    }
    out.write( content.content() );
    out.close();
    mPath = attached;
    emit committed();
  } );
  connect( mWidthSpin, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mHeightSpin, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mFillColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeColorBtn, &QgsColorButton::colorChanged, this, &KadasAnnotationStyleEditor::committed );
  connect( mStrokeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
  connect( mWedgeWidthSpin, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasAnnotationStyleEditor::committed );
}

void KadasPictureStyleEditor::loadFromItem( const QgsAnnotationItem *item )
{
  const auto *pic = dynamic_cast<const QgsAnnotationPictureItem *>( item );
  if ( !pic )
    return;
  const QSignalBlocker b1( mWidthSpin ), b2( mHeightSpin ), b3( mFillColorBtn );
  const QSignalBlocker b4( mStrokeColorBtn ), b5( mStrokeWidthSpin ), b6( mWedgeWidthSpin );
  mPath = pic->path();
  const QSizeF size = pic->fixedSize();
  mWidthSpin->setValue( static_cast<int>( std::round( size.width() ) ) );
  mHeightSpin->setValue( static_cast<int>( std::round( size.height() ) ) );

  // Pull fill / stroke from the balloon callout's symbol layer if present;
  // fall back to the same defaults the controller uses (white / black /
  // 1px / 6px wedge).
  QColor fill = Qt::white;
  QColor stroke = Qt::black;
  double strokeWidth = 1.0;
  double wedge = 6.0;
  if ( auto *balloon = dynamic_cast<QgsBalloonCallout *>( pic->callout() ) )
  {
    if ( QgsFillSymbol *sym = balloon->fillSymbol() )
    {
      if ( sym->symbolLayerCount() > 0 )
      {
        if ( const auto *sl = dynamic_cast<const QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) ) )
        {
          fill = sl->color();
          stroke = sl->strokeColor();
          strokeWidth = sl->strokeWidth();
        }
      }
    }
    wedge = balloon->wedgeWidth();
  }
  mFillColorBtn->setColor( fill );
  mStrokeColorBtn->setColor( stroke );
  mStrokeWidthSpin->setValue( strokeWidth );
  mWedgeWidthSpin->setValue( wedge );
}

void KadasPictureStyleEditor::applyToItem( QgsAnnotationItem *item ) const
{
  auto *pic = dynamic_cast<QgsAnnotationPictureItem *>( item );
  if ( !pic )
    return;

  // Image source — only re-set if the user actually picked a new file
  // (mPath is updated synchronously by the file dialog).
  if ( !mPath.isEmpty() && mPath != pic->path() )
    pic->setPath( pictureFormatFromPath( mPath ), mPath );

  pic->setFixedSize( QSizeF( mWidthSpin->value(), mHeightSpin->value() ) );
  pic->setFixedSizeUnit( Qgis::RenderUnit::Pixels );

  // Rebuild the balloon callout's fill symbol with the current colors /
  // stroke width. Keep the (margins, corner radius, wedge unit) the
  // controller installed so the look stays consistent.
  auto *balloon = dynamic_cast<QgsBalloonCallout *>( pic->callout() );
  bool installed = false;
  if ( !balloon )
  {
    balloon = new QgsBalloonCallout();
    installed = true;
  }
  auto *sl = new QgsSimpleFillSymbolLayer( mFillColorBtn->color(), Qt::SolidPattern, mStrokeColorBtn->color(), Qt::SolidLine, mStrokeWidthSpin->value() );
  sl->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
  balloon->setFillSymbol( new QgsFillSymbol( QgsSymbolLayerList() << sl ) );
  balloon->setWedgeWidth( mWedgeWidthSpin->value() );
  balloon->setWedgeWidthUnit( Qgis::RenderUnit::Pixels );
  if ( installed )
    pic->setCallout( balloon );
}
