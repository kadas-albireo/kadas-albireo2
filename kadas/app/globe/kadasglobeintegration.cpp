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
#include <qgis/qgsgeometry.h>
#include <qgis/qgsguiutils.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssymbol.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectorlayerlabeling.h>

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QStringList>

#include <osgGA/StateSetManipulator>

#include <osgEarth/ElevationQuery>
#include <osgEarth/Notify>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/Registry>
#include <osgEarth/TerrainEngineNode>
#include <osgEarth/TileSource>
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>
#include <osgEarthDrivers/engine_mp/MPTerrainEngineOptions>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
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
#include <kadas/app/globe/kadasglobeintegration.h>
#include <kadas/app/globe/kadasglobedialog.h>
#include <kadas/app/globe/kadasglobefeatureidentify.h>
#include <kadas/app/globe/kadasglobefrustumhighlight.h>
#include <kadas/app/globe/kadasglobeinteractionhandlers.h>
#include <kadas/app/globe/kadasglobetilesource.h>
#include <kadas/app/globe/kadasglobevectorlayerproperties.h>
#include <kadas/app/globe/kadasglobewidget.h>
#include <kadas/app/globe/featuresource/kadasglobefeatureoptions.h>


KadasGlobeIntegration::KadasGlobeIntegration( QAction *action3D, QObject *parent )
  : QObject( parent )
{
  mSettingsDialog = new KadasGlobeDialog( kApp->mainWindow(), QgsGuiUtils::ModalDialogFlags );
  connect( mSettingsDialog, &KadasGlobeDialog::settingsApplied, this, &KadasGlobeIntegration::applySettings );

  mLayerPropertiesFactory = new KadasGlobeLayerPropertiesFactory( this );
  kApp->registerMapLayerPropertiesFactory( mLayerPropertiesFactory );

  connect( action3D, &QAction::triggered, this, &KadasGlobeIntegration::setGlobeEnabled );
  connect( this, &KadasGlobeIntegration::xyCoordinates, kApp->mainWindow()->mapCanvas(), &QgsMapCanvas::xyCoordinates );
  connect( kApp, &KadasApplication::projectRead, this, &KadasGlobeIntegration::projectRead );
}

KadasGlobeIntegration::~KadasGlobeIntegration()
{
  if ( mDockWidget )
  {
    disconnect( mDockWidget, &QDockWidget::destroyed, this, &KadasGlobeIntegration::reset );
    delete mDockWidget;
    reset();
  }
  delete mLayerPropertiesFactory;
  mLayerPropertiesFactory = nullptr;
  delete mSettingsDialog;
  mSettingsDialog = nullptr;

  disconnect( this, &KadasGlobeIntegration::xyCoordinates, kApp->mainWindow()->mapCanvas(), &QgsMapCanvas::xyCoordinates );
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

  setupProxy();

  // Tile stats label
  mStatsLabel = new osgEarth::Util::Controls::LabelControl( "", 10 );
  mStatsLabel->setPosition( 0, 0 );
  osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer )->addControl( mStatsLabel.get() );

  mDockWidget = new KadasGlobeWidget( kApp->mainWindow() );
  connect( mDockWidget, &KadasGlobeWidget::destroyed, this, &KadasGlobeIntegration::reset );
  connect( mDockWidget, &KadasGlobeWidget::layersChanged, this, &KadasGlobeIntegration::updateLayers );
  connect( mDockWidget, &KadasGlobeWidget::showSettings, this, &KadasGlobeIntegration::showSettings );
  connect( mDockWidget, &KadasGlobeWidget::refresh, this, &KadasGlobeIntegration::rebuildQGISLayer );
  connect( mDockWidget, &KadasGlobeWidget::syncExtent, this, &KadasGlobeIntegration::syncExtent );
  kApp->mainWindow()->addDockWidget( Qt::RightDockWidgetArea, mDockWidget );


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
  mMapNode = new osgEarth::MapNode( map, mapNodeOptions );

  mRootNode = new osg::Group();
  mRootNode->addChild( mMapNode );

  osgEarth::Registry::instance()->unRefImageDataAfterApply() = false;

  // Add draped layer
  osgEarth::TileSourceOptions opts;
  opts.L2CacheSize() = 0;
  mTileSource = new KadasGlobeTileSource( kApp->mainWindow()->mapCanvas(), opts );
  mTileSource->open();

  osgEarth::ImageLayerOptions options( "QGIS" );
  options.driver()->L2CacheSize() = 0;
  options.tileSize() = 128;
  options.cachePolicy() = osgEarth::CachePolicy::USAGE_NO_CACHE;
  mQgisMapLayer = new osgEarth::ImageLayer( options, mTileSource );
  map->addLayer( mQgisMapLayer );


  // Create the frustum highlight callback
  mFrustumHighlightCallback = new KadasGlobeFrustumHighlightCallback(
    mOsgViewer, mMapNode->getTerrain(), kApp->mainWindow()->mapCanvas(), QColor( 0, 0, 0, 50 ) );

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
  QGLFormat glf = QGLFormat::defaultFormat();
  glf.setVersion( 3, 3 );
  glf.setProfile( QGLFormat::CoreProfile );
  if ( settings.value( "/Globe/anti-aliasing", true ).toBool() &&
       settings.value( "/Globe/anti-aliasing-level", "" ).toInt() > 0 )
  {
    glf.setSampleBuffers( true );
    glf.setSamples( settings.value( "/Globe/anti-aliasing-level", "" ).toInt() );
  }
  mViewerWidget->setFormat( glf );

  mDockWidget->setWidget( mViewerWidget );
  mViewerWidget->setParent( mDockWidget );

  setupControls();
  applySettings();
  updateLayers();
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

  // Advanced settings
  enableFrustumHighlight( mSettingsDialog->getFrustumHighlighting() );

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
        if ( *i != mQgisMapLayer )
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
        skyOpts.moonImageURI() = QString( "%1/globe/moon.jpg" ).arg( Kadas::pkgDataPath() ).toStdString();
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

QgsRectangle KadasGlobeIntegration::getQGISLayerExtent() const
{
  QList<QgsRectangle> extents = mLayerExtents.values();
  QgsRectangle fullExtent = extents.isEmpty() ? QgsRectangle() : extents.front();
  for ( int i = 1, n = extents.size(); i < n; ++i )
  {
    if ( !extents[i].isNull() )
      fullExtent.combineExtentWith( extents[i] );
  }
  return fullExtent;
}

void KadasGlobeIntegration::showCurrentCoordinates( const osgEarth::GeoPoint &geoPoint )
{
  osg::Vec3d pos = geoPoint.vec3d();
  emit xyCoordinates( QgsCoordinateTransform( QgsCoordinateReferenceSystem( GEO_EPSG_CRS_AUTHID ), kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() ).transform( QgsPointXY( pos.x(), pos.y() ) ) );
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
//  double height = dist.computeDistanceBearing( ll, ul );

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
  osgEarth::Util::Controls::ImageControl *control = new KadasGlobeNavigationControl( image );
  control->setPosition( x, y );
  control->setWidth( image->s() );
  control->setHeight( image->t() );
  if ( handler )
    control->addEventHandler( handler );
  osgEarth::Util::Controls::ControlCanvas::get( mOsgViewer )->addControl( control );
}

void KadasGlobeIntegration::setupControls()
{
  std::string imgDir = QDir::cleanPath( Kadas::pkgDataPath() + "/globe" ).toStdString();
  osgEarth::Util::EarthManipulator *manip = dynamic_cast<osgEarth::Util::EarthManipulator *>( mOsgViewer->getCameraManipulator() );

  // Rotate and tiltcontrols
  int imgLeft = 16;
  int imgTop = 20;
  addImageControl( imgDir + "/YawPitchWheel.png", 16, 20 );
  addControl( new KadasGlobeNavigationControl, imgLeft, imgTop + 18, 20, 22, new KadasGlobeRotateControlHandler( manip, -1., 0 ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 36, imgTop + 18, 20, 22, new KadasGlobeRotateControlHandler( manip, 1., 0 ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 20, imgTop + 18, 16, 22, new KadasGlobeRotateControlHandler( manip, 0, 0 ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 20, imgTop, 24, 19, new KadasGlobeRotateControlHandler( manip, 0, -1. ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 16, imgTop + 36, 24, 19, new KadasGlobeRotateControlHandler( manip, 0, 1. ) );

  // Move controls
  imgTop = 80;
  addImageControl( imgDir + "/MoveWheel.png", imgLeft, imgTop );
  addControl( new KadasGlobeNavigationControl, imgLeft, imgTop + 18, 20, 22, new KadasGlobePanControlHandler( manip, 1., 0 ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 36, imgTop + 18, 20, 22, new KadasGlobePanControlHandler( manip, -1., 0 ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 20, imgTop, 24, 19, new KadasGlobePanControlHandler( manip, 0, -1. ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 16, imgTop + 36, 24, 19, new KadasGlobePanControlHandler( manip, 0, 1. ) );
  addControl( new KadasGlobeNavigationControl, imgLeft + 20, imgTop + 18, 16, 22, new KadasGlobeHomeControlHandler( manip ) );

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

void KadasGlobeIntegration::refreshQGISMapLayer( const QgsRectangle &dirtyRect )
{
  if ( mTileSource )
  {
    mOsgViewer->getDatabasePager()->clear();
    mTileSource->refresh( dirtyRect );
    mOsgViewer->requestRedraw();
  }
}

void KadasGlobeIntegration::updateTileStats( int queued, int tot )
{
  if ( mStatsLabel )
    mStatsLabel->setText( QString( "Queued tiles: %1\nTot tiles: %2" ).arg( queued ).arg( tot ).toStdString() );
}

void KadasGlobeIntegration::addModelLayer( QgsVectorLayer *vLayer, KadasGlobeVectorLayerConfig *layerConfig )
{
  KadasGlobeFeatureOptions featureOpt;
  featureOpt.setLayer( vLayer );
  osgEarth::Style style;

  QgsRenderContext ctx;
  if ( !vLayer->renderer()->symbols( ctx ).isEmpty() )
  {
    for ( QgsSymbol *sym : vLayer->renderer()->symbols( ctx ) )
    {
      if ( sym->type() == QgsSymbol::Line )
      {
        osgEarth::LineSymbol *ls = style.getOrCreateSymbol<osgEarth::LineSymbol>();
        QColor color = sym->color();
        ls->stroke()->color() = osg::Vec4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() * vLayer->opacity() );
        ls->stroke()->width() = 1.0f;
      }
      else if ( sym->type() == QgsSymbol::Fill )
      {
        // TODO access border color, etc.
        osgEarth::PolygonSymbol *poly = style.getOrCreateSymbol<osgEarth::PolygonSymbol>();
        QColor color = sym->color();
        poly->fill()->color() = osg::Vec4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() * vLayer->opacity() );
        style.addSymbol( poly );
      }
    }
  }
  else
  {
    osgEarth::PolygonSymbol *poly = style.getOrCreateSymbol<osgEarth::PolygonSymbol>();
    poly->fill()->color() = osg::Vec4f( 1.f, 0, 0, vLayer->opacity() );
    style.addSymbol( poly );
    osgEarth::LineSymbol *ls = style.getOrCreateSymbol<osgEarth::LineSymbol>();
    ls->stroke()->color() = osg::Vec4f( 1.f, 0, 0, vLayer->opacity() );
    ls->stroke()->width() = 1.0f;
  }

  osgEarth::AltitudeSymbol *altitudeSymbol = style.getOrCreateSymbol<osgEarth::AltitudeSymbol>();
  altitudeSymbol->clamping() = layerConfig->altitudeClamping;
  altitudeSymbol->technique() = layerConfig->altitudeTechnique;
  altitudeSymbol->binding() = layerConfig->altitudeBinding;
  altitudeSymbol->verticalOffset() = layerConfig->verticalOffset;
  altitudeSymbol->verticalScale() = layerConfig->verticalScale;
  altitudeSymbol->clampingResolution() = layerConfig->clampingResolution;
  style.addSymbol( altitudeSymbol );

  if ( layerConfig->extrusionEnabled )
  {
    osgEarth::ExtrusionSymbol *extrusionSymbol = style.getOrCreateSymbol<osgEarth::ExtrusionSymbol>();
    bool extrusionHeightOk = false;
    float extrusionHeight = layerConfig->extrusionHeight.toFloat( &extrusionHeightOk );
    if ( extrusionHeightOk )
    {
      extrusionSymbol->height() = extrusionHeight;
    }
    else
    {
      extrusionSymbol->heightExpression() = layerConfig->extrusionHeight.toStdString();
    }

    extrusionSymbol->flatten() = layerConfig->extrusionFlatten;
    extrusionSymbol->wallGradientPercentage() = layerConfig->extrusionWallGradient;
    style.addSymbol( extrusionSymbol );
  }

  if ( layerConfig->labelingEnabled )
  {
    osgEarth::TextSymbol *textSymbol = style.getOrCreateSymbol<osgEarth::TextSymbol>();
    textSymbol->declutter() = layerConfig->labelingDeclutter;
    QgsPalLayerSettings lyr = vLayer->labeling()->settings();
    textSymbol->content() = QString( "[%1]" ).arg( lyr.fieldName ).toStdString();
    textSymbol->font() = lyr.format().font().family().toStdString();
    textSymbol->size() = lyr.format().font().pointSize();
    textSymbol->alignment() = osgEarth::TextSymbol::ALIGN_CENTER_TOP;
    osgEarth::Stroke stroke;
    QColor bufferColor = lyr.format().buffer().color();
    stroke.color() = osgEarth::Symbology::Color( bufferColor.redF(), bufferColor.greenF(), bufferColor.blueF(), bufferColor.alphaF() );
    textSymbol->halo() = stroke;
    textSymbol->haloOffset() = lyr.format().buffer().size();
  }

  osgEarth::RenderSymbol *renderSymbol = style.getOrCreateSymbol<osgEarth::RenderSymbol>();
  renderSymbol->lighting() = layerConfig->lightingEnabled;
  renderSymbol->backfaceCulling() = false;
  style.addSymbol( renderSymbol );

  osgEarth::Drivers::FeatureGeomModelOptions geomOpt;
  geomOpt.featureOptions() = featureOpt;
  geomOpt.styles() = new osgEarth::StyleSheet();
  geomOpt.styles()->addStyle( style );

  geomOpt.featureIndexing() = osgEarth::Features::FeatureSourceIndexOptions();

#if 0
  FeatureDisplayLayout layout;
  layout.tileSizeFactor() = 45.0;
  layout.addLevel( FeatureLevel( 0.0f, 200000.0f ) );
  geomOpt.layout() = layout;
#endif

  osgEarth::ModelLayerOptions modelOptions( vLayer->id().toStdString(), geomOpt );

  osgEarth::ModelLayer *nLayer = new osgEarth::ModelLayer( modelOptions );

  mMapNode->getMap()->addLayer( nLayer );
}

void KadasGlobeIntegration::updateLayers()
{
  if ( mOsgViewer )
  {
    // Get previous full extent
    QgsRectangle dirtyExtent = getQGISLayerExtent();
    mLayerExtents.clear();

    QList<QgsMapLayer *> drapedLayers;
    QStringList selectedLayerIds = mDockWidget->getSelectedLayerIds();

    // Disconnect any previous repaintRequested signals
    for ( QgsMapLayer *mapLayer : mTileSource->layers() )
    {
      if ( mapLayer )
        disconnect( mapLayer, &QgsMapLayer::repaintRequested, this, &KadasGlobeIntegration::layerChanged );
      if ( qobject_cast<QgsVectorLayer *>( mapLayer ) )
        disconnect( static_cast<QgsVectorLayer *>( mapLayer ), &QgsVectorLayer::opacityChanged, this, &KadasGlobeIntegration::layerChanged );
    }
    osgEarth::ModelLayerVector modelLayers;
    mMapNode->getMap()->getLayers( modelLayers );
    for ( const osg::ref_ptr<osgEarth::ModelLayer> &modelLayer : modelLayers )
    {
      QgsMapLayer *mapLayer = QgsProject::instance()->mapLayer( QString::fromStdString( modelLayer->getName() ) );
      if ( mapLayer )
        disconnect( mapLayer, &QgsMapLayer::repaintRequested, this, &KadasGlobeIntegration::layerChanged );
      if ( qobject_cast<QgsVectorLayer *>( mapLayer ) )
        disconnect( static_cast<QgsVectorLayer *>( mapLayer ), &QgsVectorLayer::opacityChanged, this, &KadasGlobeIntegration::layerChanged );
      if ( !selectedLayerIds.contains( QString::fromStdString( modelLayer->getName() ) ) )
        mMapNode->getMap()->removeLayer( modelLayer );
    }

    for ( const QString &layerId : selectedLayerIds )
    {
      QgsMapLayer *mapLayer = QgsProject::instance()->mapLayer( layerId );
      connect( mapLayer, &QgsMapLayer::repaintRequested, this, &KadasGlobeIntegration::layerChanged );

      KadasGlobeVectorLayerConfig *layerConfig = 0;
      if ( qobject_cast<QgsVectorLayer *>( mapLayer ) )
      {
        layerConfig = KadasGlobeVectorLayerConfig::getConfig( static_cast<QgsVectorLayer *>( mapLayer ) );
        connect( static_cast<QgsVectorLayer *>( mapLayer ), &QgsVectorLayer::opacityChanged, this, &KadasGlobeIntegration::layerChanged );
      }

      if ( layerConfig && ( layerConfig->renderingMode == KadasGlobeVectorLayerConfig::RenderingModeModelSimple || layerConfig->renderingMode == KadasGlobeVectorLayerConfig::RenderingModeModelAdvanced ) )
      {
        if ( !mMapNode->getMap()->getLayerByName( mapLayer->id().toStdString() ) )
          addModelLayer( static_cast<QgsVectorLayer *>( mapLayer ), layerConfig );
      }
      else
      {
        drapedLayers.append( mapLayer );
        QgsRectangle extent = QgsCoordinateTransform( mapLayer->crs(), QgsCoordinateReferenceSystem( GEO_EPSG_CRS_AUTHID ), QgsProject::instance()->transformContext() ).transform( mapLayer->extent() );
        mLayerExtents.insert( mapLayer->id(), extent );
      }
    }

    mTileSource->setLayers( drapedLayers );
    QgsRectangle newExtent = getQGISLayerExtent();
    if ( dirtyExtent.isNull() )
      dirtyExtent = newExtent;
    else if ( !newExtent.isNull() )
      dirtyExtent.combineExtentWith( newExtent );
    refreshQGISMapLayer( dirtyExtent );
  }
}

void KadasGlobeIntegration::layerChanged()
{
  QgsMapLayer *mapLayer = qobject_cast<QgsMapLayer *>( QObject::sender() );
  if ( !mapLayer )
  {
    return;
  }
  if ( mapLayer->isEditable() )
  {
    return;
  }
  if ( mMapNode )
  {
    KadasGlobeVectorLayerConfig *layerConfig = 0;
    if ( qobject_cast<QgsVectorLayer *>( mapLayer ) )
    {
      layerConfig = KadasGlobeVectorLayerConfig::getConfig( static_cast<QgsVectorLayer *>( mapLayer ) );
    }

    if ( layerConfig && ( layerConfig->renderingMode == KadasGlobeVectorLayerConfig::RenderingModeModelSimple || layerConfig->renderingMode == KadasGlobeVectorLayerConfig::RenderingModeModelAdvanced ) )
    {
      // If was previously a draped layer, refresh the draped layer
      if ( mTileSource->layers().contains( mapLayer ) )
      {
        QList<QgsMapLayer *> layers = mTileSource->layers();
        layers.removeAll( mapLayer );
        mTileSource->setLayers( layers );
        QgsRectangle dirtyExtent = mLayerExtents[mapLayer->id()];
        mLayerExtents.remove( mapLayer->id() );
        refreshQGISMapLayer( dirtyExtent );
      }
      osgEarth::Layer *layer = mMapNode->getMap()->getLayerByName( mapLayer->id().toStdString() );
      if ( layer )
      {
        mMapNode->getMap()->removeLayer( layer );
      }
      addModelLayer( static_cast<QgsVectorLayer *>( mapLayer ), layerConfig );
    }
    else
    {
      // Re-insert into layer set if necessary
      if ( !mTileSource->layers().contains( mapLayer ) )
      {
        QList<QgsMapLayer *> layers;
        for ( const QString &layerId : mDockWidget->getSelectedLayerIds() )
        {
          if ( ! mMapNode->getMap()->getLayerByName( layerId.toStdString() ) )
          {
            QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
            if ( layer )
            {
              layers.append( layer );
            }
          }
        }
        mTileSource->setLayers( layers );
        QgsRectangle extent = QgsCoordinateTransform( mapLayer->crs(), QgsCoordinateReferenceSystem( GEO_EPSG_CRS_AUTHID ), QgsProject::instance()->transformContext() ).transform( mapLayer->extent() );
        mLayerExtents.insert( mapLayer->id(), extent );
      }
      // Remove any model layer of that layer, in case one existed
      osgEarth::Layer *layer = mMapNode->getMap()->getLayerByName( mapLayer->id().toStdString() );
      if ( layer )
      {
        mMapNode->getMap()->removeLayer( layer );
      }
      QgsRectangle layerExtent = QgsCoordinateTransform( mapLayer->crs(), QgsCoordinateReferenceSystem( GEO_EPSG_CRS_AUTHID ), QgsProject::instance()->transformContext() ).transform( mapLayer->extent() );
      QgsRectangle dirtyExtent = layerExtent;
      if ( mLayerExtents.contains( mapLayer->id() ) )
      {
        if ( dirtyExtent.isNull() )
          dirtyExtent = mLayerExtents[mapLayer->id()];
        else if ( !mLayerExtents[mapLayer->id()].isNull() )
          dirtyExtent.combineExtentWith( mLayerExtents[mapLayer->id()] );
      }
      mLayerExtents[mapLayer->id()] = layerExtent;
      refreshQGISMapLayer( dirtyExtent );
    }
  }
}

void KadasGlobeIntegration::rebuildQGISLayer()
{
  if ( mMapNode )
  {
    mMapNode->getMap()->removeLayer( mQgisMapLayer );
    mLayerExtents.clear();

    osgEarth::TileSourceOptions opts;
    opts.L2CacheSize() = 0;
    mTileSource = new KadasGlobeTileSource( kApp->mainWindow()->mapCanvas(), opts );
    mTileSource->open();

    osgEarth::ImageLayerOptions options( "QGIS" );
    options.driver()->L2CacheSize() = 0;
    options.tileSize() = 128;
    options.cachePolicy() = osgEarth::CachePolicy::USAGE_NO_CACHE;
    mQgisMapLayer = new osgEarth::ImageLayer( options, mTileSource );
    mMapNode->getMap()->addLayer( mQgisMapLayer );
    updateLayers();
  }
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

void KadasGlobeIntegration::reset()
{
  mStatsLabel = nullptr;
  mMapNode->getMap()->removeLayer( mQgisMapLayer ); // abort any rendering
  mTileSource->waitForFinished();
  mOsgViewer = nullptr;
  mMapNode = nullptr;
  mRootNode = nullptr;
  mSkyNode = nullptr;
  mBaseLayer = nullptr;
  mBaseLayerUrl.clear();
  mQgisMapLayer = nullptr;
  mTileSource = nullptr;
  mVerticalScale = nullptr;
  mFrustumHighlightCallback = nullptr;
  mViewerWidget = nullptr;
  mDockWidget = nullptr;
  mImagerySources.clear();
  mElevationSources.clear();
  mLayerExtents.clear();
#ifdef GLOBE_SHOW_TILE_STATS
  disconnect( KadasGlobeTileStatistics::instance(), &KadasGlobeTileStatistics::changed, this, &KadasGlobeIntegration::updateTileStats );
  delete KadasGlobeTileStatistics::instance();
#endif
}

void KadasGlobeIntegration::enableFrustumHighlight( bool status )
{
  if ( status )
    mMapNode->getTerrainEngine()->addUpdateCallback( mFrustumHighlightCallback );
  else
    mMapNode->getTerrainEngine()->removeUpdateCallback( mFrustumHighlightCallback );
}
