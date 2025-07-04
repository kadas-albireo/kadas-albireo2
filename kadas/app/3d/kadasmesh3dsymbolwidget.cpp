/***************************************************************************
  qgsmesh3dsymbolwidget.cpp
  -------------------------
  Date                 : January 2019
  Copyright            : (C) 2019 by Peter Petrik
  Email                : zilolv at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadasmesh3dsymbolwidget.h"
#include "qgs3dtypes.h"
#include "qgsmeshlayer.h"
#include "qgstriangularmesh.h"
#include "qgsmeshdataprovider.h"
#include "qgsmesh3dsymbol.h"
#include "qgsmeshlayer3drenderer.h"
#include "qgsmeshdatasetgrouptreeview.h"

KadasMesh3DSymbolWidget::KadasMesh3DSymbolWidget( QgsMeshLayer *meshLayer, QWidget *parent )
  : QWidget( parent )
{
  setupUi( this );
  mSpinBoxWireframeLineWidth->setClearValue( 1.0 );
  mSpinBoxVerticaleScale->setClearValue( 1.0 );
  mArrowsSpacingSpinBox->setClearValue( 25.0 );

  mComboBoxTextureType->addItem( tr( "Single Color" ), static_cast<int>( QgsMesh3DSymbol::RenderingStyle::SingleColor ) );
  mComboBoxTextureType->setCurrentIndex( 0 );

  mCullingMode->addItem( tr( "No Culling" ), Qgs3DTypes::NoCulling );
  mCullingMode->addItem( tr( "Front" ), Qgs3DTypes::Front );
  mCullingMode->addItem( tr( "Back" ), Qgs3DTypes::Back );

  mCullingMode->setItemData( 0, tr( "Both sides of the mesh are visible" ), Qt::ToolTipRole );
  mCullingMode->setItemData( 1, tr( "Only the back of the mesh is visible" ), Qt::ToolTipRole );
  mCullingMode->setItemData( 2, tr( "Only the front of the mesh is visible" ), Qt::ToolTipRole );

  mDatasetGroupListModel = new QgsMeshDatasetGroupListModel( this );
  mComboBoxDatasetVertical->setModel( mDatasetGroupListModel );
  setLayer( meshLayer );

  connect( mCullingMode, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &KadasMesh3DSymbolWidget::changed );

  connect( mChkSmoothTriangles, &QCheckBox::clicked, this, &KadasMesh3DSymbolWidget::changed );
  connect( mGroupBoxWireframe, &QGroupBox::toggled, this, &KadasMesh3DSymbolWidget::changed );
  connect( mColorButtonWireframe, &QgsColorButton::colorChanged, this, &KadasMesh3DSymbolWidget::changed );
  connect( mSpinBoxWireframeLineWidth, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &KadasMesh3DSymbolWidget::changed );
  connect( mLodSlider, &QSlider::valueChanged, this, &KadasMesh3DSymbolWidget::changed );

  connect( mColorRampShaderMinMaxReloadButton, &QPushButton::clicked, this, &KadasMesh3DSymbolWidget::reloadColorRampShaderMinMax );
  connect( mColorRampShaderWidget, &QgsColorRampShaderWidget::widgetChanged, this, &KadasMesh3DSymbolWidget::changed );

  connect( mColorRampShaderMinEdit, &QLineEdit::editingFinished, this, &KadasMesh3DSymbolWidget::onColorRampShaderMinMaxChanged );
  connect( mColorRampShaderMaxEdit, &QLineEdit::editingFinished, this, &KadasMesh3DSymbolWidget::onColorRampShaderMinMaxChanged );

  connect( mComboBoxTextureType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &KadasMesh3DSymbolWidget::onColoringTypeChanged );
  connect( mComboBoxTextureType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &KadasMesh3DSymbolWidget::changed );

  connect( mMeshSingleColorButton, &QgsColorButton::colorChanged, this, &KadasMesh3DSymbolWidget::changed );

  connect( mSpinBoxVerticaleScale, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &KadasMesh3DSymbolWidget::changed );

  connect( mComboBoxDatasetVertical, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &KadasMesh3DSymbolWidget::changed );

  connect( mCheckBoxVerticalMagnitudeRelative, &QCheckBox::clicked, this, &KadasMesh3DSymbolWidget::changed );

  connect( mGroupBoxArrowsSettings, &QGroupBox::toggled, this, &KadasMesh3DSymbolWidget::changed );

  connect( mArrowsSpacingSpinBox, static_cast<void ( QDoubleSpinBox::* )()>( &QDoubleSpinBox::editingFinished ), this, &KadasMesh3DSymbolWidget::changed );

  connect( mArrowsFixedSizeCheckBox, &QCheckBox::clicked, this, &KadasMesh3DSymbolWidget::changed );

  connect( mGroupBoxTextureSettings, &QgsCollapsibleGroupBox::collapsedStateChanged, this, &KadasMesh3DSymbolWidget::onTextureSettingsCollapseStateChanged );
}

void KadasMesh3DSymbolWidget::setSymbol( const QgsMesh3DSymbol *symbol )
{
  mSymbol.reset( symbol->clone() );
  mCullingMode->setCurrentIndex( mCullingMode->findData( symbol->cullingMode() ) );
  mChkSmoothTriangles->setChecked( symbol->smoothedTriangles() );
  mGroupBoxWireframe->setChecked( symbol->wireframeEnabled() );
  mColorButtonWireframe->setColor( symbol->wireframeLineColor() );
  mSpinBoxWireframeLineWidth->setValue( symbol->wireframeLineWidth() );
  if ( mLayer && mLayer->meshSimplificationSettings().isEnabled() )
    mLodSlider->setValue( mLayer->triangularMeshLevelOfDetailCount() - symbol->levelOfDetailIndex() - 1 );
  else
    mLodSlider->setValue( mLodSlider->maximum() );


  mSpinBoxVerticaleScale->setValue( symbol->verticalScale() );
  mComboBoxTextureType->setCurrentIndex( mComboBoxTextureType->findData( static_cast<int>( symbol->renderingStyle() ) ) );
  mMeshSingleColorButton->setColor( symbol->singleMeshColor() );
  mColorRampShaderWidget->setFromShader( symbol->colorRampShader() );
  mColorRampShaderWidget->setMinimumMaximumAndClassify( symbol->colorRampShader().minimumValue(), symbol->colorRampShader().maximumValue() );

  setColorRampMinMax( symbol->colorRampShader().minimumValue(), symbol->colorRampShader().maximumValue() );
  mComboBoxDatasetVertical->setCurrentIndex( symbol->verticalDatasetGroupIndex() );
  mCheckBoxVerticalMagnitudeRelative->setChecked( symbol->isVerticalMagnitudeRelative() );

  mGroupBoxArrowsSettings->setChecked( symbol->arrowsEnabled() );
  mArrowsSpacingSpinBox->setValue( symbol->arrowsSpacing() );
  mArrowsFixedSizeCheckBox->setChecked( symbol->arrowsFixedSize() );
}

void KadasMesh3DSymbolWidget::configureForTerrain()
{
  mComboBoxTextureType->addItem( tr( "Color Ramp Shader" ), static_cast<int>( QgsMesh3DSymbol::RenderingStyle::ColorRamp ) );
  enableVerticalSetting( false );
  enableArrowSettings( false );

  onColoringTypeChanged();
}

void KadasMesh3DSymbolWidget::configureForDataset()
{
  mComboBoxTextureType->addItem( tr( "2D Contour Color Ramp Shader" ), static_cast<int>( QgsMesh3DSymbol::RenderingStyle::ColorRamp2DRendering ) );
  mGroupBoxColorRampShader->hide();
  enableVerticalSetting( true );
  enableArrowSettings( true );

  onColoringTypeChanged();
}

void KadasMesh3DSymbolWidget::setLayer( QgsMeshLayer *meshLayer, bool updateSymbol )
{
  mLayer = meshLayer;

  if ( meshLayer && meshLayer->meshSimplificationSettings().isEnabled() )
  {
    mLodSlider->setEnabled( true );
    const int lodCount = meshLayer->triangularMeshLevelOfDetailCount();
    mLodSlider->setTickInterval( 1 );
    mLodSlider->setMaximum( lodCount - 1 );
  }
  else
  {
    mLodSlider->setValue( mLodSlider->maximum() );
    mLodSlider->setEnabled( false );
  }

  if ( !updateSymbol )
    return;

  if ( meshLayer )
  {
    mDatasetGroupListModel->syncToLayer( meshLayer );
    QgsMeshLayer3DRenderer *renderer = static_cast<QgsMeshLayer3DRenderer *>( meshLayer->renderer3D() );
    if ( renderer && renderer->type() == QLatin1String( "mesh" ) )
    {
      if ( renderer->symbol() && renderer->symbol()->type() == QLatin1String( "mesh" ) )
      {
        setSymbol( static_cast<const QgsMesh3DSymbol *>( renderer->symbol() ) );
        return;
      }
    }
  }

  const std::unique_ptr<QgsMesh3DSymbol> defaultSymbol = std::make_unique<QgsMesh3DSymbol>();
  // set symbol does not take ownership!
  setSymbol( defaultSymbol.get() );

  reloadColorRampShaderMinMax(); //As the symbol is new, the Color ramp shader needs to be initialized with min max value
}

QgsMeshLayer *KadasMesh3DSymbolWidget::meshLayer() const { return mLayer; }

double KadasMesh3DSymbolWidget::lineEditValue( const QLineEdit *lineEdit ) const
{
  if ( lineEdit->text().isEmpty() )
  {
    return std::numeric_limits<double>::quiet_NaN();
  }

  return lineEdit->text().toDouble();
}

std::unique_ptr<QgsMesh3DSymbol> KadasMesh3DSymbolWidget::symbol() const
{
  std::unique_ptr<QgsMesh3DSymbol> sym( mSymbol->clone() );

  sym->setCullingMode( static_cast<Qgs3DTypes::CullingMode>( mCullingMode->currentData().toInt() ) );
  sym->setSmoothedTriangles( mChkSmoothTriangles->isChecked() );
  sym->setWireframeEnabled( mGroupBoxWireframe->isChecked() );
  sym->setWireframeLineColor( mColorButtonWireframe->color() );
  sym->setWireframeLineWidth( mSpinBoxWireframeLineWidth->value() );
  if ( mLayer )
    sym->setLevelOfDetailIndex( mLayer->triangularMeshLevelOfDetailCount() - mLodSlider->sliderPosition() - 1 );
  else
    sym->setLevelOfDetailIndex( 0 );

  sym->setVerticalScale( mSpinBoxVerticaleScale->value() );
  sym->setRenderingStyle( static_cast<QgsMesh3DSymbol::RenderingStyle>( mComboBoxTextureType->currentData().toInt() ) );
  sym->setSingleMeshColor( mMeshSingleColorButton->color() );
  sym->setVerticalDatasetGroupIndex( mComboBoxDatasetVertical->currentIndex() );
  sym->setIsVerticalMagnitudeRelative( mCheckBoxVerticalMagnitudeRelative->isChecked() );

  if ( sym->renderingStyle() == QgsMesh3DSymbol::RenderingStyle::ColorRamp )
    sym->setColorRampShader( mColorRampShaderWidget->shader() );

  sym->setArrowsEnabled( mGroupBoxArrowsSettings->isChecked() );
  sym->setArrowsSpacing( mArrowsSpacingSpinBox->value() );
  sym->setArrowsFixedSize( mArrowsFixedSizeCheckBox->isChecked() );

  return sym;
}

void KadasMesh3DSymbolWidget::reloadColorRampShaderMinMax()
{
  if ( !mLayer )
    return;

  QgsTriangularMesh *triangleMesh = mLayer->triangularMesh();
  if ( !triangleMesh )
    return;

  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();
  for ( int i = 0; i < triangleMesh->vertices().count(); ++i )
  {
    const double zValue = triangleMesh->vertices().at( i ).z();
    if ( zValue > max )
      max = zValue;
    if ( zValue < min )
      min = zValue;
  }
  setColorRampMinMax( min, max );

  mColorRampShaderWidget->setMinimumMaximum( min, max );
  mColorRampShaderWidget->classify();
}

void KadasMesh3DSymbolWidget::onColorRampShaderMinMaxChanged()
{
  const double min = lineEditValue( mColorRampShaderMinEdit );
  const double max = lineEditValue( mColorRampShaderMaxEdit );
  mColorRampShaderWidget->setMinimumMaximum( min, max );
  mColorRampShaderWidget->classify();
}

void KadasMesh3DSymbolWidget::onColoringTypeChanged()
{
  mGroupBoxColorRampShader->setVisible( static_cast<QgsMesh3DSymbol::RenderingStyle>( mComboBoxTextureType->currentData().toInt() ) == QgsMesh3DSymbol::RenderingStyle::ColorRamp );
  mMeshSingleColorWidget->setVisible( static_cast<QgsMesh3DSymbol::RenderingStyle>( mComboBoxTextureType->currentData().toInt() ) == QgsMesh3DSymbol::RenderingStyle::SingleColor );
}

void KadasMesh3DSymbolWidget::onTextureSettingsCollapseStateChanged( bool collapsed )
{
  if ( !collapsed )
  {
    onColoringTypeChanged();
  }
}

void KadasMesh3DSymbolWidget::setColorRampMinMax( double min, double max )
{
  whileBlocking( mColorRampShaderMinEdit )->setText( QString::number( min ) );
  whileBlocking( mColorRampShaderMaxEdit )->setText( QString::number( max ) );
}

void KadasMesh3DSymbolWidget::enableVerticalSetting( bool isEnable )
{
  mGroupBoxVerticaleSettings->setVisible( isEnable );
}

void KadasMesh3DSymbolWidget::enableArrowSettings( bool isEnable )
{
  mGroupBoxArrowsSettings->setVisible( isEnable );
}
