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

#include "kadas/app/3d/kadas3dmapcanvaswidget.h"

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QToolBar>
#include <QUrl>
#include <QAction>
#include <QPushButton>
#include <QWidget>
#include <QActionGroup>

#include "kadas/app/3d/kadas3dmapcanvas.h"
#include "kadas/app/3d/kadas3dmapconfigwidget.h"
#include "kadas/app/kadasapplication.h"

#include <qgs3dmapscene.h>
#include <qgscameracontroller.h>
#include <qgshelp.h>
#include <qgsmapcanvas.h>
#include <qgsmessagebar.h>
#include <qgsapplication.h>
#include <qgssettings.h>
#include <qgsgui.h>
#include <qgsmapthemecollection.h>
#include <qgs3dmapsettings.h>
#include <qgs3dutils.h>
#include <qgsdockablewidgethelper.h>
#include <qgsrubberband.h>


Kadas3DMapCanvasWidget::Kadas3DMapCanvasWidget( const QString &name, bool isDocked )
  : QWidget( nullptr )
  , mCanvasName( name )
{
  const QgsSettings setting;

  QToolBar *toolBar = new QToolBar( this );
// TODO
//  toolBar->setIconSize( QgisApp::instance()->iconSize( true ) );

  QAction *actionCameraControl = toolBar->addAction( QIcon( QgsApplication::iconPath( "mActionPan.svg" ) ),
                                 tr( "Camera Control" ), this, &Kadas3DMapCanvasWidget::cameraControl );
  actionCameraControl->setCheckable( true );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionZoomFullExtent.svg" ) ),
                      tr( "Zoom Full" ), this, &Kadas3DMapCanvasWidget::resetView );

  QAction *toggleOnScreenNavigation = toolBar->addAction(
                                        QgsApplication::getThemeIcon( QStringLiteral( "mAction3DNavigation.svg" ) ),
                                        tr( "Toggle On-Screen Navigation" ) );

  toggleOnScreenNavigation->setCheckable( true );
  toggleOnScreenNavigation->setChecked(
    setting.value( QStringLiteral( "/3D/navigationWidget/visibility" ), true, QgsSettings::Gui ).toBool()
  );
  QObject::connect( toggleOnScreenNavigation, &QAction::toggled, this, &Kadas3DMapCanvasWidget::toggleNavigationWidget );

  toolBar->addSeparator();

  // Create action group to make the action exclusive
  QActionGroup *actionGroup = new QActionGroup( this );
  actionGroup->addAction( actionCameraControl );
  actionGroup->setExclusive( true );
  actionCameraControl->setChecked( true );

  toolBar->addAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionSaveMapAsImage.svg" ) ),
                      tr( "Save as Image…" ), this, &Kadas3DMapCanvasWidget::saveAsImage );
  toolBar->addSeparator();

  // Map Theme Menu
  mMapThemeMenu = new QMenu( this );
  connect( mMapThemeMenu, &QMenu::aboutToShow, this, &Kadas3DMapCanvasWidget::mapThemeMenuAboutToShow );
  connect( QgsProject::instance()->mapThemeCollection(), &QgsMapThemeCollection::mapThemeRenamed, this, &Kadas3DMapCanvasWidget::currentMapThemeRenamed );

  mBtnMapThemes = new QToolButton();
  mBtnMapThemes->setAutoRaise( true );
  mBtnMapThemes->setToolTip( tr( "Set View Theme" ) );
  mBtnMapThemes->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionShowAllLayers.svg" ) ) );
  mBtnMapThemes->setPopupMode( QToolButton::InstantPopup );
  mBtnMapThemes->setMenu( mMapThemeMenu );

  toolBar->addWidget( mBtnMapThemes );

  toolBar->addSeparator();

  // Options Menu
  mOptionsMenu = new QMenu( this );

  mBtnOptions = new QToolButton();
  mBtnOptions->setAutoRaise( true );
  mBtnOptions->setToolTip( tr( "Options" ) );
  mBtnOptions->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "mActionOptions.svg" ) ) );
  mBtnOptions->setPopupMode( QToolButton::InstantPopup );
  mBtnOptions->setMenu( mOptionsMenu );

  toolBar->addWidget( mBtnOptions );

  mActionEnableShadows = new QAction( tr( "Show Shadows" ), this );
  mActionEnableShadows->setCheckable( true );
  connect( mActionEnableShadows, &QAction::toggled, this, [ = ]( bool enabled )
  {
    QgsShadowSettings settings = mCanvas->map()->shadowSettings();
    settings.setRenderShadows( enabled );
    mCanvas->map()->setShadowSettings( settings );
  } );
  mOptionsMenu->addAction( mActionEnableShadows );

  mActionEnableEyeDome = new QAction( tr( "Show Eye Dome Lighting" ), this );
  mActionEnableEyeDome->setCheckable( true );
  connect( mActionEnableEyeDome, &QAction::triggered, this, [ = ]( bool enabled )
  {
    mCanvas->map()->setEyeDomeLightingEnabled( enabled );
  } );
  mOptionsMenu->addAction( mActionEnableEyeDome );

  mActionEnableAmbientOcclusion = new QAction( tr( "Show Ambient Occlusion" ), this );
  mActionEnableAmbientOcclusion->setCheckable( true );
  connect( mActionEnableAmbientOcclusion, &QAction::triggered, this, [ = ]( bool enabled )
  {
    QgsAmbientOcclusionSettings ambientOcclusionSettings = mCanvas->map()->ambientOcclusionSettings();
    ambientOcclusionSettings.setEnabled( enabled );
    mCanvas->map()->setAmbientOcclusionSettings( ambientOcclusionSettings );
  } );
  mOptionsMenu->addAction( mActionEnableAmbientOcclusion );

  mOptionsMenu->addSeparator();

  mActionSync2DNavTo3D = new QAction( tr( "2D Map View Follows 3D Camera" ), this );
  mActionSync2DNavTo3D->setCheckable( true );
  connect( mActionSync2DNavTo3D, &QAction::triggered, this, [ = ]( bool enabled )
  {
    Qgis::ViewSyncModeFlags syncMode = mCanvas->map()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D, enabled );
    mCanvas->map()->setViewSyncMode( syncMode );
  } );
  mOptionsMenu->addAction( mActionSync2DNavTo3D );

  mActionSync3DNavTo2D = new QAction( tr( "3D Camera Follows 2D Map View" ), this );
  mActionSync3DNavTo2D->setCheckable( true );
  connect( mActionSync3DNavTo2D, &QAction::triggered, this, [ = ]( bool enabled )
  {
    Qgis::ViewSyncModeFlags syncMode = mCanvas->map()->viewSyncMode();
    syncMode.setFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D, enabled );
    mCanvas->map()->setViewSyncMode( syncMode );
  } );
  mOptionsMenu->addAction( mActionSync3DNavTo2D );

  mShowFrustumPolyogon = new QAction( tr( "Show Visible Camera Area in 2D Map View" ), this );
  mShowFrustumPolyogon->setCheckable( true );
  connect( mShowFrustumPolyogon, &QAction::triggered, this, [ = ]( bool enabled )
  {
    mCanvas->map()->setViewFrustumVisualizationEnabled( enabled );
  } );
  mOptionsMenu->addAction( mShowFrustumPolyogon );

  mOptionsMenu->addSeparator();

  QAction *configureAction = new QAction( QgsApplication::getThemeIcon( QStringLiteral( "mActionOptions.svg" ) ),
                                          tr( "Configure…" ), this );
  connect( configureAction, &QAction::triggered, this, &Kadas3DMapCanvasWidget::configure );
  mOptionsMenu->addAction( configureAction );

  mCanvas = new Kadas3DMapCanvas( this );
  mCanvas->setMinimumSize( QSize( 200, 200 ) );
  mCanvas->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

  connect( mCanvas, &Kadas3DMapCanvas::savedAsImage, this, [ = ]( const QString & fileName )
  {
// TODO
//    QgisApp::instance()->messageBar()->pushSuccess( tr( "Save as Image" ), tr( "Successfully saved the 3D map to <a href=\"%1\">%2</a>" ).arg( QUrl::fromLocalFile( fileName ).toString(), QDir::toNativeSeparators( fileName ) ) );
  } );

  connect( mCanvas, &Kadas3DMapCanvas::fpsCountChanged, this, &Kadas3DMapCanvasWidget::updateFpsCount );
  connect( mCanvas, &Kadas3DMapCanvas::fpsCounterEnabledChanged, this, &Kadas3DMapCanvasWidget::toggleFpsCounter );
  connect( mCanvas, &Kadas3DMapCanvas::cameraNavigationSpeedChanged, this, &Kadas3DMapCanvasWidget::cameraNavigationSpeedChanged );
  connect( mCanvas, &Kadas3DMapCanvas::viewed2DExtentFrom3DChanged, this, &Kadas3DMapCanvasWidget::onViewed2DExtentFrom3DChanged );

  mLabelPendingJobs = new QLabel( this );
  mProgressPendingJobs = new QProgressBar( this );
  mProgressPendingJobs->setRange( 0, 0 );
  mLabelFpsCounter = new QLabel( this );
  mLabelNavigationSpeed = new QLabel( this );

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
  connect( mLabelNavSpeedHideTimeout, &QTimer::timeout, this, [ = ]
  {
    mLabelNavigationSpeed->hide();
    mLabelNavSpeedHideTimeout->stop();
  } );

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 0 );
  layout->addLayout( topLayout );
  layout->addWidget( mMessageBar );
  layout->addWidget( mCanvas );

  setLayout( layout );

  onTotalPendingJobsCountChanged();

  mDockableWidgetHelper = new QgsDockableWidgetHelper( isDocked, mCanvasName, this, QgsDockableWidgetHelper::sOwnerWindow );
  if ( QDialog *dialog = mDockableWidgetHelper->dialog() )
  {
    QFontMetrics fm( font() );
    const int initialSize = fm.horizontalAdvance( '0' ) * 75;
    dialog->resize( initialSize, initialSize );
  }
  QToolButton *toggleButton = mDockableWidgetHelper->createDockUndockToolButton();
  toggleButton->setToolTip( tr( "Dock 3D Map View" ) );
  toolBar->addWidget( toggleButton );
  connect( mDockableWidgetHelper, &QgsDockableWidgetHelper::closed, this, [ = ]()
  {
// TODO
//    QgisApp::instance()->close3DMapView( canvasName() );
  } );
}

void Kadas3DMapCanvasWidget::resizeEvent( QResizeEvent *event )
{
  QWidget::resizeEvent( event );
  mCanvas->resize( event->size() );
}

void Kadas3DMapCanvasWidget::saveAsImage()
{
  const QPair< QString, QString> fileNameAndFilter = QgsGuiUtils::getSaveAsImageName( this, tr( "Choose a file name to save the 3D map canvas to an image" ) );
  if ( !fileNameAndFilter.first.isEmpty() )
  {
    mCanvas->saveAsImage( fileNameAndFilter.first, fileNameAndFilter.second );
  }
}

void Kadas3DMapCanvasWidget::cameraControl()
{
  QAction *action = qobject_cast<QAction *>( sender() );
  if ( !action )
    return;

  mCanvas->setMapTool( nullptr );
}

void Kadas3DMapCanvasWidget::setCanvasName( const QString &name )
{
  mCanvasName = name;
  mDockableWidgetHelper->setWindowTitle( name );
}

void Kadas3DMapCanvasWidget::toggleNavigationWidget( bool visibility )
{
  mCanvas->setOnScreenNavigationVisibility( visibility );
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

  mCanvas->setMap( map );

  connect( mCanvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, this, &Kadas3DMapCanvasWidget::onTotalPendingJobsCountChanged );
  connect( mCanvas->scene(), &Qgs3DMapScene::gpuMemoryLimitReached, this, &Kadas3DMapCanvasWidget::onGpuMemoryLimitReached );

  // Disable button for switching the map theme if the terrain generator is a mesh, or if there is no terrain
  mBtnMapThemes->setDisabled( !mCanvas->map()->terrainRenderingEnabled()
                              || !mCanvas->map()->terrainGenerator()
                              || mCanvas->map()->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  mLabelFpsCounter->setVisible( map->isFpsCounterEnabled() );

  connect( map, &Qgs3DMapSettings::viewFrustumVisualizationEnabledChanged, this, &Kadas3DMapCanvasWidget::onViewFrustumVisualizationEnabledChanged );
  connect( map, &Qgs3DMapSettings::extentChanged, this, &Kadas3DMapCanvasWidget::onExtentChanged );
  connect( map, &Qgs3DMapSettings::showExtentIn2DViewChanged, this, &Kadas3DMapCanvasWidget::onExtentChanged );
  onExtentChanged();
}

void Kadas3DMapCanvasWidget::setMainCanvas( QgsMapCanvas *canvas )
{
  mMainCanvas = canvas;

  connect( mMainCanvas, &QgsMapCanvas::layersChanged, this, &Kadas3DMapCanvasWidget::onMainCanvasLayersChanged );
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

  Qgs3DMapSettings *map = mCanvas->map();
  Kadas3DMapConfigWidget *w = new Kadas3DMapConfigWidget( map, mMainCanvas, mCanvas, mConfigureDialog );
  QDialogButtonBox *buttons = new QDialogButtonBox( QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, mConfigureDialog );

  auto applyConfig = [ = ]()
  {
    const QgsVector3D oldOrigin = map->origin();
    const QgsCoordinateReferenceSystem oldCrs = map->crs();
    const QgsCameraPose oldCameraPose = mCanvas->cameraController()->cameraPose();
    const QgsVector3D oldLookingAt = oldCameraPose.centerPoint();

    // update map
    w->apply();

    const QgsVector3D p = Qgs3DUtils::transformWorldCoordinates(
                            oldLookingAt,
                            oldOrigin, oldCrs,
                            map->origin(), map->crs(), QgsProject::instance()->transformContext() );

    if ( p != oldLookingAt )
    {
      // apply() call has moved origin of the world so let's move camera so we look still at the same place
      QgsCameraPose newCameraPose = oldCameraPose;
      newCameraPose.setCenterPoint( p );
      mCanvas->cameraController()->setCameraPose( newCameraPose );
    }

    // Disable map theme button if the terrain generator is a mesh, or if there is no terrain
    mBtnMapThemes->setDisabled( !mCanvas->map()->terrainRenderingEnabled()
                                || !mCanvas->map()->terrainGenerator()
                                || map->terrainGenerator()->type() == QgsTerrainGenerator::Mesh );
  };

  connect( buttons, &QDialogButtonBox::rejected, mConfigureDialog, &QDialog::reject );
  connect( buttons, &QDialogButtonBox::clicked, mConfigureDialog, [ = ]( QAbstractButton * button )
  {
    if ( button == buttons->button( QDialogButtonBox::Apply ) || button == buttons->button( QDialogButtonBox::Ok ) )
      applyConfig();
    if ( button == buttons->button( QDialogButtonBox::Ok ) )
      mConfigureDialog->accept();
  } );
  connect( buttons, &QDialogButtonBox::helpRequested, w, []() { QgsHelp::openHelp( QStringLiteral( "map_views/3d_map_view.html#scene-configuration" ) ); } );

  connect( w, &Kadas3DMapConfigWidget::isValidChanged, this, [ = ]( bool valid )
  {
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

void Kadas3DMapCanvasWidget::onMainCanvasLayersChanged()
{
  mCanvas->map()->setLayers( mMainCanvas->layers( true ) );
}

void Kadas3DMapCanvasWidget::onMainCanvasColorChanged()
{
  mCanvas->map()->setBackgroundColor( mMainCanvas->canvasColor() );
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

void Kadas3DMapCanvasWidget::mapThemeMenuAboutToShow()
{
  qDeleteAll( mMapThemeMenuPresetActions );
  mMapThemeMenuPresetActions.clear();

  const QString currentTheme = mCanvas->map()->terrainMapTheme();

  QAction *actionFollowMain = new QAction( tr( "(none)" ), mMapThemeMenu );
  actionFollowMain->setCheckable( true );
  if ( currentTheme.isEmpty() || !QgsProject::instance()->mapThemeCollection()->hasMapTheme( currentTheme ) )
  {
    actionFollowMain->setChecked( true );
  }
  connect( actionFollowMain, &QAction::triggered, this, [ = ]
  {
    mCanvas->map()->setTerrainMapTheme( QString() );
  } );
  mMapThemeMenuPresetActions.append( actionFollowMain );

  const auto constMapThemes = QgsProject::instance()->mapThemeCollection()->mapThemes();
  for ( const QString &grpName : constMapThemes )
  {
    QAction *a = new QAction( grpName, mMapThemeMenu );
    a->setCheckable( true );
    if ( grpName == currentTheme )
    {
      a->setChecked( true );
    }
    connect( a, &QAction::triggered, this, [a, this]
    {
      mCanvas->map()->setTerrainMapTheme( a->text() );
    } );
    mMapThemeMenuPresetActions.append( a );
  }
  mMapThemeMenu->addActions( mMapThemeMenuPresetActions );
}

void Kadas3DMapCanvasWidget::currentMapThemeRenamed( const QString &theme, const QString &newTheme )
{
  if ( theme == mCanvas->map()->terrainMapTheme() )
  {
    mCanvas->map()->setTerrainMapTheme( newTheme );
  }
}

void Kadas3DMapCanvasWidget::onMainMapCanvasExtentChanged()
{
  if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
  {
    mCanvas->setViewFrom2DExtent( mMainCanvas->extent() );
  }
}

void Kadas3DMapCanvasWidget::onViewed2DExtentFrom3DChanged( QVector<QgsPointXY> extent )
{
  if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync2DTo3D ) )
  {
    QgsRectangle extentRect;
    extentRect.setNull();
    for ( QgsPointXY &pt : extent )
    {
      extentRect.include( pt );
    }
    if ( !extentRect.isEmpty() && extentRect.isFinite() && !extentRect.isNull() )
    {
      if ( mCanvas->map()->viewSyncMode().testFlag( Qgis::ViewSyncModeFlag::Sync3DTo2D ) )
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
  if ( mCanvas->map()->viewFrustumVisualizationEnabled() )
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
  Qgs3DMapSettings *mapSettings = mCanvas->map();
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
                            .arg( memLimit ), Qgis::MessageLevel::Warning );
  mGpuMemoryLimitReachedReported = true;
}
