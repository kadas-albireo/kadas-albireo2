/***************************************************************************
  qgs3dmapcanvaswidget.cpp
  --------------------------------------
  Date                 : January 2022
  Copyright            : (C) 2022 by Belgacem Nedjima
  Email                : belgacem dot nedjima at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadas3dmapcanvaswidget.h"

#include <QAction>
#include <QActionGroup>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>
#include <QWidget>

#include "qgs3dmapcanvas.h"
#include "qgs3dmapscene.h"
#include "qgscameracontroller.h"
#include "qgshelp.h"
#include "qgslayertree.h"
#include "qgsmapcanvas.h"
#include "qgsmaptoolextent.h"
#include "qgsmessagebar.h"
#include "qgsapplication.h"
#include "qgssettings.h"
#include "qgsgui.h"
#include "qgsmapthemecollection.h"
#include "qgsshortcutsmanager.h"
#include "qgsrubberband.h"


// #include "qgs3danimationsettings.h"
// #include "qgs3danimationwidget.h"
#include "qgs3dmapsettings.h"
//#include "qgs3dmaptoolidentify.h"
//#include "qgs3dmaptoolmeasureline.h"
//#include "qgs3dnavigationwidget.h"
#include "qgs3dutils.h"
#include "qgswindow3dengine.h"

#include "kadas/app/3d/kadas3dmapconfigwidget.h"
#include "kadas/app/3d/kadas3dnavigationwidget.h"
#include "kadas/app/3d/kadas3dlayertreemodel.h"

//#include "qgsmap3dexportwidget.h"
//#include "qgs3dmapexportsettings.h"


Kadas3DMapCanvasWidget::Kadas3DMapCanvasWidget( const QString &name, bool isDocked )
  : QWidget( nullptr )
  , mCanvasName( name )
{
  const QgsSettings setting;

  QToolBar *toolBar = new QToolBar( this );
  // toolBar->setIconSize( QgisApp::instance()->iconSize( true ) );

  QAction *actionCameraControl = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionPan.svg" ) ), tr( "Camera Control" ), this, &Kadas3DMapCanvasWidget::cameraControl );
  actionCameraControl->setCheckable( true );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionZoomFullExtent.svg" ) ), tr( "Zoom Full" ), this, &Kadas3DMapCanvasWidget::resetView );

  QAction *toggleOnScreenNavigation = toolBar->addAction(
    QgsApplication::getThemeIcon( QStringLiteral( "mAction3DNavigation.svg" ) ),
    tr( "Toggle On-Screen Navigation" )
  );

  toggleOnScreenNavigation->setCheckable( true );
  toggleOnScreenNavigation->setChecked( sSettingNavigationVisible->value() );
  QObject::connect( toggleOnScreenNavigation, &QAction::toggled, this, &Kadas3DMapCanvasWidget::toggleNavigationWidget );

  toolBar->addSeparator();

#if 0
  QAction *actionIdentify = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionIdentify.svg" ) ),
                            tr( "Identify" ), this, &Kadas3DMapCanvasWidget::identify );
  actionIdentify->setCheckable( true );

  QAction *actionMeasurementTool = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionMeasure.svg" ) ),
                                   tr( "Measurement Line" ), this, &Kadas3DMapCanvasWidget::measureLine );
  actionMeasurementTool->setCheckable( true );
#endif

  // Create action group to make the action exclusive
  QActionGroup *actionGroup = new QActionGroup( this );
  actionGroup->addAction( actionCameraControl );
  //actionGroup->addAction( actionIdentify );
  // actionGroup->addAction( actionMeasurementTool );
  actionGroup->setExclusive( true );
  actionCameraControl->setChecked( true );
#if 0
  mActionAnim = toolBar->addAction( QIcon( QgsApplication::iconPath( "mTaskRunning.svg" ) ),
                                    tr( "Animations" ), this, &Kadas3DMapCanvasWidget::toggleAnimations );
  mActionAnim->setCheckable( true );
#endif

  // Export Menu
  mExportMenu = new QMenu( this );

  mActionExport = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionSharingExport.svg" ) ), tr( "Export" ), this );
  mActionExport->setMenu( mExportMenu );
  toolBar->addAction( mActionExport );
  QToolButton *exportButton = qobject_cast<QToolButton *>( toolBar->widgetForAction( mActionExport ) );
  exportButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  mExportMenu->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionSaveMapAsImage.svg" ) ), tr( "Save as Image…" ), this, &Kadas3DMapCanvasWidget::saveAsImage );

#if 0
  mExportMenu->addAction( QgsApplication::getThemeIcon( QStringLiteral( "3d.svg" ) ),
                          tr( "Export 3D Scene" ), this, &Kadas3DMapCanvasWidget::exportScene );
#endif

  toolBar->addSeparator();

  QAction *toggleLayerTree = toolBar->addAction(
    QgsApplication::getThemeIcon( QStringLiteral( "/mActionShowAllLayers.svg" ) ),
    tr( "Toggle Layer Tree" )
  );
  toggleLayerTree->setCheckable( true );
  toggleLayerTree->setChecked( sSettingLayerTreeVisible->value() );
  connect( toggleLayerTree, &QAction::toggled, this, &Kadas3DMapCanvasWidget::toggleLayerTreeWidget );

  toolBar->addSeparator();

  // Camera Menu
  mCameraMenu = new QMenu( this );
  mActionCamera = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mIconCamera.svg" ) ), tr( "Camera" ), this );
  mActionCamera->setMenu( mCameraMenu );
  toolBar->addAction( mActionCamera );
  QToolButton *cameraButton = qobject_cast<QToolButton *>( toolBar->widgetForAction( mActionCamera ) );
  cameraButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  mActionSync2DNavTo3D = new QAction( tr( "2D Map View Follows 3D Camera" ), this );
  mActionSync2DNavTo3D->setCheckable( true );
  connect( mActionSync2DNavTo3D, &QAction::triggered, this, [=]( bool enabled ) {
    mActionSync3DNavTo2D->setChecked( false );
    Qgis::ViewSyncModeFlags syncMode = mCanvas->mapSettings()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, enabled );
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, false );
    mCanvas->mapSettings()->setViewSyncMode( syncMode );
  } );
  mCameraMenu->addAction( mActionSync2DNavTo3D );

  mActionSync3DNavTo2D = new QAction( tr( "3D Camera Follows 2D Map View" ), this );
  mActionSync3DNavTo2D->setCheckable( true );
  connect( mActionSync3DNavTo2D, &QAction::triggered, this, [=]( bool enabled ) {
    mActionSync2DNavTo3D->setChecked( false );
    Qgis::ViewSyncModeFlags syncMode = mCanvas->mapSettings()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, enabled );
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, false );
    mCanvas->mapSettings()->setViewSyncMode( syncMode );
  } );
  mCameraMenu->addAction( mActionSync3DNavTo2D );

  mShowFrustumPolyogon = new QAction( tr( "Show Visible Camera Area in 2D Map View" ), this );
  mShowFrustumPolyogon->setCheckable( true );
  connect( mShowFrustumPolyogon, &QAction::triggered, this, [=]( bool enabled ) {
    mCanvas->mapSettings()->setViewFrustumVisualizationEnabled( enabled );
  } );
  mCameraMenu->addAction( mShowFrustumPolyogon );
#if 0
  mActionSetSceneExtent = mCameraMenu->addAction( QgsApplication::getThemeIcon( QStringLiteral( "extents.svg" ) ),
                          tr( "Set 3D Scene Extent on 2D Map View" ), this, &Kadas3DMapCanvasWidget::setSceneExtentOn2DCanvas );
  mActionSetSceneExtent->setCheckable( true );
#endif
  auto createShortcuts = [=]( const QString &objectName, void ( Kadas3DMapCanvasWidget::*slot )() ) {
    if ( QShortcut *sc = QgsGui::shortcutsManager()->shortcutByName( objectName ) )
      connect( sc, &QShortcut::activated, this, slot );
  };
  //createShortcuts( QStringLiteral( "m3DSetSceneExtent" ), &Kadas3DMapCanvasWidget::setSceneExtentOn2DCanvas );

  // Effects Menu
  mEffectsMenu = new QMenu( this );

  mActionEffects = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mIconShadow.svg" ) ), tr( "Effects" ), this );
  mActionEffects->setMenu( mEffectsMenu );
  toolBar->addAction( mActionEffects );
  QToolButton *effectsButton = qobject_cast<QToolButton *>( toolBar->widgetForAction( mActionEffects ) );
  effectsButton->setPopupMode( QToolButton::ToolButtonPopupMode::InstantPopup );

  mActionEnableShadows = new QAction( tr( "Show Shadows" ), this );
  mActionEnableShadows->setCheckable( true );
  connect( mActionEnableShadows, &QAction::toggled, this, [=]( bool enabled ) {
    QgsShadowSettings settings = mCanvas->mapSettings()->shadowSettings();
    settings.setRenderShadows( enabled );
    mCanvas->mapSettings()->setShadowSettings( settings );
  } );
  mEffectsMenu->addAction( mActionEnableShadows );

  mActionEnableEyeDome = new QAction( tr( "Show Eye Dome Lighting" ), this );
  mActionEnableEyeDome->setCheckable( true );
  connect( mActionEnableEyeDome, &QAction::triggered, this, [=]( bool enabled ) {
    mCanvas->mapSettings()->setEyeDomeLightingEnabled( enabled );
  } );
  mEffectsMenu->addAction( mActionEnableEyeDome );

  mActionEnableAmbientOcclusion = new QAction( tr( "Show Ambient Occlusion" ), this );
  mActionEnableAmbientOcclusion->setCheckable( true );
  connect( mActionEnableAmbientOcclusion, &QAction::triggered, this, [=]( bool enabled ) {
    QgsAmbientOcclusionSettings ambientOcclusionSettings = mCanvas->mapSettings()->ambientOcclusionSettings();
    ambientOcclusionSettings.setEnabled( enabled );
    mCanvas->mapSettings()->setAmbientOcclusionSettings( ambientOcclusionSettings );
  } );
  mEffectsMenu->addAction( mActionEnableAmbientOcclusion );

  // Options Menu
  QAction *configureAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionOptions.svg" ) ), tr( "Configure…" ), this );
  connect( configureAction, &QAction::triggered, this, &Kadas3DMapCanvasWidget::configure );
  toolBar->addAction( configureAction );

  mCanvas = new Qgs3DMapCanvas;
  mCanvas->setMinimumSize( QSize( 200, 200 ) );

#if 0
  connect( mCanvas, &Qgs3DMapCanvas::savedAsImage, this, [ = ]( const QString & fileName )
  {
    QgisApp::instance()->messageBar()->pushSuccess( tr( "Save as Image" ), tr( "Successfully saved the 3D map to <a href=\"%1\">%2</a>" ).arg( QUrl::fromLocalFile( fileName ).toString(), QDir::toNativeSeparators( fileName ) ) );
  } );
#endif

  connect( mCanvas, &Qgs3DMapCanvas::fpsCountChanged, this, &Kadas3DMapCanvasWidget::updateFpsCount );
  connect( mCanvas, &Qgs3DMapCanvas::fpsCounterEnabledChanged, this, &Kadas3DMapCanvasWidget::toggleFpsCounter );
  connect( mCanvas, &Qgs3DMapCanvas::cameraNavigationSpeedChanged, this, &Kadas3DMapCanvasWidget::cameraNavigationSpeedChanged );
  connect( mCanvas, &Qgs3DMapCanvas::viewed2DExtentFrom3DChanged, this, &Kadas3DMapCanvasWidget::onViewed2DExtentFrom3DChanged );

  // mMapToolIdentify = new Qgs3DMapToolIdentify( mCanvas );

  // mMapToolMeasureLine = new Qgs3DMapToolMeasureLine( mCanvas );

  mLabelPendingJobs = new QLabel( this );
  mProgressPendingJobs = new QProgressBar( this );
  mProgressPendingJobs->setRange( 0, 0 );
  mLabelFpsCounter = new QLabel( this );
  mLabelNavigationSpeed = new QLabel( this );

#if 0
  mAnimationWidget = new Qgs3DAnimationWidget( this );
  mAnimationWidget->setVisible( false );
#endif

  mMessageBar = new QgsMessageBar( this );
  mMessageBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->setContentsMargins( 0, 0, 0, 0 );
  topLayout->setSpacing( style()->pixelMetric( QStyle::PM_LayoutHorizontalSpacing ) );
  topLayout->addWidget( toolBar );
  topLayout->addStretch( 1 );
  topLayout->addWidget( mLabelPendingJobs );
  topLayout->addWidget( mProgressPendingJobs );
  topLayout->addWidget( mLabelNavigationSpeed );
  mLabelNavigationSpeed->hide();
  topLayout->addWidget( mLabelFpsCounter );

  mLabelNavSpeedHideTimeout = new QTimer( this );
  mLabelNavSpeedHideTimeout->setInterval( 1000 );
  connect( mLabelNavSpeedHideTimeout, &QTimer::timeout, this, [=] {
    mLabelNavigationSpeed->hide();
    mLabelNavSpeedHideTimeout->stop();
  } );

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 0 );
  layout->addLayout( topLayout );
  layout->addWidget( mMessageBar );

  // mContainer takes ownership of Qgs3DMapCanvas
  mContainer = QWidget::createWindowContainer( mCanvas, this, Qt::Widget );
  mContainer->lower();
  mContainer->setMinimumSize( QSize( 200, 200 ) );
  mContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

  mNavigationWidget = new Kadas3DNavigationWidget( mCanvas );
  mNavigationWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );

  mLayerTreeView = new QTreeView( this );
  mLayerTreeView->setModel( new Kadas3DLayerTreeModel( mCanvas ) );
  mLayerTreeView->setMinimumWidth( 200 );
  mLayerTreeView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
  mLayerTreeView->setVisible( sSettingLayerTreeVisible->value() );

  QHBoxLayout *hLayout = new QHBoxLayout;
  hLayout->setContentsMargins( 0, 0, 0, 0 );
  hLayout->addWidget( mLayerTreeView );
  hLayout->addWidget( mContainer );
  hLayout->addWidget( mNavigationWidget );
  hLayout->setStretch( 1, 4 );

  toggleNavigationWidget(
    setting.value( QStringLiteral( "/3D/navigationWidget/visibility" ), false, QgsSettings::Gui ).toBool()
  );

  layout->addLayout( hLayout );
  // layout->addWidget( mAnimationWidget );

  setLayout( layout );

  onTotalPendingJobsCountChanged();

  mDockableWidgetHelper = new QgsDockableWidgetHelper( mCanvasName, this, QgsDockableWidgetHelper::sOwnerWindow, QStringLiteral( "3dmapcanvaswidget" ) );
  if ( QDialog *dialog = mDockableWidgetHelper->dialog() )
  {
    QFontMetrics fm( font() );
    const int initialSize = fm.horizontalAdvance( '0' ) * 75;
    dialog->resize( initialSize, initialSize );
  }
  QAction *dockAction = mDockableWidgetHelper->createDockUndockAction( tr( "Dock 3D Map View" ), this );
  toolBar->addAction( dockAction );
}

void Kadas3DMapCanvasWidget::saveAsImage()
{
  const QPair<QString, QString> fileNameAndFilter = QgsGuiUtils::getSaveAsImageName( this, tr( "Choose a file name to save the 3D map canvas to an image" ) );
  if ( !fileNameAndFilter.first.isEmpty() )
  {
    mCanvas->saveAsImage( fileNameAndFilter.first, fileNameAndFilter.second );
  }
}

#if 0
void Kadas3DMapCanvasWidget::toggleAnimations()
{
  if ( mAnimationWidget->isVisible() )
  {
    mAnimationWidget->setVisible( false );
    return;
  }

  mAnimationWidget->setVisible( true );

  // create a dummy animation when first started - better to have something than nothing...
  if ( mAnimationWidget->animation().duration() == 0 )
  {
    mAnimationWidget->setDefaultAnimation();
  }
}
#endif

void Kadas3DMapCanvasWidget::cameraControl()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( nullptr );
}

#if 0
void Kadas3DMapCanvasWidget::identify()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( action->isChecked() ? mMapToolIdentify : nullptr );
}
#endif

#if 0
void Kadas3DMapCanvasWidget::measureLine()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( action->isChecked() ? mMapToolMeasureLine : nullptr );
}
#endif

void Kadas3DMapCanvasWidget::setCanvasName( const QString &name )
{
  mCanvasName = name;
  mDockableWidgetHelper->setWindowTitle( name );
}

void Kadas3DMapCanvasWidget::toggleNavigationWidget( bool visibility )
{
  mNavigationWidget->setVisible( visibility );
  sSettingNavigationVisible->setValue( visibility );
}

void Kadas3DMapCanvasWidget::toggleLayerTreeWidget( bool visibility )
{
  mLayerTreeView->setVisible( visibility );
  sSettingLayerTreeVisible->setValue( visibility );
}

void Kadas3DMapCanvasWidget::toggleFpsCounter( bool visibility )
{
  mLabelFpsCounter->setVisible( visibility );
}

void Kadas3DMapCanvasWidget::setMapSettings( Qgs3DMapSettings *map )
{
  whileBlocking( mActionEnableShadows )->setChecked( map->shadowSettings().renderShadows() );
  whileBlocking( mActionEnableEyeDome )->setChecked( map->eyeDomeLightingEnabled() );
  whileBlocking( mActionEnableAmbientOcclusion )->setChecked( map->ambientOcclusionSettings().isEnabled() );
  whileBlocking( mActionSync2DNavTo3D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  whileBlocking( mActionSync3DNavTo2D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  whileBlocking( mShowFrustumPolyogon )->setChecked( map->viewFrustumVisualizationEnabled() );

  mCanvas->setMapSettings( map );

  // Connect the camera to the navigation widget.
  // connect( mCanvas->cameraController(), &QgsCameraController::cameraChanged, mNavigationWidget, &Kadas3DNavigationWidget::updateFromCamera );
  connect( mCanvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, this, &Kadas3DMapCanvasWidget::onTotalPendingJobsCountChanged );
  connect( mCanvas->scene(), &Qgs3DMapScene::gpuMemoryLimitReached, this, &Kadas3DMapCanvasWidget::onGpuMemoryLimitReached );

  // update the navigation widget when the near/far planes have been updated by the map scene
  // connect( mCanvas->cameraController()->camera(), &Qt3DRender::QCamera::nearPlaneChanged, mNavigationWidget, &Kadas3DNavigationWidget::updateFromCamera );
  // connect( mCanvas->cameraController()->camera(), &Qt3DRender::QCamera::farPlaneChanged, mNavigationWidget, &Kadas3DNavigationWidget::updateFromCamera );

  // mAnimationWidget->setCameraController( mCanvas->cameraController() );
  // mAnimationWidget->setMap( map );

  // Disable button for switching the map theme if the terrain generator is a mesh, or if there is no terrain
  // mActionMapThemes->setDisabled( !mCanvas->mapSettings()->terrainRenderingEnabled() || !mCanvas->mapSettings()->terrainGenerator() || mCanvas->mapSettings()->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  mLabelFpsCounter->setVisible( map->isFpsCounterEnabled() );

  connect( map, &Qgs3DMapSettings::viewFrustumVisualizationEnabledChanged, this, &Kadas3DMapCanvasWidget::onViewFrustumVisualizationEnabledChanged );
  connect( map, &Qgs3DMapSettings::extentChanged, this, &Kadas3DMapCanvasWidget::onExtentChanged );
  connect( map, &Qgs3DMapSettings::showExtentIn2DViewChanged, this, &Kadas3DMapCanvasWidget::onExtentChanged );
  onExtentChanged();
}

void Kadas3DMapCanvasWidget::setMainCanvas( QgsMapCanvas *canvas )
{
  mMainCanvas = canvas;

#if 0
  mMapToolExtent = std::make_unique< QgsMapToolExtent >( canvas );
  mMapToolExtent->setAction( mActionSetSceneExtent );
  connect( mMapToolExtent.get(), &QgsMapToolExtent::extentChanged, this, &Kadas3DMapCanvasWidget::setSceneExtent );
#endif

  connect( mMainCanvas, &QgsMapCanvas::canvasColorChanged, this, &Kadas3DMapCanvasWidget::onMainCanvasColorChanged );
  connect( mMainCanvas, &QgsMapCanvas::extentsChanged, this, &Kadas3DMapCanvasWidget::onMainMapCanvasExtentChanged );

  if ( !mViewFrustumHighlight )
  {
    mViewFrustumHighlight.reset( new QgsRubberBand( canvas, Qgis::GeometryType::Polygon ) );
    mViewFrustumHighlight->setColor( QColor::fromRgba( qRgba( 0, 0, 255, 50 ) ) );
  }

  if ( !mViewExtentHighlight )
  {
    mViewExtentHighlight.reset( new QgsRubberBand( canvas, Qgis::GeometryType::Polygon ) );
    mViewExtentHighlight->setColor( QColor::fromRgba( qRgba( 255, 0, 0, 50 ) ) );
  }
}

void Kadas3DMapCanvasWidget::resetView()
{
  mCanvas->resetView();
}

void Kadas3DMapCanvasWidget::configure()
{
  if ( mConfigureDialog )
  {
    mConfigureDialog->raise();
    return;
  }

  mConfigureDialog = new QDialog( this );
  mConfigureDialog->setAttribute( Qt::WA_DeleteOnClose );
  mConfigureDialog->setWindowTitle( tr( "3D Configuration" ) );
  mConfigureDialog->setObjectName( QStringLiteral( "3DConfigurationDialog" ) );
  mConfigureDialog->setMinimumSize( 600, 460 );
  QgsGui::enableAutoGeometryRestore( mConfigureDialog );

  Qgs3DMapSettings *map = mCanvas->mapSettings();
  Kadas3DMapConfigWidget *w = new Kadas3DMapConfigWidget( map, mMainCanvas, mCanvas, mConfigureDialog );
  QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, mConfigureDialog );

  auto applyConfig = [=]() {
    const QgsVector3D oldOrigin = map->origin();
    const QgsCoordinateReferenceSystem oldCrs = map->crs();
    const QgsCameraPose oldCameraPose = mCanvas->cameraController()->cameraPose();
    const QgsVector3D oldLookingAt = oldCameraPose.centerPoint();

    // update map
    w->apply();

    const QgsVector3D p = Qgs3DUtils::transformWorldCoordinates(
      oldLookingAt,
      oldOrigin, oldCrs,
      map->origin(), map->crs(), QgsProject::instance()->transformContext()
    );

    if ( p != oldLookingAt )
    {
      // apply() call has moved origin of the world so let's move camera so we look still at the same place
      QgsCameraPose newCameraPose = oldCameraPose;
      newCameraPose.setCenterPoint( p );
      mCanvas->cameraController()->setCameraPose( newCameraPose );
    }

    // Disable map theme button if the terrain generator is a mesh, or if there is no terrain
    //mActionMapThemes->setDisabled( !mCanvas->mapSettings()->terrainRenderingEnabled() || !mCanvas->mapSettings()->terrainGenerator() || map->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  };

  connect( buttons, &QDialogButtonBox::rejected, mConfigureDialog, &QDialog::reject );
  connect( buttons, &QDialogButtonBox::clicked, mConfigureDialog, [=]( QAbstractButton *button ) {
    if ( button == buttons->button( QDialogButtonBox::Apply ) || button == buttons->button( QDialogButtonBox::Ok ) )
      applyConfig();
    if ( button == buttons->button( QDialogButtonBox::Ok ) )
      mConfigureDialog->accept();
  } );
  connect( buttons, &QDialogButtonBox::helpRequested, w, []() { QgsHelp::openHelp( QStringLiteral( "map_views/3d_map_view.html#scene-configuration" ) ); } );

  connect( w, &Kadas3DMapConfigWidget::isValidChanged, this, [=]( bool valid ) {
    buttons->button( QDialogButtonBox::Apply )->setEnabled( valid );
    buttons->button( QDialogButtonBox::Ok )->setEnabled( valid );
  } );

  QVBoxLayout *layout = new QVBoxLayout( mConfigureDialog );
  layout->addWidget( w, 1 );
  layout->addWidget( buttons );

  mConfigureDialog->show();

  whileBlocking( mActionEnableShadows )->setChecked( map->shadowSettings().renderShadows() );
  whileBlocking( mActionEnableEyeDome )->setChecked( map->eyeDomeLightingEnabled() );
  whileBlocking( mActionEnableAmbientOcclusion )->setChecked( map->ambientOcclusionSettings().isEnabled() );
  whileBlocking( mActionSync2DNavTo3D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) );
  whileBlocking( mActionSync3DNavTo2D )->setChecked( map->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) );
  whileBlocking( mShowFrustumPolyogon )->setChecked( map->viewFrustumVisualizationEnabled() );
}

#if 0
void Kadas3DMapCanvasWidget::exportScene()
{

  QDialog dlg;
  dlg.setWindowTitle( tr( "Export 3D Scene" ) );
  dlg.setObjectName( QStringLiteral( "3DSceneExportDialog" ) );
  QgsGui::enableAutoGeometryRestore( &dlg );

  Qgs3DMapExportSettings exportSettings;
  QgsMap3DExportWidget w( mCanvas->scene(), &exportSettings );

  QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Cancel | QDialogButtonBox::Help | QDialogButtonBox::Ok, &dlg );

  connect( buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept );
  connect( buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject );
  connect( buttons, &QDialogButtonBox::helpRequested, &dlg, [ = ] { QgsHelp::openHelp( QStringLiteral( "map_views/3d_map_view.html" ) ); } );

  QVBoxLayout *layout = new QVBoxLayout( &dlg );
  layout->addWidget( &w, 1 );
  layout->addWidget( buttons );
  if ( dlg.exec() )
    w.exportScene();
}
#endif

void Kadas3DMapCanvasWidget::onMainCanvasColorChanged()
{
  mCanvas->mapSettings()->setBackgroundColor( mMainCanvas->canvasColor() );
}

void Kadas3DMapCanvasWidget::onTotalPendingJobsCountChanged()
{
  const int count = mCanvas->scene() ? mCanvas->scene()->totalPendingJobsCount() : 0;
  mProgressPendingJobs->setVisible( count );
  mLabelPendingJobs->setVisible( count );
  if ( count )
    mLabelPendingJobs->setText( tr( "Loading %n tile(s)", nullptr, count ) );
}

void Kadas3DMapCanvasWidget::updateFpsCount( float fpsCount )
{
  mLabelFpsCounter->setText( QStringLiteral( "%1 fps" ).arg( fpsCount, 10, 'f', 2, QLatin1Char( ' ' ) ) );
}

void Kadas3DMapCanvasWidget::cameraNavigationSpeedChanged( double speed )
{
  mLabelNavigationSpeed->setText( QStringLiteral( "Speed: %1 ×" ).arg( QString::number( speed, 'f', 2 ) ) );
  mLabelNavigationSpeed->show();
  mLabelNavSpeedHideTimeout->start();
}

void Kadas3DMapCanvasWidget::onMainMapCanvasExtentChanged()
{
  if ( mCanvas->mapSettings()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
  {
    mCanvas->setViewFrom2DExtent( mMainCanvas->extent() );
  }
}

void Kadas3DMapCanvasWidget::onViewed2DExtentFrom3DChanged( QVector<QgsPointXY> extent )
{
  if ( mCanvas->mapSettings()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) )
  {
    QgsRectangle extentRect;
    extentRect.setNull();
    for ( QgsPointXY &pt : extent )
    {
      extentRect.include( pt );
    }
    if ( !extentRect.isEmpty() && extentRect.isFinite() && !extentRect.isNull() )
    {
      if ( mCanvas->mapSettings()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
      {
        whileBlocking( mMainCanvas )->setExtent( extentRect );
      }
      else
      {
        mMainCanvas->setExtent( extentRect );
      }
      mMainCanvas->refresh();
    }
  }

  onViewFrustumVisualizationEnabledChanged();
}

void Kadas3DMapCanvasWidget::onViewFrustumVisualizationEnabledChanged()
{
  mViewFrustumHighlight->reset( Qgis::GeometryType::Polygon );
  if ( mCanvas->mapSettings()->viewFrustumVisualizationEnabled() )
  {
    for ( QgsPointXY &pt : mCanvas->viewFrustum2DExtent() )
    {
      mViewFrustumHighlight->addPoint( pt, false );
    }
    mViewFrustumHighlight->closePoints();
  }
}

void Kadas3DMapCanvasWidget::onExtentChanged()
{
  Qgs3DMapSettings *mapSettings = mCanvas->mapSettings();
  mViewExtentHighlight->reset( Qgis::GeometryType::Polygon );
  if ( mapSettings->showExtentIn2DView() )
  {
    QgsRectangle extent = mapSettings->extent();
    mViewExtentHighlight->addPoint( QgsPointXY( extent.xMinimum(), extent.yMinimum() ), false );
    mViewExtentHighlight->addPoint( QgsPointXY( extent.xMinimum(), extent.yMaximum() ), false );
    mViewExtentHighlight->addPoint( QgsPointXY( extent.xMaximum(), extent.yMaximum() ), false );
    mViewExtentHighlight->addPoint( QgsPointXY( extent.xMaximum(), extent.yMinimum() ), false );
    mViewExtentHighlight->closePoints();
  }
}

void Kadas3DMapCanvasWidget::onGpuMemoryLimitReached()
{
  // let's report this issue just once, rather than spamming user if this happens repeatedly
  if ( mGpuMemoryLimitReachedReported )
    return;

  const QgsSettings settings;
  double memLimit = settings.value( QStringLiteral( "map3d/gpuMemoryLimit" ), 500.0, QgsSettings::App ).toDouble();
  mMessageBar->pushMessage( tr( "A map layer has used all graphics memory allowed (%1 MB). "
                                "You may want to lower the amount of detail in the scene, or increase the limit in the options." )
                              .arg( memLimit ),
                            Qgis::MessageLevel::Warning );
  mGpuMemoryLimitReachedReported = true;
}

#if 0
void Kadas3DMapCanvasWidget::setSceneExtentOn2DCanvas()
{
  if ( !qobject_cast<QgsMapToolExtent *>( mMainCanvas->mapTool() ) )
    mMapToolPrevious = mMainCanvas->mapTool();

  mMainCanvas->setMapTool( mMapToolExtent.get() );
  QgisApp::instance()->activateWindow();
  QgisApp::instance()->raise();
  mMessageBar->pushInfo( QString(), tr( "Drag a rectangle on the main 2D map view to define this 3D scene's extent" ) );
}
#endif

void Kadas3DMapCanvasWidget::setSceneExtent( const QgsRectangle &extent )
{
  this->activateWindow();
  this->raise();
  mMessageBar->clearWidgets();
  if ( !extent.isEmpty() )
    mCanvas->mapSettings()->setExtent( extent );

#if 0 // TODO: checkme
  if ( mMapToolPrevious )
    mMainCanvas->setMapTool( mMapToolPrevious );
  else
    mMainCanvas->unsetMapTool( mMapToolExtent.get() );
#endif
}
