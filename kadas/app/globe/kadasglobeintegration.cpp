/***************************************************************************
    kadasglobeintegration.cpp
    -------------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsguiutils.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgssettings.h>

#include <QAction>
#include <QDir>
#include <QDockWidget>

#include <osgGA/StateSetManipulator>

#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/Registry>
#include <osgEarth/TerrainEngineNode>
#include <osgEarth/TileSource>
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>
#include <osgEarthDrivers/engine_rex/RexTerrainEngineOptions>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/sky_simple/SimpleSkyOptions>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarthDrivers/wms/WMSOptions>
#include <osgEarthQt/ViewerWidget>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/Sky>
#include <osgEarthUtil/VerticalScale>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <kadas/core/kadas.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/globe/kadasglobebillboardmanager.h>
#include <kadas/app/globe/kadasglobedialog.h>
#include <kadas/app/globe/kadasglobeintegration.h>
#include <kadas/app/globe/kadasglobeinteractionhandlers.h>
#include <kadas/app/globe/kadasglobeprojectlayermanager.h>
#include <kadas/app/globe/kadasglobevectorlayerproperties.h>
#include <kadas/app/globe/kadasglobewidget.h>


KadasGlobeIntegration::KadasGlobeIntegration( QAction *action3D, QObject *parent )
  : QObject( parent ), mAction3D( action3D )
{
  mSettingsDialog = new KadasGlobeDialog( kApp->mainWindow(), QgsGuiUtils::ModalDialogFlags );
  connect( mSettingsDialog, &KadasGlobeDialog::settingsApplied, this, &KadasGlobeIntegration::applySettings );

  mLayerPropertiesFactory = new KadasGlobeLayerPropertiesFactory( this );
  kApp->registerMapLayerPropertiesFactory( mLayerPropertiesFactory );

  mProjectLayerManager = new KadasGlobeProjectLayerManager( this );
  mBillboardManager = new KadasGlobeBillboardManager( this );

  connect( action3D, &QAction::triggered, this, &KadasGlobeIntegration::setGlobeEnabled );
  connect( this, &KadasGlobeIntegration::xyCoordinates, kApp->mainWindow()->mapCanvas(), &QgsMapCanvas::xyCoordinates );
  connect( kApp, &KadasApplication::projectRead, this, &KadasGlobeIntegration::projectRead );
  connect( kApp, &KadasApplication::projectWillBeClosed, this, [action3D] { action3D->setChecked( false ); } );
}

KadasGlobeIntegration::~KadasGlobeIntegration()
{
  if ( mDockWidget )
  {
    disconnect( mDockWidget, &QDockWidget::destroyed, this, &KadasGlobeIntegration::reset );
    delete mDockWidget;
    reset();
  }
  delete mSettingsDialog;
}

void KadasGlobeIntegration::reset()
{
  mStatsLabel = nullptr;
  mProjectLayerManager->reset();
  mBillboardManager->reset();
  mOsgViewer = nullptr;
  mMapNode = nullptr;
  mRootNode = nullptr;
  mSkyNode = nullptr;
  mBaseLayer = nullptr;
  mVerticalScale = nullptr;
  mViewerWidget = nullptr;
  mDockWidget = nullptr;
  mImagerySources.clear();
  mElevationSources.clear();
#ifdef GLOBE_SHOW_TILE_STATS
  disconnect( KadasGlobeTileStatistics::instance(), &KadasGlobeTileStatistics::changed, this, &KadasGlobeIntegration::updateTileStats );
  delete KadasGlobeTileStatistics::instance();
#endif
}

void KadasGlobeIntegration::run()
{
  if ( mViewerWidget != 0 )
  {
    return;
  }
#ifdef GLOBE_SHOW_TILE_STATS
  KadasGlobeTileStatistics *tileStats = new KadasGlobeTileStatistics();
  connect( tileStats, &KadasGlobeTileStatistics::changed, this, &KadasGlobeIntegration::updateTileStats );
#endif
  QgsSettings settings;

  mOsgViewer = new osgViewer::Viewer();
  mOsgViewer->setThreadingModel( osgViewer::Viewer::SingleThreaded );
  mOsgViewer->setRunFrameScheme( osgViewer::Viewer::ON_DEMAND );

  // Set camera manipulator with default home position
  osgEarth::Util::EarthManipulator *manip = new osgEarth::Util::EarthManipulator();
  mOsgViewer->setCameraManipulator( manip );

  // Tile stats label
  mStatsLabel = new osgEarth::Util::Controls::LabelControl( "", 10 );
  mStatsLabel->setPosition( 0, 0 );
  osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer )->addControl( mStatsLabel.get() );

  mDockWidget = new KadasGlobeWidget( mAction3D, kApp->mainWindow() );
  connect( mDockWidget, &KadasGlobeWidget::destroyed, this, &KadasGlobeIntegration::reset );
  connect( mDockWidget, &KadasGlobeWidget::layersChanged, mProjectLayerManager, [this] { mProjectLayerManager->updateLayers( mDockWidget->getSelectedLayerIds() ); } );
  connect( mDockWidget, &KadasGlobeWidget::layersChanged, mBillboardManager, [this] { mBillboardManager->updateLayers( mDockWidget->getSelectedLayerIds() ); } );
  connect( mDockWidget, &KadasGlobeWidget::showSettings, this, &KadasGlobeIntegration::showSettings );
  connect( mDockWidget, &KadasGlobeWidget::refresh, mProjectLayerManager, [this] { mMapNode->getTerrainEngine()->dirtyTerrain(); } );
  connect( mDockWidget, &KadasGlobeWidget::syncExtent, this, &KadasGlobeIntegration::syncExtent );

  QString cacheDirectory = settings.value( "cache/directory" ).toString();
  if ( cacheDirectory.isEmpty() )
    cacheDirectory = QgsApplication::qgisSettingsDirPath() + "cache";
  osgEarth::Drivers::FileSystemCacheOptions cacheOptions;
  cacheOptions.rootPath() = cacheDirectory.toStdString();

  osgEarth::MapOptions mapOptions;
  mapOptions.cache() = cacheOptions;
  osgEarth::Map *map = new osgEarth::Map( mapOptions );

  // The MapNode will render the Map object in the scene graph.
  osgEarth::MapNodeOptions mapNodeOptions;
  osgEarth::Drivers::RexTerrainEngine::RexTerrainEngineOptions terrainOptions;
  terrainOptions.morphImagery() = false;
  mapNodeOptions.setTerrainOptions( terrainOptions );
  mMapNode = new osgEarth::MapNode( map, mapNodeOptions );

  mRootNode = new osg::Group();
  mRootNode->addChild( mMapNode );

  osgEarth::Registry::instance()->unRefImageDataAfterApply() = false;

  mRootNode->addChild( osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer ) );

  mOsgViewer->setSceneData( mRootNode );

  mOsgViewer->addEventHandler( new KadasGlobeQueryCoordinatesHandler( this ) );
  mOsgViewer->addEventHandler( new KadasGlobeKeyboardControlHandler( manip ) );
  mOsgViewer->addEventHandler( new osgViewer::StatsHandler() );
  mOsgViewer->addEventHandler( new osgViewer::WindowSizeHandler() );
  mOsgViewer->addEventHandler( new osgGA::StateSetManipulator( mOsgViewer->getCamera()->getOrCreateStateSet() ) );
  mOsgViewer->getCamera()->addCullCallback( new osgEarth::Util::AutoClipPlaneCullCallback( mMapNode ) );

  // osgEarth benefits from pre-compilation of GL objects in the pager. In newer versions of
  // OSG, this activates OSG's IncrementalCompileOpeartion in order to avoid frame breaks.
  mOsgViewer->getDatabasePager()->setDoPreCompile( true );

  mViewerWidget = new osgEarth::QtGui::ViewerWidget( mOsgViewer );

  mDockWidget->setWidget( mViewerWidget );
  mViewerWidget->setParent( mDockWidget );

  int viewerWidth = kApp->mainWindow()->mapCanvas()->width() / 2.;
  kApp->mainWindow()->addDockWidget( Qt::RightDockWidgetArea, mDockWidget );
  kApp->mainWindow()->resizeDocks( {mDockWidget}, {viewerWidth}, Qt::Horizontal );

  setupProxy();
  setupControls();
  applySettings();

  mProjectLayerManager->init( mMapNode, mDockWidget->getSelectedLayerIds() );
  mBillboardManager->init( mMapNode, mDockWidget->getSelectedLayerIds() );
}

void KadasGlobeIntegration::showSettings()
{
  mSettingsDialog->exec();
}

void KadasGlobeIntegration::projectRead()
{
  setGlobeEnabled( false ); // Hide globe when new projects loaded, on some systems it is very slow loading a new project with globe enabled
  mSettingsDialog->readProjectSettings();
  applyProjectSettings();
}

void KadasGlobeIntegration::addControl( osgEarth::Util::Controls::Control *control, int x, int y, int w, int h, osgEarth::Util::Controls::ControlEventHandler *handler )
{
  control->setPosition( x, y );
  control->setHeight( h );
  control->setWidth( w );
  control->addEventHandler( handler );
  osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer )->addControl( control );
}

void KadasGlobeIntegration::addImageControl( const std::string &imgPath, int x, int y, osgEarth::Util::Controls::ControlEventHandler *handler )
{
  osg::Image *image = osgDB::readImageFile( imgPath );
  osgEarth::Util::Controls::ImageControl *control = new KadasGlobeNavigationControl( mOsgViewer, image );
  control->setPosition( x, y );
  control->setWidth( image->s() );
  control->setHeight( image->t() );
  if ( handler )
    control->addEventHandler( handler );
  osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer )->addControl( control );
}

void KadasGlobeIntegration::applySettings()
{
  if ( !mOsgViewer )
  {
    return;
  }

  osgEarth::Util::EarthManipulator *manip = dynamic_cast<osgEarth::Util::EarthManipulator *>( mOsgViewer->getCameraManipulator() );
  osgEarth::Util::EarthManipulator::Settings *settings = manip->getSettings();
  settings->setScrollSensitivity( mSettingsDialog->getScrollSensitivity() );
  if ( !mSettingsDialog->getInvertScrollWheel() )
  {
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN, osgGA::GUIEventAdapter::SCROLL_UP );
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT, osgGA::GUIEventAdapter::SCROLL_DOWN );
  }
  else
  {
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT, osgGA::GUIEventAdapter::SCROLL_UP );
    settings->bindScroll( osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN, osgGA::GUIEventAdapter::SCROLL_DOWN );
  }

  applyProjectSettings();
}

void KadasGlobeIntegration::applyProjectSettings()
{
  if ( mOsgViewer )
  {
    // Imagery settings
    QList<KadasGlobeDialog::LayerDataSource> imageryDataSources = mSettingsDialog->getImageryDataSources();
    if ( imageryDataSources != mImagerySources )
    {
      mImagerySources = imageryDataSources;
      QgsDebugMsg( "imageryLayersChanged: Globe Running, executing" );
      osg::ref_ptr<osgEarth::Map> map = mMapNode->getMap();

      // Remove image layers
      osgEarth::ImageLayerVector list;
      map->getLayers( list );
      for ( osgEarth::ImageLayerVector::iterator i = list.begin(); i != list.end(); ++i )
      {
        if ( *i != mProjectLayerManager->drapedLayer() )
          map->removeLayer( *i );
      }
      if ( !list.empty() )
      {
        mOsgViewer->getDatabasePager()->clear();
      }

      // Add image layers
      for ( const KadasGlobeDialog::LayerDataSource &datasource : mImagerySources )
      {
        osgEarth::ImageLayer *layer = 0;
        if ( "Raster" == datasource.type )
        {
          osgEarth::Drivers::GDALOptions options;
          options.url() = datasource.uri.toStdString();
          layer = new osgEarth::ImageLayer( datasource.uri.toStdString(), options );
        }
        else if ( "TMS" == datasource.type )
        {
          osgEarth::Drivers::TMSOptions options;
          options.url() = datasource.uri.toStdString();
          layer = new osgEarth::ImageLayer( datasource.uri.toStdString(), options );
        }
        else if ( "WMS" == datasource.type )
        {
          osgEarth::Drivers::WMSOptions options;
          options.url() = datasource.uri.toStdString();
          layer = new osgEarth::ImageLayer( datasource.uri.toStdString(), options );
        }
        map->insertLayer( layer, 0 );
      }
    }

    // Elevation settings
    QList<KadasGlobeDialog::LayerDataSource> elevationDataSources = mSettingsDialog->getElevationDataSources();
    if ( elevationDataSources != mElevationSources )
    {
      mElevationSources = elevationDataSources;
      QgsDebugMsg( "elevationLayersChanged: Globe Running, executing" );
      osg::ref_ptr<osgEarth::Map> map = mMapNode->getMap();

      // Remove elevation layers
      osgEarth::ElevationLayerVector list;
      map->getLayers( list );
      for ( osgEarth::ElevationLayerVector::iterator i = list.begin(); i != list.end(); ++i )
      {
        map->removeLayer( *i );
      }
      if ( !list.empty() )
      {
        mOsgViewer->getDatabasePager()->clear();
      }

      // Add elevation layers
      for ( const KadasGlobeDialog::LayerDataSource &datasource : mElevationSources )
      {
        osgEarth::ElevationLayer *layer = 0;
        if ( "Raster" == datasource.type )
        {
          osgEarth::Drivers::GDALOptions options;
          options.interpolation() = osgEarth::Drivers::INTERP_NEAREST;
          options.url() = datasource.uri.toStdString();
          layer = new osgEarth::ElevationLayer( datasource.uri.toStdString(), options );
        }
        else if ( "TMS" == datasource.type )
        {
          osgEarth::Drivers::TMSOptions options;
          options.url() = datasource.uri.toStdString();
          layer = new osgEarth::ElevationLayer( datasource.uri.toStdString(), options );
        }
        map->addLayer( layer );
      }
    }

    double verticalScaleValue = mSettingsDialog->getVerticalScale();
    if ( !mVerticalScale.get() || mVerticalScale->getScale() != verticalScaleValue )
    {
      mMapNode->getTerrainEngine()->removeEffect( mVerticalScale );
      mVerticalScale = new osgEarth::Util::VerticalScale();
      mVerticalScale->setScale( verticalScaleValue );
      mMapNode->getTerrainEngine()->addEffect( mVerticalScale );
    }

    // Sky settings
    if ( mSettingsDialog->getSkyEnabled() )
    {
      // Create if not yet done
      if ( !mSkyNode.get() )
      {
        osgEarth::SimpleSky::SimpleSkyOptions skyOpts;
        skyOpts.moonImageURI() = QString( "%1/globe/moon.jpg" ).arg( Kadas::pkgResourcePath() ).toStdString();
        mSkyNode = osgEarth::Util::SkyNode::create( skyOpts, mMapNode );
        mSkyNode->attach( mOsgViewer );
        mRootNode->addChild( mSkyNode );
        // Insert sky between root and map
        mSkyNode->addChild( mMapNode );
        mRootNode->removeChild( mMapNode );
      }

      mSkyNode->setLighting( mSettingsDialog->getSkyAutoAmbience() ? osg::StateAttribute::ON : osg::StateAttribute::OFF );
      double ambient = mSettingsDialog->getSkyMinAmbient();
      mSkyNode->getSunLight()->setAmbient( osg::Vec4( ambient, ambient, ambient, 1 ) );

      QDateTime dateTime = mSettingsDialog->getSkyDateTime();
      mSkyNode->setDateTime( osgEarth::DateTime(
                               dateTime.date().year(),
                               dateTime.date().month(),
                               dateTime.date().day(),
                               dateTime.time().hour() + dateTime.time().minute() / 60.0 ) );
    }
    else if ( mSkyNode != 0 )
    {
      mRootNode->addChild( mMapNode );
      mSkyNode->removeChild( mMapNode );
      mRootNode->removeChild( mSkyNode );
      mSkyNode = 0;
    }
  }
}

void KadasGlobeIntegration::showCurrentCoordinates( double lon, double lat )
{
  emit xyCoordinates( QgsCoordinateTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() ).transform( lon, lat ) );
}

void KadasGlobeIntegration::syncExtent()
{
  const QgsMapSettings &mapSettings = kApp->mainWindow()->mapCanvas()->mapSettings();
  QgsRectangle extent = kApp->mainWindow()->mapCanvas()->extent();

  long epsgGlobe = 4326;
  QgsCoordinateReferenceSystem globeCrs;
  globeCrs.createFromOgcWmsCrs( QString( "EPSG:%1" ).arg( epsgGlobe ) );

  // transform extent to WGS84
  if ( mapSettings.destinationCrs().authid().compare( QString( "EPSG:%1" ).arg( epsgGlobe ), Qt::CaseInsensitive ) != 0 )
  {
    QgsCoordinateReferenceSystem srcCRS( mapSettings.destinationCrs() );
    extent = QgsCoordinateTransform( srcCRS, globeCrs, QgsProject::instance()->transformContext() ).transformBoundingBox( extent );
  }

  QgsDistanceArea dist;
  dist.setSourceCrs( globeCrs, QgsProject::instance()->transformContext() );
  dist.setEllipsoid( "WGS84" );

  QgsPointXY ll = QgsPointXY( extent.xMinimum(), extent.yMinimum() );
  QgsPointXY ul = QgsPointXY( extent.xMinimum(), extent.yMaximum() );
  double height = dist.measureLine( ll, ul );

  double camViewAngle = 30;
  double camDistance = height / tan( camViewAngle * osg::PI / 180 ); //c = b*cotan(B(rad))
  osgEarth::Util::Viewpoint viewpoint;
  viewpoint.focalPoint() = osgEarth::GeoPoint( osgEarth::SpatialReference::get( "wgs84" ), extent.center().x(), extent.center().y(), 0.0 );
  viewpoint.heading() = 0.0;
  viewpoint.pitch() = -90.0;
  viewpoint.range() = camDistance;

  OE_NOTICE << "map extent: " << height << " camera distance: " << camDistance << std::endl;

  osgEarth::Util::EarthManipulator *manip = dynamic_cast<osgEarth::Util::EarthManipulator *>( mOsgViewer->getCameraManipulator() );
  manip->setRotation( osg::Quat() );
  manip->setViewpoint( viewpoint, 4.0 );
  mOsgViewer->requestRedraw();
}

void KadasGlobeIntegration::setupControls()
{
  std::string imgDir = QDir::cleanPath( Kadas::pkgResourcePath() + "/globe" ).toStdString();
  osgEarth::Util::EarthManipulator *manip = dynamic_cast<osgEarth::Util::EarthManipulator *>( mOsgViewer->getCameraManipulator() );

  // Rotate and tiltcontrols
  int imgLeft = 16;
  int imgTop = 20;
  addImageControl( imgDir + "/YawPitchWheel.png", 16, 20 );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft, imgTop + 18, 20, 22, new KadasGlobeRotateControlHandler( manip, -1., 0 ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 36, imgTop + 18, 20, 22, new KadasGlobeRotateControlHandler( manip, 1., 0 ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 20, imgTop + 18, 16, 22, new KadasGlobeRotateControlHandler( manip, 0, 0 ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 20, imgTop, 24, 19, new KadasGlobeRotateControlHandler( manip, 0, -1. ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 16, imgTop + 36, 24, 19, new KadasGlobeRotateControlHandler( manip, 0, 1. ) );

  // Move controls
  imgTop = 80;
  addImageControl( imgDir + "/MoveWheel.png", imgLeft, imgTop );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft, imgTop + 18, 20, 22, new KadasGlobePanControlHandler( manip, 1., 0 ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 36, imgTop + 18, 20, 22, new KadasGlobePanControlHandler( manip, -1., 0 ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 20, imgTop, 24, 19, new KadasGlobePanControlHandler( manip, 0, -1. ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 16, imgTop + 36, 24, 19, new KadasGlobePanControlHandler( manip, 0, 1. ) );
  addControl( new KadasGlobeNavigationControl( mOsgViewer ), imgLeft + 20, imgTop + 18, 16, 22, new KadasGlobeHomeControlHandler( manip ) );

  // Zoom controls
  imgLeft = 28;
  imgTop = imgTop + 62;
  addImageControl( imgDir + "/button-background.png", imgLeft, imgTop );
  addImageControl( imgDir + "/zoom-in.png", imgLeft + 3, imgTop + 2, new KadasGlobeZoomControlHandler( manip, 0, -1. ) );
  addImageControl( imgDir + "/zoom-out.png", imgLeft + 3, imgTop + 29, new KadasGlobeZoomControlHandler( manip, 0, 1. ) );
}

void KadasGlobeIntegration::setupProxy()
{
  QgsSettings settings;
  settings.beginGroup( "proxy" );
  if ( settings.value( "/proxyEnabled" ).toBool() )
  {
    osgEarth::ProxySettings proxySettings( settings.value( "/proxyHost" ).toString().toStdString(),
                                           settings.value( "/proxyPort" ).toInt() );
    if ( !settings.value( "/proxyUser" ).toString().isEmpty() )
    {
      QString auth = settings.value( "/proxyUser" ).toString() + ":" + settings.value( "/proxyPassword" ).toString();
      qputenv( "OSGEARTH_CURL_PROXYAUTH", auth.toLocal8Bit() );
    }
    //TODO: settings.value("/proxyType")
    //TODO: URL exclusions
    osgEarth::HTTPClient::setProxySettings( proxySettings );
  }
  settings.endGroup();
}

void KadasGlobeIntegration::setGlobeEnabled( bool enabled )
{
  if ( enabled )
  {
    run();
  }
  else if ( mDockWidget )
  {
    mDockWidget->close(); // triggers reset
  }
}

void KadasGlobeIntegration::updateTileStats( int queued, int tot )
{
  if ( mStatsLabel )
  {
    mStatsLabel->setText( QString( "Queued tiles: %1\nTot tiles: %2" ).arg( queued ).arg( tot ).toStdString() );
    mOsgViewer->requestRedraw();
  }
}
