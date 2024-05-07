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

#include "kadas/app/3d/kadas3dmapconfigwidget.h"
#include "kadas/app/3d/kadas3dmapcanvas.h"
#include "kadas/app/3d/kadasskyboxrenderingsettingswidget.h"
#include "kadas/app/3d/kadasshadowrenderingsettingswidget.h"
#include "kadas/app/3d/kadasambientocclusionsettingswidget.h"

#include <qgs3dmapsettings.h>
#include <qgsdemterraingenerator.h>
#include <qgsflatterraingenerator.h>
#include <qgsonlineterraingenerator.h>
#include <qgsmeshterraingenerator.h>
#include <qgs3dutils.h>
#include <qgsguiutils.h>
#include <qgsmapcanvas.h>
#include <qgsrasterlayer.h>
#include <qgsmeshlayer.h>
#include <qgsproject.h>
#include <qgssettings.h>

Kadas3DMapConfigWidget::Kadas3DMapConfigWidget( Qgs3DMapSettings *map, QgsMapCanvas *mainCanvas, Kadas3DMapCanvas *mapCanvas3D, QWidget *parent )
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
  m3DOptionsListWidget->setIconSize( QSize( iconSize, iconSize ) ) ;

  mCameraNavigationModeCombo->addItem( tr( "Terrain Based" ), QVariant::fromValue( Qgis::NavigationMode::TerrainBased ) );
  mCameraNavigationModeCombo->addItem( tr( "Walk Mode (First Person)" ), QVariant::fromValue( Qgis::NavigationMode::Walk ) );

  // get rid of annoying outer focus rect on Mac
  m3DOptionsListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );
  m3DOptionsListWidget->setCurrentRow( settings.value( QStringLiteral( "Windows/3DMapConfig/Tab" ), 0 ).toInt() );
  connect( m3DOptionsListWidget, &QListWidget::currentRowChanged, this, [ = ]( int index ) { m3DOptionsStackedWidget->setCurrentIndex( index ); } );
  m3DOptionsStackedWidget->setCurrentIndex( m3DOptionsListWidget->currentRow() );

  if ( !settings.contains( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ) )
  {
    // set left list widget width on initial showing
    QList<int> splitsizes;
    splitsizes << 115;
    m3DOptionsSplitter->setSizes( splitsizes );
  }
  m3DOptionsSplitter->restoreState( settings.value( QStringLiteral( "Windows/3DMapConfig/OptionsSplitState" ) ).toByteArray() );
  cboCameraProjectionType->addItem( tr( "Perspective Projection" ), Qt3DRender::QCameraLens::PerspectiveProjection );
  cboCameraProjectionType->addItem( tr( "Orthogonal Projection" ), Qt3DRender::QCameraLens::OrthographicProjection );
  connect( cboCameraProjectionType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, [ = ]()
  {
    spinCameraFieldOfView->setEnabled( cboCameraProjectionType->currentIndex() == cboCameraProjectionType->findData( Qt3DRender::QCameraLens::PerspectiveProjection ) );
  } );

  mCameraMovementSpeed->setClearValue( 4 );
  spinCameraFieldOfView->setClearValue( 45.0 );
  spinTerrainScale->setClearValue( 1.0 );
  spinTerrainResolution->setClearValue( 16 );
  spinTerrainSkirtHeight->setClearValue( 10 );
  spinMapResolution->setClearValue( 512 );
  spinScreenError->setClearValue( 3 );
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

  groupTerrain->setChecked( mMap->terrainRenderingEnabled() );

  QgsTerrainGenerator *terrainGen = mMap->terrainGenerator();
  if ( terrainGen && terrainGen->type() == QgsTerrainGenerator::Dem )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Dem ) );
    QgsDemTerrainGenerator *demTerrainGen = static_cast<QgsDemTerrainGenerator *>( terrainGen );
    spinTerrainResolution->setValue( demTerrainGen->resolution() );
    spinTerrainSkirtHeight->setValue( demTerrainGen->skirtHeight() );
    cboTerrainLayer->setLayer( demTerrainGen->layer() );
    cboTerrainLayer->setFilters( Qgis::LayerFilter::RasterLayer );
  }
  else if ( terrainGen && terrainGen->type() == QgsTerrainGenerator::Online )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Online ) );
    QgsOnlineTerrainGenerator *onlineTerrainGen = static_cast<QgsOnlineTerrainGenerator *>( terrainGen );
    spinTerrainResolution->setValue( onlineTerrainGen->resolution() );
    spinTerrainSkirtHeight->setValue( onlineTerrainGen->skirtHeight() );
  }
  else if ( terrainGen )
  {
    cboTerrainType->setCurrentIndex( cboTerrainType->findData( QgsTerrainGenerator::Flat ) );
    cboTerrainLayer->setLayer( nullptr );
    spinTerrainResolution->setValue( 16 );
    spinTerrainSkirtHeight->setValue( 10 );
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  mDebugOverlayCheckBox->setVisible( true );
#endif

  groupTerrainShading->setChecked( mMap->isTerrainShadingEnabled() );
  widgetTerrainMaterial->setTechnique( QgsMaterialSettingsRenderingTechnique::TrianglesWithFixedTexture );
  QgsPhongMaterialSettings terrainShadingMaterial = mMap->terrainShadingMaterial();
  widgetTerrainMaterial->setSettings( &terrainShadingMaterial, nullptr );

  widgetLights->setLights( mMap->lightSources() );

  connect( cboTerrainType, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::onTerrainTypeChanged );
  connect( cboTerrainLayer, static_cast<void ( QComboBox::* )( int )>( &QgsMapLayerComboBox::currentIndexChanged ), this, &Kadas3DMapConfigWidget::onTerrainLayerChanged );
  connect( spinMapResolution, static_cast<void ( QSpinBox::* )( int )>( &QSpinBox::valueChanged ), this, &Kadas3DMapConfigWidget::updateMaxZoomLevel );
  connect( spinGroundError, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, &Kadas3DMapConfigWidget::updateMaxZoomLevel );

  // ==================
  // Page: Skybox
  mSkyboxSettingsWidget = new QgsSkyboxRenderingSettingsWidget( this );
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
  mCbo3dAxisType->addItem( tr( "Coordinate Reference System" ), static_cast< int >( Qgs3DAxisSettings::Mode::Crs ) );
  mCbo3dAxisType->addItem( tr( "Cube" ), static_cast< int >( Qgs3DAxisSettings::Mode::Cube ) );

  mCbo3dAxisHorizPos->addItem( tr( "Left" ), static_cast< int >( Qt::AnchorPoint::AnchorLeft ) );
  mCbo3dAxisHorizPos->addItem( tr( "Center" ), static_cast< int >( Qt::AnchorPoint::AnchorHorizontalCenter ) );
  mCbo3dAxisHorizPos->addItem( tr( "Right" ), static_cast< int >( Qt::AnchorPoint::AnchorRight ) );

  mCbo3dAxisVertPos->addItem( tr( "Top" ), static_cast< int >( Qt::AnchorPoint::AnchorTop ) );
  mCbo3dAxisVertPos->addItem( tr( "Middle" ), static_cast< int >( Qt::AnchorPoint::AnchorVerticalCenter ) );
  mCbo3dAxisVertPos->addItem( tr( "Bottom" ), static_cast< int >( Qt::AnchorPoint::AnchorBottom ) );

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
  switch ( terrainType )
  {
    case QgsTerrainGenerator::Flat:
    {
      QgsFlatTerrainGenerator *flatTerrainGen = new QgsFlatTerrainGenerator;
      flatTerrainGen->setCrs( mMap->crs() );
      mMap->setTerrainGenerator( flatTerrainGen );
    }
    break;
    case QgsTerrainGenerator::Dem:
    {
      QgsRasterLayer *demLayer = qobject_cast<QgsRasterLayer *>( cboTerrainLayer->currentLayer() );

      bool tGenNeedsUpdate = true;
      if ( mMap->terrainGenerator()->type() == QgsTerrainGenerator::Dem )
      {
        // if we already have a DEM terrain generator, check whether there was actually any change
        QgsDemTerrainGenerator *oldDemTerrainGen = static_cast<QgsDemTerrainGenerator *>( mMap->terrainGenerator() );
        if ( oldDemTerrainGen->layer() == demLayer &&
             oldDemTerrainGen->resolution() == spinTerrainResolution->value() &&
             oldDemTerrainGen->skirtHeight() == spinTerrainSkirtHeight->value() )
          tGenNeedsUpdate = false;
      }

      if ( tGenNeedsUpdate )
      {
        QgsDemTerrainGenerator *demTerrainGen = new QgsDemTerrainGenerator;
        demTerrainGen->setCrs( mMap->crs(), QgsProject::instance()->transformContext() );
        demTerrainGen->setLayer( demLayer );
        demTerrainGen->setResolution( spinTerrainResolution->value() );
        demTerrainGen->setSkirtHeight( spinTerrainSkirtHeight->value() );
        mMap->setTerrainGenerator( demTerrainGen );
      }
    }
    break;
    case QgsTerrainGenerator::Online:
    {
      bool tGenNeedsUpdate = true;
      if ( mMap->terrainGenerator()->type() == QgsTerrainGenerator::Online )
      {
        QgsOnlineTerrainGenerator *oldOnlineTerrainGen = static_cast<QgsOnlineTerrainGenerator *>( mMap->terrainGenerator() );
        if ( oldOnlineTerrainGen->resolution() == spinTerrainResolution->value() &&
             oldOnlineTerrainGen->skirtHeight() == spinTerrainSkirtHeight->value() )
          tGenNeedsUpdate = false;
      }

      if ( tGenNeedsUpdate )
      {
        QgsOnlineTerrainGenerator *onlineTerrainGen = new QgsOnlineTerrainGenerator;
        onlineTerrainGen->setCrs( mMap->crs(), QgsProject::instance()->transformContext() );
        onlineTerrainGen->setResolution( spinTerrainResolution->value() );
        onlineTerrainGen->setSkirtHeight( spinTerrainSkirtHeight->value() );
        mMap->setTerrainGenerator( onlineTerrainGen );
      }
    }
    break;
    break;
  }

  mMap->setFieldOfView( spinCameraFieldOfView->value() );
  mMap->setProjectionType( cboCameraProjectionType->currentData().value< Qt3DRender::QCameraLens::ProjectionType >() );
  mMap->setCameraNavigationMode( mCameraNavigationModeCombo->currentData().value< Qgis::NavigationMode>() );
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

  const std::unique_ptr< QgsAbstractMaterialSettings > terrainMaterial( widgetTerrainMaterial->settings() );
  if ( QgsPhongMaterialSettings *phongMaterial = dynamic_cast< QgsPhongMaterialSettings * >( terrainMaterial.get() ) )
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
  mMap->setDebugShadowMapSettings( mDebugShadowMapGroupBox->isChecked(), static_cast<Qt::Corner>( mDebugShadowMapCornerComboBox->currentIndex() ), mDebugShadowMapSizeSpinBox->value() );
}

void Kadas3DMapConfigWidget::onTerrainTypeChanged()
{
  const QgsTerrainGenerator::Type genType = static_cast<QgsTerrainGenerator::Type>( cboTerrainType->currentData().toInt() );

  labelTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat ) );
  spinTerrainResolution->setVisible( !( genType == QgsTerrainGenerator::Flat ) );
  labelTerrainScale->setVisible( genType != QgsTerrainGenerator::Flat );
  spinTerrainScale->setVisible( genType != QgsTerrainGenerator::Flat );
  labelTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat ) );
  spinTerrainSkirtHeight->setVisible( !( genType == QgsTerrainGenerator::Flat ) );
  labelTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem );
  cboTerrainLayer->setVisible( genType == QgsTerrainGenerator::Dem );

  QgsMapLayer *oldTerrainLayer = cboTerrainLayer->currentLayer();
  if ( cboTerrainType->currentData() == QgsTerrainGenerator::Dem )
  {
    cboTerrainLayer->setFilters( Qgis::LayerFilter::RasterLayer );
  }

  if ( cboTerrainLayer->currentLayer() != oldTerrainLayer )
    onTerrainLayerChanged();

  updateMaxZoomLevel();
  validate();
}

void Kadas3DMapConfigWidget::onTerrainLayerChanged()
{
  updateMaxZoomLevel();
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
      if ( ! cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a DEM terrain" ), Qgis::MessageLevel::Critical );
      }
      break;

    case QgsTerrainGenerator::Mesh:
      if ( ! cboTerrainLayer->currentLayer() )
      {
        valid = false;
        mMessageBar->pushMessage( tr( "An elevation layer must be selected for a mesh terrain" ), Qgis::MessageLevel::Critical );
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
    mCbo3dAxisType->setCurrentIndex( mCbo3dAxisType->findData( static_cast< int >( s.mode() ) ) );
  }

  mCbo3dAxisHorizPos->setCurrentIndex( mCbo3dAxisHorizPos->findData( static_cast< int >( s.horizontalPosition() ) ) );
  mCbo3dAxisVertPos->setCurrentIndex( mCbo3dAxisVertPos->findData( static_cast< int >( s.verticalPosition() ) ) );
}

void Kadas3DMapConfigWidget::on3DAxisChanged()
{
  Qgs3DAxisSettings s = mMap->get3DAxisSettings();
  Qgs3DAxisSettings::Mode m;

  if ( mGroupBox3dAxis->isChecked() )
    m = static_cast< Qgs3DAxisSettings::Mode >( mCbo3dAxisType->currentData().toInt() );
  else
    m = Qgs3DAxisSettings::Mode::Off;

  if ( s.mode() != m )
  {
    s.setMode( m );
  }
  else
  {
    const Qt::AnchorPoint hPos = static_cast< Qt::AnchorPoint >( mCbo3dAxisHorizPos->currentData().toInt() );
    const Qt::AnchorPoint vPos = static_cast< Qt::AnchorPoint >( mCbo3dAxisVertPos->currentData().toInt() );

    if ( s.horizontalPosition() != hPos || s.verticalPosition() != vPos )
    {
      s.setHorizontalPosition( hPos );
      s.setVerticalPosition( vPos );
    }
  }

  mMap->set3DAxisSettings( s );
}
