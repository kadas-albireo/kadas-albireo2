/***************************************************************************
  qgs3dmapconfigwidget.cpp
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadas3dmapconfigwidget.h"


#include "qgs3dmapsettings.h"
#include "qgsdemterrainsettings.h"
#include "qgsflatterrainsettings.h"
#include "qgsonlinedemterrainsettings.h"
#include "qgsmeshterrainsettings.h"
#include "qgsquantizedmeshterrainsettings.h"
#include "qgs3dutils.h"
#include "qgsguiutils.h"
#include "qgsmapcanvas.h"
#include "qgsquantizedmeshterraingenerator.h"
#include "qgsrasterlayer.h"
#include "qgsmeshlayer.h"
#include "qgsproject.h"
#include "kadasmesh3dsymbolwidget.h"
#include "qgssettings.h"
#include "kadasskyboxrenderingsettingswidget.h"
#include "kadasshadowrenderingsettingswidget.h"
#include "kadasambientocclusionsettingswidget.h"
#include "qgs3dmapcanvas.h"
#include "qgsterraingenerator.h"
#include "qgstiledscenelayer.h"

Kadas3DMapConfigWidget::Kadas3DMapConfigWidget( Qgs3DMapSettings *map, QgsMapCanvas *mainCanvas, Qgs3DMapCanvas *mapCanvas3D, QWidget *parent )
  : QWidget( parent )
  , mMap( map )
  , mMainCanvas( mainCanvas )
  , m3DMapCanvas( mapCanvas3D )
{
  setupUi( this );

  Q_ASSERT( map );
  Q_ASSERT( mainCanvas );

  const QgsSettings settings;

  const int iconSize = QgsGuiUtils::scaleIconSize( 20 );
  m3DOptionsListWidget->setIconSize( QSize( iconSize, iconSize ) );

  mCameraNavigationModeCombo->addItem( tr( "Terrain Based" ), QVariant::fromValue( Qgis::NavigationMode::TerrainBased ) );
  mCameraNavigationModeCombo->addItem( tr( "Walk Mode (First Person)" ), QVariant::fromValue( Qgis::NavigationMode::Walk ) );

  // get rid of annoying outer focus rect on Mac
  m3DOptionsListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );
  m3DOptionsListWidget->setCurrentRow( settings.value( QStringLiteral( "Windows/3DMapConfig/Tab" ), 0 ).toInt() );
  connect( m3DOptionsListWidget, &QListWidget::currentRowChanged, this, [=]( int index ) { m3DOptionsStackedWidget->setCurrentIndex( index ); } );
  m3DOptionsStackedWidget->setCurrentIndex( m3DOptionsListWidget->currentRow() );

  if ( !settings.contains( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ) )
  {
    // set left list widget width on initial showing
    QList<int> splitsizes;
    splitsizes << 115;
    m3DOptionsSplitter->setSizes( splitsizes );
  }
  m3DOptionsSplitter->restoreState( settings.value( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ).toByteArray() );

  mMeshSymbolWidget = new KadasMesh3DSymbolWidget( nullptr, groupMeshTerrainShading );
  mMeshSymbolWidget->configureForTerrain();

  cboCameraProjectionType->addItem( tr( "Perspective Projection" ), Qt3DRender::QCameraLens::PerspectiveProjection );
  cboCameraProjectionType->addItem( tr( "Orthogonal Projection" ), Qt3DRender::QCameraLens::OrthographicProjection );
  connect( cboCameraProjectionType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, [=]() {
    spinCameraFieldOfView->setEnabled( cboCameraProjectionType->currentIndex() == cboCameraProjectionType->findData( Qt3DRender::QCameraLens::PerspectiveProjection ) );
  } );

  mCameraMovementSpeed->setClearValue( 4 );
  spinCameraFieldOfView->setClearValue( 45.0 );
  spinTerrainScale->setClearValue( 1.0 );
  spinTerrainResolution->setClearValue( 32 );
  spinTerrainSkirtHeight->setClearValue( 300 );
  spinMapResolution->setClearValue( 512 );
  spinScreenError->setClearValue( 2 );
  spinGroundError->setClearValue( 1 );
  terrainElevationOffsetSpinBox->setClearValue( 0.0 );
  edlStrengthSpinBox->setClearValue( 1000 );
  edlDistanceSpinBox->setClearValue( 1 );
  mDebugShadowMapSizeSpinBox->setClearValue( 0.1 );
  mDebugDepthMapSizeSpinBox->setClearValue( 0.1 );

  cboTerrainLayer->setAllowEmptyLayer( true );
  cboTerrainLayer->setFilters( Qgis::LayerFilter::RasterLayer );

  cboTerrainType->addItem( tr( "Flat Terrain" ), QgsTerrainGenerator::Flat );
  cboTerrainType->addItem( tr( "DEM (Raster Layer)" ), QgsTerrainGenerator::Dem );
  cboTerrainType->addItem( tr( "Online" ), QgsTerrainGenerator::Online );
  cboTerrainType->addItem( tr( "Mesh" ), QgsTerrainGenerator::Mesh );
  cboTerrainType->addItem( tr( "Quantized Mesh" ), QgsTerrainGenerator::QuantizedMesh );

  groupTerrain->setChecked( mMap->terrainRenderingEnabled() );

  const QgsAbstractTerrainSettings *terrainSettings = mMap->terrainSettings();
  if ( terrainSettings )
  {
    // common properties
    terrainElevationOffsetSpinBox->setValue( terrainSettings->elevationOffset() );
    spinTerrainScale->setValue( terrainSettings->verticalScale() );
    spinMapResolution->setValue( terrainSettings->mapTileResolution() );
    spinScreenError->setValue( terrainSettings->maximumScreenError() );
    spinGroundError->setValue( terrainSettings->maximumGroundError() );
  }

  if ( terrainSettings && terrainSettings->type() == QLatin1String( "dem" ) )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Dem ) );
    const QgsDemTerrainSettings *demTerrainSettings = qgis::down_cast<const QgsDemTerrainSettings *>( terrainSettings );
    spinTerrainResolution->setValue( demTerrainSettings->resolution() );
    spinTerrainSkirtHeight->setValue( demTerrainSettings->skirtHeight() );
    cboTerrainLayer->setFilters( Qgis::LayerFilter::RasterLayer );
    cboTerrainLayer->setLayer( demTerrainSettings->layer() );
  }
  else if ( terrainSettings && terrainSettings->type() == QLatin1String( "online" ) )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Online ) );
    const QgsOnlineDemTerrainSettings *demTerrainSettings = qgis::down_cast<const QgsOnlineDemTerrainSettings *>( terrainSettings );
    spinTerrainResolution->setValue( demTerrainSettings->resolution() );
    spinTerrainSkirtHeight->setValue( demTerrainSettings->skirtHeight() );
  }
  else if ( terrainSettings && terrainSettings->type() == QLatin1String( "mesh" ) )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Mesh ) );
    const QgsMeshTerrainSettings *meshTerrainSettings = qgis::down_cast<const QgsMeshTerrainSettings *>( terrainSettings );
    cboTerrainLayer->setFilters( Qgis::LayerFilter::MeshLayer );
    cboTerrainLayer->setLayer( meshTerrainSettings->layer() );
    mMeshSymbolWidget->setLayer( meshTerrainSettings->layer(), false );
    mMeshSymbolWidget->setSymbol( meshTerrainSettings->symbol() );
    spinTerrainScale->setValue( meshTerrainSettings->symbol()->verticalScale() );
  }
  else if ( terrainSettings && terrainSettings->type() == QLatin1String( "quantizedmesh" ) )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::QuantizedMesh ) );
    const QgsQuantizedMeshTerrainSettings *quantizedMeshTerrainSettings = qgis::down_cast<const QgsQuantizedMeshTerrainSettings *>( terrainSettings );
    cboTerrainLayer->setFilters( Qgis::LayerFilter::TiledSceneLayer );
    cboTerrainLayer->setLayer( quantizedMeshTerrainSettings->layer() );
  }
  else if ( terrainSettings && terrainSettings->type() == QLatin1String( "flat" ) )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Flat ) );
    cboTerrainLayer->setLayer( nullptr );
    spinTerrainResolution->setValue( 32 );
    spinTerrainSkirtHeight->setValue( 300 );
  }

  spinCameraFieldOfView->setValue( mMap->fieldOfView() );
  cboCameraProjectionType->setCurrentIndex( cboCameraProjectionType->findData( mMap->projectionType() ) );
  mCameraNavigationModeCombo->setCurrentIndex( mCameraNavigationModeCombo->findData( QVariant::fromValue( mMap->cameraNavigationMode() ) ) );
  mCameraMovementSpeed->setValue( mMap->cameraMovementSpeed() );
  spinTerrainScale->setValue( mMap->terrainVerticalScale() );
  spinMapResolution->setValue( mMap->mapTileResolution() );
  spinScreenError->setValue( mMap->maxTerrainScreenError() );
  spinGroundError->setValue( mMap->maxTerrainGroundError() );
  terrainElevationOffsetSpinBox->setValue( mMap->terrainElevationOffset() );
  chkShowLabels->setChecked( mMap->showLabels() );
  chkShowTileInfo->setChecked( mMap->showTerrainTilesInfo() );
  chkShowBoundingBoxes->setChecked( mMap->showTerrainBoundingBoxes() );
  chkShowCameraViewCenter->setChecked( mMap->showCameraViewCenter() );
  chkShowCameraRotationCenter->setChecked( mMap->showCameraRotationCenter() );
  chkShowLightSourceOrigins->setChecked( mMap->showLightSourceOrigins() );
  mFpsCounterCheckBox->setChecked( mMap->isFpsCounterEnabled() );

  mDebugOverlayCheckBox->setChecked( mMap->isDebugOverlayEnabled() );
  mDebugOverlayCheckBox->setVisible( true );

  groupTerrainShading->setChecked( mMap->isTerrainShadingEnabled() );
  widgetTerrainMaterial->setTechnique( QgsMaterialSettingsRenderingTechnique::TrianglesWithFixedTexture );
  QgsPhongMaterialSettings terrainShadingMaterial = mMap->terrainShadingMaterial();
  widgetTerrainMaterial->setSettings( &terrainShadingMaterial, nullptr );

  widgetLights->setLights( mMap->lightSources() );

  connect( cboTerrainType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::onTerrainTypeChanged );
  connect( cboTerrainLayer, static_cast<void ( QComboBox::* )( int )>( &QgsMapLayerComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::onTerrainLayerChanged );
  connect( spinMapResolution, static_cast<void ( QSpinBox::* )( int )>( &QSpinBox::valueChanged ), this, &Kadas3DMapConfigWidget::updateMaxZoomLevel );
  connect( spinGroundError, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &Kadas3DMapConfigWidget::updateMaxZoomLevel );

  groupMeshTerrainShading->layout()->addWidget( mMeshSymbolWidget );

  // ==================
  // Page: Skybox
  mSkyboxSettingsWidget = new KadasSkyboxRenderingSettingsWidget( this );
  mSkyboxSettingsWidget->setSkyboxSettings( map->skyboxSettings() );
  groupSkyboxSettings->layout()->addWidget( mSkyboxSettingsWidget );
  groupSkyboxSettings->setChecked( mMap->isSkyboxEnabled() );

  // ==================
  // Page: Shadows
  mShadowSettingsWidget = new KadasShadowRenderingSettingsWidget( this );
  mShadowSettingsWidget->onDirectionalLightsCountChanged( widgetLights->directionalLightCount() );
  mShadowSettingsWidget->setShadowSettings( map->shadowSettings() );
  groupShadowRendering->layout()->addWidget( mShadowSettingsWidget );
  connect( widgetLights, &KadasLightsWidget::directionalLightsCountChanged, mShadowSettingsWidget, &KadasShadowRenderingSettingsWidget::onDirectionalLightsCountChanged );

  connect( widgetLights, &KadasLightsWidget::lightsAdded, this, &Kadas3DMapConfigWidget::validate );
  connect( widgetLights, &KadasLightsWidget::lightsRemoved, this, &Kadas3DMapConfigWidget::validate );

  groupShadowRendering->setChecked( map->shadowSettings().renderShadows() );

  // ==================
  // Page: 3D axis
  mCbo3dAxisType->addItem( tr( "Coordinate Reference System" ), static_cast<int>( Qgs3DAxisSettings::Mode::Crs ) );
  mCbo3dAxisType->addItem( tr( "Cube" ), static_cast<int>( Qgs3DAxisSettings::Mode::Cube ) );

  mCbo3dAxisHorizPos->addItem( tr( "Left" ), static_cast<int>( Qt::AnchorPoint::AnchorLeft ) );
  mCbo3dAxisHorizPos->addItem( tr( "Center" ), static_cast<int>( Qt::AnchorPoint::AnchorHorizontalCenter ) );
  mCbo3dAxisHorizPos->addItem( tr( "Right" ), static_cast<int>( Qt::AnchorPoint::AnchorRight ) );

  mCbo3dAxisVertPos->addItem( tr( "Top" ), static_cast<int>( Qt::AnchorPoint::AnchorTop ) );
  mCbo3dAxisVertPos->addItem( tr( "Middle" ), static_cast<int>( Qt::AnchorPoint::AnchorVerticalCenter ) );
  mCbo3dAxisVertPos->addItem( tr( "Bottom" ), static_cast<int>( Qt::AnchorPoint::AnchorBottom ) );

  init3DAxisPage();

  // ==================
  // Page: 2D/3D canvas sync
  mSync2DTo3DCheckbox->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  mSync3DTo2DCheckbox->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  mVisualizeExtentCheckBox->setChecked( map->viewFrustumVisualizationEnabled() );

  // ==================
  // Page: Advanced

  // EyeDomeLight
  edlGroupBox->setChecked( map->eyeDomeLightingEnabled() );
  edlStrengthSpinBox->setValue( map->eyeDomeLightingStrength() );
  edlDistanceSpinBox->setValue( map->eyeDomeLightingDistance() );

  mDebugShadowMapCornerComboBox->addItem( tr( "Top Left" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Top Right" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Bottom Left" ) );
  mDebugShadowMapCornerComboBox->addItem( tr( "Bottom Right" ) );

  mDebugDepthMapCornerComboBox->addItem( tr( "Top Left" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Top Right" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Bottom Left" ) );
  mDebugDepthMapCornerComboBox->addItem( tr( "Bottom Right" ) );

  mDebugShadowMapGroupBox->setChecked( map->debugShadowMapEnabled() );

  mDebugShadowMapCornerComboBox->setCurrentIndex( static_cast<int>( map->debugShadowMapCorner() ) );
  mDebugShadowMapSizeSpinBox->setValue( map->debugShadowMapSize() );

  mDebugDepthMapGroupBox->setChecked( map->debugDepthMapEnabled() );
  mDebugDepthMapCornerComboBox->setCurrentIndex( static_cast<int>( map->debugDepthMapCorner() ) );
  mDebugDepthMapSizeSpinBox->setValue( map->debugDepthMapSize() );

  // Ambient occlusion
  mAmbientOcclusionSettingsWidget->setAmbientOcclusionSettings( map->ambientOcclusionSettings() );

  // ==================
  // Page: General

  groupExtent->setOutputCrs( mMap->crs() );
  groupExtent->setCurrentExtent( mMap->extent(), mMap->crs() );
  groupExtent->setOutputExtentFromCurrent();
  groupExtent->setMapCanvas( mMainCanvas );

  // checkbox to display the extent in the 2D Map View
  mShowExtentIn2DViewCheckbox = new QCheckBox( tr( "Show in 2D map view" ) );
  mShowExtentIn2DViewCheckbox->setChecked( map->showExtentIn2DView() );
  groupExtent->layout()->addWidget( mShowExtentIn2DViewCheckbox );

  onTerrainTypeChanged();
}

Kadas3DMapConfigWidget::~Kadas3DMapConfigWidget()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ), m3DOptionsSplitter->saveState() );
  settings.setValue( QStringLiteral( "Windows/3DMapConfig/Tab" ), m3DOptionsListWidget->currentRow() );
}

void Kadas3DMapConfigWidget::apply()
{
  mMap->setExtent( groupExtent->outputExtent() );
  mMap->setShowExtentIn2DView( mShowExtentIn2DViewCheckbox->isChecked() );

  const QgsTerrainGenerator::Type terrainType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );

  mMap->setTerrainRenderingEnabled( groupTerrain->isChecked() );
  std::unique_ptr<QgsAbstractTerrainSettings> terrainSettings;
  switch ( terrainType )
  {
    case QgsTerrainGenerator::Flat:
    {
      terrainSettings = std::make_unique<QgsFlatTerrainSettings>();
      break;
    }

    case QgsTerrainGenerator::Dem:
    {
      auto demTerrainSettings = std::make_unique<QgsDemTerrainSettings>();
      demTerrainSettings->setLayer( qobject_cast<QgsRasterLayer *>( cboTerrainLayer->currentLayer() ) );
      demTerrainSettings->setResolution( spinTerrainResolution->value() );
      demTerrainSettings->setSkirtHeight( spinTerrainSkirtHeight->value() );
      terrainSettings = std::move( demTerrainSettings );
      break;
    }

    case QgsTerrainGenerator::Online:
    {
      auto onlineTerrainSettings = std::make_unique<QgsOnlineDemTerrainSettings>();
      onlineTerrainSettings->setResolution( spinTerrainResolution->value() );
      onlineTerrainSettings->setSkirtHeight( spinTerrainSkirtHeight->value() );
      terrainSettings = std::move( onlineTerrainSettings );
      break;
    }

    case QgsTerrainGenerator::Mesh:
    {
      auto meshTerrainSettings = std::make_unique<QgsMeshTerrainSettings>();
      meshTerrainSettings->setLayer( qobject_cast<QgsMeshLayer *>( cboTerrainLayer->currentLayer() ) );

      std::unique_ptr<QgsMesh3DSymbol> symbol = mMeshSymbolWidget->symbol();
      symbol->setVerticalScale( spinTerrainScale->value() );
      meshTerrainSettings->setSymbol( symbol.release() );

      terrainSettings = std::move( meshTerrainSettings );
      break;
    }

    case QgsTerrainGenerator::QuantizedMesh:
    {
      auto meshTerrainSettings = std::make_unique<QgsQuantizedMeshTerrainSettings>();
      meshTerrainSettings->setLayer( qobject_cast<QgsTiledSceneLayer *>( cboTerrainLayer->currentLayer() ) );

      terrainSettings = std::move( meshTerrainSettings );
      break;
    }
  }

  if ( terrainSettings )
  {
    // set common terrain settings
    terrainSettings->setVerticalScale( spinTerrainScale->value() );
    terrainSettings->setMapTileResolution( spinMapResolution->value() );
    terrainSettings->setMaximumScreenError( spinScreenError->value() );
    terrainSettings->setMaximumGroundError( spinGroundError->value() );
    terrainSettings->setElevationOffset( terrainElevationOffsetSpinBox->value() );
    mMap->setTerrainSettings( terrainSettings.release() );
  }

  mMap->setFieldOfView( spinCameraFieldOfView->value() );
  mMap->setProjectionType( cboCameraProjectionType->currentData().value<Qt3DRender::QCameraLens::ProjectionType>() );
  mMap->setCameraNavigationMode( mCameraNavigationModeCombo->currentData().value<Qgis::NavigationMode>() );
  mMap->setCameraMovementSpeed( mCameraMovementSpeed->value() );
  mMap->setTerrainVerticalScale( spinTerrainScale->value() );
  mMap->setMapTileResolution( spinMapResolution->value() );
  mMap->setMaxTerrainScreenError( spinScreenError->value() );
  mMap->setMaxTerrainGroundError( spinGroundError->value() );
  mMap->setTerrainElevationOffset( terrainElevationOffsetSpinBox->value() );
  mMap->setShowLabels( chkShowLabels->isChecked() );
  mMap->setShowTerrainTilesInfo( chkShowTileInfo->isChecked() );
  mMap->setShowTerrainBoundingBoxes( chkShowBoundingBoxes->isChecked() );
  mMap->setShowCameraViewCenter( chkShowCameraViewCenter->isChecked() );
  mMap->setShowCameraRotationCenter( chkShowCameraRotationCenter->isChecked() );
  mMap->setShowLightSourceOrigins( chkShowLightSourceOrigins->isChecked() );
  mMap->setIsFpsCounterEnabled( mFpsCounterCheckBox->isChecked() );
  mMap->setTerrainShadingEnabled( groupTerrainShading->isChecked() );
  mMap->setIsDebugOverlayEnabled( mDebugOverlayCheckBox->isChecked() );

  const std::unique_ptr<QgsAbstractMaterialSettings> terrainMaterial( widgetTerrainMaterial->settings() );
  if ( QgsPhongMaterialSettings *phongMaterial = dynamic_cast<QgsPhongMaterialSettings *>( terrainMaterial.get() ) )
    mMap->setTerrainShadingMaterial( *phongMaterial );

  mMap->setLightSources( widgetLights->lightSources() );
  mMap->setIsSkyboxEnabled( groupSkyboxSettings->isChecked() );
  mMap->setSkyboxSettings( mSkyboxSettingsWidget->toSkyboxSettings() );
  QgsShadowSettings shadowSettings = mShadowSettingsWidget->toShadowSettings();
  shadowSettings.setRenderShadows( groupShadowRendering->isChecked() );
  mMap->setShadowSettings( shadowSettings );

  mMap->setEyeDomeLightingEnabled( edlGroupBox->isChecked() );
  mMap->setEyeDomeLightingStrength( edlStrengthSpinBox->value() );
  mMap->setEyeDomeLightingDistance( edlDistanceSpinBox->value() );

  mMap->setAmbientOcclusionSettings( mAmbientOcclusionSettingsWidget->toAmbientOcclusionSettings() );

  Qgis::ViewSyncModeFlags viewSyncMode;
  viewSyncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, mSync2DTo3DCheckbox->isChecked() );
  viewSyncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, mSync3DTo2DCheckbox->isChecked() );
  mMap->setViewSyncMode( viewSyncMode );
  mMap->setViewFrustumVisualizationEnabled( mVisualizeExtentCheckBox->isChecked() );

  mMap->setDebugDepthMapSettings( mDebugDepthMapGroupBox->isChecked(), static_cast<Qt::Corner>( mDebugDepthMapCornerComboBox->currentIndex() ), mDebugDepthMapSizeSpinBox->value() );

  // Do not display the shadow debug map if the shadow effect is not enabled.
  mMap->setDebugShadowMapSettings( mDebugShadowMapGroupBox->isChecked() && groupShadowRendering->isChecked(), static_cast<Qt::Corner>( mDebugShadowMapCornerComboBox->currentIndex() ), mDebugShadowMapSizeSpinBox->value() );
}

void Kadas3DMapConfigWidget::onTerrainTypeChanged()
{
  const QgsTerrainGenerator::Type genType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );

  labelTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh ) );
  spinTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh ) );
  labelTerrainScale->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::QuantizedMesh ) );
  spinTerrainScale->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::QuantizedMesh ) );
  terrainElevationOffsetSpinBox->setVisible( genType != QgsTerrainGenerator::QuantizedMesh );
  labelterrainElevationOffset->setVisible( genType != QgsTerrainGenerator::QuantizedMesh );
  labelTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh ) );
  spinTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh ) );
  labelTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh );
  cboTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem || genType == QgsTerrainGenerator::Mesh || genType == QgsTerrainGenerator::QuantizedMesh );
  groupMeshTerrainShading->setVisible( genType == QgsTerrainGenerator::Mesh );
  groupTerrainShading->setVisible( genType != QgsTerrainGenerator::Mesh );

  QgsMapLayer *oldTerrainLayer = cboTerrainLayer->currentLayer();
  if ( cboTerrainType->currentData() == QgsTerrainGenerator::Dem )
  {
    cboTerrainLayer->setFilters( Qgis::LayerFilter::RasterLayer );
  }
  else if ( cboTerrainType->currentData() == QgsTerrainGenerator::Mesh )
  {
    cboTerrainLayer->setFilters( Qgis::LayerFilter::MeshLayer );
  }
  else if ( cboTerrainType->currentData() == QgsTerrainGenerator::QuantizedMesh )
  {
    cboTerrainLayer->setFilters( Qgis::LayerFilter::TiledSceneLayer );
  }

  if ( cboTerrainLayer->currentLayer() != oldTerrainLayer )
    onTerrainLayerChanged();

  updateMaxZoomLevel();
  validate();
}

void Kadas3DMapConfigWidget::onTerrainLayerChanged()
{
  updateMaxZoomLevel();

  if ( cboTerrainType->currentData() == QgsTerrainGenerator::Mesh )
  {
    QgsMeshLayer *meshLayer = qobject_cast<QgsMeshLayer *>( cboTerrainLayer->currentLayer() );
    if ( meshLayer )
    {
      QgsMeshLayer *oldLayer = mMeshSymbolWidget->meshLayer();

      mMeshSymbolWidget->setLayer( meshLayer, false );
      if ( oldLayer != meshLayer )
        mMeshSymbolWidget->reloadColorRampShaderMinMax();
    }
  }
  validate();
}

void Kadas3DMapConfigWidget::updateMaxZoomLevel()
{
  const QgsRectangle te = groupExtent->outputExtent();

  const double tile0width = std::max( te.width(), te.height() );
  const int zoomLevel = Qgs3DUtils::maxZoomLevel( tile0width, spinMapResolution->value(), spinGroundError->value() );
  labelZoomLevels->setText( QStringLiteral( "0 - %1" ).arg( zoomLevel ) );
}

void Kadas3DMapConfigWidget::validate()
{
  mMessageBar->clearWidgets();

  bool valid = true;
  switch ( static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() ) )
  {
    case QgsTerrainGenerator::Dem:
      if ( !cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a DEM terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::Mesh:
      if ( !cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a mesh terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::QuantizedMesh:
      if ( !cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a quantized mesh terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::Online:
    case QgsTerrainGenerator::Flat:
      break;
  }

  if ( valid && widgetLights->lightSourceCount() == 0 )
  {
    mMessageBar->pushMessage( tr( "No lights exist in the scene" ), Qgis::MessageLevel::Warning );
  }

  emit isValidChanged( valid );
}

void Kadas3DMapConfigWidget::init3DAxisPage()
{
  connect( mGroupBox3dAxis, &QGroupBox::toggled, this, &Kadas3DMapConfigWidget::on3DAxisChanged );
  connect( mCbo3dAxisType, qOverload<int>( &QComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::on3DAxisChanged );
  connect( mCbo3dAxisHorizPos, qOverload<int>( &QComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::on3DAxisChanged );
  connect( mCbo3dAxisVertPos, qOverload<int>( &QComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::on3DAxisChanged );

  Qgs3DAxisSettings s = mMap->get3DAxisSettings();

  if ( s.mode() == Qgs3DAxisSettings::Mode::Off )
    mGroupBox3dAxis->setChecked( false );
  else
  {
    mGroupBox3dAxis->setChecked( true );
    mCbo3dAxisType->setCurrentIndex( mCbo3dAxisType->findData( static_cast<int>( s.mode() ) ) );
  }

  mCbo3dAxisHorizPos->setCurrentIndex( mCbo3dAxisHorizPos->findData( static_cast<int>( s.horizontalPosition() ) ) );
  mCbo3dAxisVertPos->setCurrentIndex( mCbo3dAxisVertPos->findData( static_cast<int>( s.verticalPosition() ) ) );
}

void Kadas3DMapConfigWidget::on3DAxisChanged()
{
  Qgs3DAxisSettings s = mMap->get3DAxisSettings();
  Qgs3DAxisSettings::Mode m;

  if ( mGroupBox3dAxis->isChecked() )
    m = static_cast<Qgs3DAxisSettings::Mode>( mCbo3dAxisType->currentData().toInt() );
  else
    m = Qgs3DAxisSettings::Mode::Off;

  if ( s.mode() != m )
  {
    s.setMode( m );
  }
  else
  {
    const Qt::AnchorPoint hPos = static_cast<Qt::AnchorPoint>( mCbo3dAxisHorizPos->currentData().toInt() );
    const Qt::AnchorPoint vPos = static_cast<Qt::AnchorPoint>( mCbo3dAxisVertPos->currentData().toInt() );

    if ( s.horizontalPosition() != hPos || s.verticalPosition() != vPos )
    {
      s.setHorizontalPosition( hPos );
      s.setVerticalPosition( vPos );
    }
  }

  mMap->set3DAxisSettings( s );
}
