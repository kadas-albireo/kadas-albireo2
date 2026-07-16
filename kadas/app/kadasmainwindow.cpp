/***************************************************************************
    kadasmainwindow.cpp
    -------------------
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

#include <QDrag>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImageReader>
#include <QActionGroup>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QShortcut>
#include <QUrlQuery>

#include <gdal.h>

#include <qgis/qgsgui.h>
#include <qgis/qgsblockingnetworkrequest.h>
#include <qgis/qgsdoublespinbox.h>
#include <qgis/qgsfloatingwidget.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreemapcanvasbridge.h>
#include <qgis/qgslayertreeregistrybridge.h>
#include <qgis/qgslayertreeviewdefaultactions.h>
#include <qgis/qgslocator.h>
#include <qgis/qgslocatorwidget.h>
#include <qgis/qgsmaptool.h>
#include <qgis/qgsmaplayertemporalproperties.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmimedatautils.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsscalecombobox.h>
#include <qgis/qgssnappingutils.h>
#include <qgis/qgssourceselectproviderregistry.h>
#include <qgis/qgssourceselectprovider.h>
#include <qgis/qgsvectortilelayer.h>
#include <qgis/qgselevationcontrollerwidget.h>
#include <qgis/qgsrasterlayerelevationproperties.h>

#include "kadas/core/kadas.h"
#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadasclipboard.h"
#include "kadas/gui/kadascoordinatedisplayer.h"
#include "kadas/gui/kadascrsselection.h"
#include "kadas/gui/kadassidepanelhost.h"
#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"
#include "kadas/gui/kadasprojecttemplateselectiondialog.h"
#include "kadas/gui/kadasmapwidget.h"

#include "kadas/gui/catalog/kadasarcgisrestcatalogprovider.h"
#include "kadas/gui/catalog/kadasgeoadminrestcatalogprovider.h"
#include "kadas/gui/catalog/kadasvbscatalogprovider.h"
#include "kadas/gui/catalog/kadasarcgisportalcatalogprovider.h"

#include "kadas/gui/maptools/kadasmaptooldeleteitems.h"
#include "kadas/gui/maptools/kadasmaptoolheightprofile.h"
#include "kadas/gui/maptools/kadasmaptoolhillshade.h"
#include "kadas/gui/maptools/kadasmaptoolmeasure.h"
#include "kadas/gui/maptools/kadasmaptoolslope.h"
#include "kadas/gui/maptools/kadasmaptoolviewshed.h"
#include "kadas/gui/maptools/kadasmaptoolminmax.h"

#include "kadas/gui/search/kadasalternategotolocatorfilter.h"
#include "kadas/gui/search/kadaslocaldatasearchprovider.h"
#include "kadas/gui/search/kadaslocationsearchprovider.h"
#include "kadas/gui/search/kadasmapserverfindsearchprovider.h"
#include "kadas/gui/search/kadaspinsearchprovider.h"
#include "kadas/gui/search/kadasremotedatasearchprovider.h"
#include "kadas/gui/search/kadasworldlocationsearchprovider.h"

#include "kadasapplayerhandling.h"
#include "kadasapplication.h"
#include "kadasbookmarksmenu.h"
#include "kadashelpviewer.h"
#include "kadasgpsintegration.h"
#include "kadasgpxintegration.h"
#include "kadaslayertreeviewmenuprovider.h"
#include "kadaslayertreeviewtemporalindicator.h"
#include "kadasmainwindow.h"
#include "kadasmapwidgetmanager.h"
#include "kadasnewspopup.h"
#include "kadasfeedback.h"
#include "kadastemporalcontroller.h"
#include "kadaspluginmanager.h"
#include "kadaspythonintegration.h"
#include "kadas3dintegration.h"
#include "kadasredliningintegration.h"
#include "kadasribbonsplitbutton.h"
#include "kadasstatusbar.h"
#include "auth/kadasportalauth.h"
#include "bullseye/kadasmaptoolbullseye.h"
#include "guidegrid/kadasmaptoolguidegrid.h"
#include "iamauth/kadasiamauth.h"
#include "kml/kadaskmlintegration.h"
#include "mapgrid/kadasmaptoolmapgrid.h"
#include "milx/kadasmilxintegration.h"


static const QgsSettingsEntryInteger sSettingsLayersWidgetWidth( QStringLiteral( "layers-widget-width" ), KadasSettingsTree::sTreeKadas, 200, QStringLiteral( "Width of the layers side panel." ) );
static const QgsSettingsEntryInteger sSettingsLayersWidgetTab( QStringLiteral( "layers-widget-tab" ), KadasSettingsTree::sTreeKadas, 0, QStringLiteral( "Last-selected layers panel tab." ) );

static bool clipboardHasPastableContent()
{
  // Paste only handles raster images and SVG (see KadasApplication::paste),
  // so enable the action solely for those - not for plain text or other data.
  const QMimeData *mimeData = KadasClipboard::instance()->mimeData();
  return mimeData && ( mimeData->hasImage() || mimeData->hasFormat( "image/svg+xml" ) );
}

KadasMainWindow::KadasMainWindow()
{
  KadasWindowBase::setupUi( this );

  // Wrap the map canvas in a horizontal layout so that reflow side panels
  // (which push the canvas rather than overlay it) can be docked beside it.
  verticalLayoutCentralWidget->removeWidget( mMapCanvas );
  QHBoxLayout *canvasRow = new QHBoxLayout();
  canvasRow->setContentsMargins( 0, 0, 0, 0 );
  canvasRow->setSpacing( 0 );
  mLeftPanelHost = new KadasSidePanelHost( KadasSidePanelHost::Edge::Left );
  mLeftPanelHost->setMapCanvas( mMapCanvas );
  canvasRow->addWidget( mLeftPanelHost, 0 );
  canvasRow->addWidget( mMapCanvas, 1 );
  mRightPanelHost = new KadasSidePanelHost( KadasSidePanelHost::Edge::Right );
  mRightPanelHost->setMapCanvas( mMapCanvas );
  canvasRow->addWidget( mRightPanelHost, 0 );
  verticalLayoutCentralWidget->insertLayout( 0, canvasRow );
}

KadasMainWindow::~KadasMainWindow()
{
  // Delete these explicitly since they access kApp->mainWindow in their destructor
  delete mGpxIntegration;
  delete mKmlIntegration;
  delete mMilxIntegration;
  delete mHelpViewer;
  delete mKadasTemporalController;
}

void KadasMainWindow::init()
{
  // Split from constructor since certain calls may require kApp->mainWindow() to return a constructed instance

  QWidget *topWidget = new QWidget();
  KadasTopWidget::setupUi( topWidget );
  setMenuWidget( topWidget );

  mStatusBar = new KadasStatusBar();
  statusBar()->addPermanentWidget( mStatusBar, 0 );

  mMapCanvas->setFlags( Qgis::MapCanvasFlag::ShowMainAnnotationLayer );
  mMapCanvas->setCanvasColor( Qt::transparent );
  mMapCanvas->enableAntiAliasing( QgsSettings().value( "/kadas/mapAntialiasing", true ).toBool() );
  mMapCanvas->enableMapTileRendering( QgsSettings().value( "/kadas/mapTileRendering", true ).toBool() );
  mMapCanvas->setMapUpdateInterval( QgsSettings().value( "/kadas/mapUpdateInterval", 500 ).toInt() );
  mMapCanvas->setCachingEnabled( QgsSettings().value( "kadas/enableRenderCaching", true ).toBool() );
  mMapCanvas->setParallelRenderingEnabled( QgsSettings().value( "kadas/parallelRendering", true ).toBool() );
  mMapCanvas->setPreviewJobsEnabled( true );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );

  mProjectTemplateDialog = new KadasProjectTemplateSelectionDialog( this );
  connect( mProjectTemplateDialog, &KadasProjectTemplateSelectionDialog::templateSelected, kApp, &KadasApplication::projectCreateFromTemplate );

  mGpsIntegration = new KadasGpsIntegration( this, mStatusBar->gpsButton(), mActionEnableGPS, mActionMoveWithGPS );
  mMapWidgetManager = new KadasMapWidgetManager( mMapCanvas, this );

  QgsLocatorWidget *lw = new QgsLocatorWidget( mMapCanvas );
  lw->setPlaceholderText( tr( "Search for Places, Coordinates, Adresses, ..." ) );
  lw->setResultContainerAnchors( QgsFloatingWidget::AnchorPoint::TopLeft, QgsFloatingWidget::AnchorPoint::BottomLeft );
  lw->findChild<QgsFilterLineEdit *>()->setFixedHeight( 40 );
  mLocatorLayout->insertWidget( 0, lw );

  mLayersWidget->setVisible( false );
  mLayersWidget->setFixedWidth( std::clamp( sSettingsLayersWidgetWidth.value(), 10, 800 ) );

  // The catalog button toggles between the layer tree (unchecked) and the catalog (checked)
  connect( mGeodataTabButton, &QAbstractButton::toggled, this, [this]( bool checked ) {
    mLayersStack->setCurrentIndex( checked ? 1 : 0 );
    sSettingsLayersWidgetTab.setValue( checked ? 1 : 0 );
  } );
  const int layersTab = std::clamp( sSettingsLayersWidgetTab.value(), 0, 1 );
  mLayersStack->setCurrentIndex( layersTab );
  mGeodataTabButton->setChecked( layersTab == 1 );
  QShortcut *layerTreeShortcut = new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_L ), this );
  connect( layerTreeShortcut, &QShortcut::activated, this, &KadasMainWindow::toggleLayerTree );

  // The MilX integration enables the tab, if connection to the MilX server succeeds
  mRibbonWidget->setTabEnabled( mRibbonWidget->indexOf( mMssTab ), false );

  mLanguageCombo->addItem( tr( "System language" ), "" );
  mLanguageCombo->addItem( "English", "en" );
  mLanguageCombo->addItem( "Deutsch", "de" );
  mLanguageCombo->addItem( QString( "Fran%1ais" ).arg( QChar( 0x00E7 ) ), "fr" );
  mLanguageCombo->addItem( "Italiano", "it" );
  bool customLocale = QgsSettings().value( "/locale/overrideFlag", false ).toBool();
  if ( !customLocale )
  {
    mLanguageCombo->setCurrentIndex( 0 );
  }
  else
  {
    QString userLocale = QgsSettings().value( "/locale/userLocale", "" ).toString();
    int idx = mLanguageCombo->findData( userLocale.left( 2 ).toLower() );
    mLanguageCombo->setCurrentIndex( std::max( 0, idx ) );
  }
  connect( mLanguageCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMainWindow::onLanguageChanged );

  mCheckboxIgnoreSystemScaling->setChecked( QgsSettings().value( "/kadas/ignore_dpi_scale", false ).toBool() );
  connect( mCheckboxIgnoreSystemScaling, &QCheckBox::toggled, this, &KadasMainWindow::toggleIgnoreDpiScale );

  mSpinBoxDecimalPlaces->setValue( QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt() );
  connect( mSpinBoxDecimalPlaces, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasMainWindow::onDecimalPlacesChanged );

  mSnappingCheckbox->setChecked( QgsSettings().value( "/kadas/snapping_enabled", false ).toBool() );
  connect( mSnappingCheckbox, &QCheckBox::toggled, this, &KadasMainWindow::onSnappingChanged );

  mInfoBar = new QgsMessageBar( mMapCanvas );
  mInfoBar->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

  mLoadingLabel->adjustSize();
  mLoadingLabel->hide();
  mLoadingTimer.setSingleShot( true );
  mLoadingTimer.setInterval( 500 );

  QMenu *openLayerMenu = new QMenu( this );
  openLayerMenu->addAction( tr( "Add vector layer" ), this, [this] { showSourceSelectDialog( "ogr" ); } );
  openLayerMenu->addAction( tr( "Add raster layer" ), this, [this] { showSourceSelectDialog( "gdal" ); } );
  openLayerMenu->addAction( tr( "Add CSV layer" ), this, [this] { showSourceSelectDialog( "delimitedtext" ); } );
  mOpenLayerButton->setMenu( openLayerMenu );

  QMenu *addServiceMenu = new QMenu( this );
  addServiceMenu->addAction( tr( "Add WMS layer" ), this, [this] { showSourceSelectDialog( "wms" ); } );
  addServiceMenu->addAction( tr( "Add WFS layer" ), this, [this] { showSourceSelectDialog( "WFS" ); } );
  addServiceMenu->addAction( tr( "Add WCS layer" ), this, [this] { showSourceSelectDialog( "wcs" ); } );
  addServiceMenu->addAction( tr( "Add vector tile layer" ), this, [this] { showSourceSelectDialog( "vectortile" ); } );
  addServiceMenu->addAction( tr( "Add XYZ layer" ), this, [this] { showSourceSelectDialog( "xyz" ); } );
  addServiceMenu->addAction( tr( "Add MapServer layer" ), this, [this] { showSourceSelectDialog( "arcgisfeatureserver" ); } );
  mAddServiceButton->setMenu( addServiceMenu );

  mMapCanvas->installEventFilter( this );
  mLayersWidgetResizeHandle->installEventFilter( this );

  QgsDoubleSpinBox *magnifierSpinBox = mStatusBar->magnifierSpinBox();
  mCoordinateDisplayer = new KadasCoordinateDisplayer( mStatusBar->displayCrsButton(), mStatusBar->coordinateEdit(), mStatusBar->heightEdit(), mHeightUnitCombo, mMapCanvas, this );
  mStatusBar->crsSelectionButton()->setMapCanvas( mMapCanvas );
  magnifierSpinBox->setDecimals( 0 );
  magnifierSpinBox->setRange( 100 * QgsGuiUtils::CANVAS_MAGNIFICATION_MIN, 100 * QgsGuiUtils::CANVAS_MAGNIFICATION_MAX );
  magnifierSpinBox->setValue( 100 );
  magnifierSpinBox->setWrapping( false );
  magnifierSpinBox->setSingleStep( 10 );
  magnifierSpinBox->setToolTip( tr( "Magnifier level" ) );
  magnifierSpinBox->setClearValueMode( QgsDoubleSpinBox::CustomValue );
  magnifierSpinBox->setClearValue( 100 * QgsSettings().value( QStringLiteral( "qgis/magnifier_factor_default" ), 1.0 ).toDouble() );

  connect( mStatusBar->scaleCombo(), &QgsScaleComboBox::scaleChanged, this, &KadasMainWindow::setMapScale );
  connect( mStatusBar->scaleLockButton(), &QToolButton::toggled, this, &KadasMainWindow::toggleScaleLock );
  connect( magnifierSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasMainWindow::setMapMagnifier );
  connect( mMapCanvas, &QgsMapCanvas::magnificationChanged, [this]( double value ) {
    QgsDoubleSpinBox *spinBox = mStatusBar->magnifierSpinBox();
    spinBox->blockSignals( true );
    spinBox->setValue( value * 100 );
    spinBox->blockSignals( false );
  } );

  mNumericInputCheckbox->setChecked( KadasSettingsTree::settingsShowNumericInput->value() );
  connect( mNumericInputCheckbox, &QCheckBox::toggled, this, &KadasMainWindow::onNumericInputCheckboxToggled );

  QgsLayerTreeModel *model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
  model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
  model->setFlag( QgsLayerTreeModel::AllowNodeRename );
  model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
  model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
  model->setFlag( QgsLayerTreeModel::UseTextFormatting );
  model->setAutoCollapseLegendNodes( 1 );

  mLayerTreeView->setModel( model );
  mLayerTreeView->setMenuProvider( new KadasLayerTreeViewMenuProvider( mLayerTreeView ) );
  //connect( QgsProject::instance(), &QgsProject::layersRemoved, this, &KadasLayerSelectionWidget::repopulateLayers );
  connect( mLayerTreeView, &QAbstractItemView::doubleClicked, this, &KadasMainWindow::layerTreeViewDoubleClicked );

  QgsSnappingConfig snappingConfig;
  snappingConfig.setMode( Qgis::SnappingMode::AllLayers );
  snappingConfig.setTypeFlag( Qgis::SnappingType::Vertex );
  int snappingRadius = QgsSettings().value( "/kadas/snapping_radius", 10 ).toInt();
  snappingConfig.setTolerance( snappingRadius );
  snappingConfig.setUnits( Qgis::MapToolUnit::Pixels );
  snappingConfig.setEnabled( true );
  mMapCanvas->snappingUtils()->setConfig( snappingConfig );

  // KML
  mKmlIntegration = new KadasKmlIntegration( mKMLButton, this );

  // Redlining
  mRedliningIntegration = new KadasRedliningIntegration( this );

  // GPX routes
  mGpxIntegration = new KadasGpxIntegration( mActionDrawWaypoint, mActionDrawRoute, mActionExportGPX, mActionImportGPX, this );

  // Milx
  KadasMilxIntegration::MilxUi milxUi;
  milxUi.mRibbonWidget = mRibbonWidget;
  milxUi.mMssTab = mMssTab;
  milxUi.mActionMilx = mActionMilx;
  milxUi.mActionSaveMilx = mActionSaveMilx;
  milxUi.mActionMilxKmlExport = mActionMilxKmlExport;
  milxUi.mActionLoadMilx = mActionLoadMilx;
  milxUi.mSymbolSizeSlider = mSymbolSizeSlider;
  milxUi.mLineWidthSlider = mLineWidthSlider;
  milxUi.mWorkModeCombo = mWorkModeCombo;
  milxUi.mLeaderLineWidthSpin = mLeaderLineWidthSpin;
  milxUi.mLeaderLineColorButton = mLeaderLineColorButton;
  mMilxIntegration = new KadasMilxIntegration( milxUi, this );

  // IAM Auth
  KadasIamAuth *iamAuth = new KadasIamAuth( mLoginButton, mLogoutButton, mCatalogBrowser->refreshButton(), this );
  Q_UNUSED( iamAuth );
  // The catalog login toolbar only carries the (hidden unless login is configured)
  // login/logout buttons and username label; collapse it entirely when unused so
  // it does not reserve empty space above the catalog filter.
  if ( mLoginButton->isHidden() && mLogoutButton->isHidden() )
  {
    mGeodataToolbar->hide();
  }

  Kadas3DIntegration *my3Dintegration = new Kadas3DIntegration( mAction3D, mMapCanvas, this );

  // Help file server
  mHelpViewer = new KadasHelpViewer( this );

  // Temporal controller
  mKadasTemporalController = new KadasTemporalController( mapCanvas() );
  mKadasTemporalController->hide();
  new KadasLayerTreeViewTemporalIndicator( mLayerTreeView, mKadasTemporalController ); // gets parented to the layer view

  // Plugin manager
  mPluginManager = new KadasPluginManager( mapCanvas(), mActionPluginManager );
  mPluginManager->hide();

  setElevationControllerRangeFromHeightmap();


  configureButtons();

  restoreFavoriteButton( mFavoriteButton1 );
  restoreFavoriteButton( mFavoriteButton2 );
  restoreFavoriteButton( mFavoriteButton3 );
  restoreFavoriteButton( mFavoriteButton4 );
  connect( mFavoriteButton1, &KadasRibbonButton::contextMenuRequested, this, &KadasMainWindow::showFavoriteContextMenu );
  connect( mFavoriteButton2, &KadasRibbonButton::contextMenuRequested, this, &KadasMainWindow::showFavoriteContextMenu );
  connect( mFavoriteButton3, &KadasRibbonButton::contextMenuRequested, this, &KadasMainWindow::showFavoriteContextMenu );
  connect( mFavoriteButton4, &KadasRibbonButton::contextMenuRequested, this, &KadasMainWindow::showFavoriteContextMenu );

  connect( mMapCanvas, &QgsMapCanvas::layersChanged, this, &KadasMainWindow::checkOnTheFlyProjection );
  connect( mMapCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasMainWindow::checkOnTheFlyProjection );
  connect( mMapCanvas, &QgsMapCanvas::scaleChanged, this, &KadasMainWindow::showScale );
  connect( mMapCanvas, &QgsMapCanvas::mapToolSet, this, &KadasMainWindow::switchToTabForTool );
  connect( mMapCanvas, &QgsMapCanvas::renderStarting, &mLoadingTimer, qOverload<>( &QTimer::start ) );
  connect( mMapCanvas, &QgsMapCanvas::mapCanvasRefreshed, &mLoadingTimer, &QTimer::stop );
  connect( mMapCanvas, &QgsMapCanvas::mapCanvasRefreshed, mLoadingLabel, &QLabel::hide );
  connect( mMapCanvas, &QgsMapCanvas::currentLayerChanged, mLayerTreeView, &QgsLayerTreeView::setCurrentLayer );
  // The layer tree view accepts dataset (e.g. catalog entry) drops itself and
  // just announces them; add the dropped layer like a window-level drop.
  connect( mLayerTreeView, &QgsLayerTreeView::datasetsDropped, this, [this]( QDropEvent *event ) {
    const QgsMimeDataUtils::UriList list = QgsMimeDataUtils::decodeUriList( event->mimeData() );
    if ( !list.isEmpty() )
    {
      addCatalogLayer( list.front(), event->mimeData()->property( "metadataUrl" ).toString(), event->mimeData()->property( "sublayers" ).toList(), true );
    }
  } );
  connect( mMapCanvas, &QgsMapCanvas::layersChanged, this, &KadasMainWindow::updateBgLayerZoomResolutions );
  connect( mMapCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasMainWindow::updateBgLayerZoomResolutions );
  connect( &mLoadingTimer, &QTimer::timeout, mLoadingLabel, &QLabel::show );
  connect( mRibbonWidget, &QTabWidget::currentChanged, [this] { mMapCanvas->unsetMapTool( mMapCanvas->mapTool() ); } ); // Clear tool when changing active kadas tab
  connect( mZoomInButton, &QPushButton::clicked, this, &KadasMainWindow::zoomIn );
  connect( mZoomOutButton, &QPushButton::clicked, this, &KadasMainWindow::zoomOut );
  connect( mHomeButton, &QPushButton::clicked, this, &KadasMainWindow::zoomFull );
  connect( KadasClipboard::instance(), &KadasClipboard::dataChanged, [this] { mActionPaste->setEnabled( clipboardHasPastableContent() ); } );
  connect( QgsProject::instance(), &QgsProject::layerWasAdded, this, &KadasMainWindow::checkLayerProjection );
  connect( QgsProject::instance(), &QgsProject::layerWasAdded, this, &KadasMainWindow::checkLayerTemporalCapabilities );
  connect( QgsProject::instance(), &QgsProject::layerWasAdded, this, &KadasMainWindow::checkWMSLayerIgnoreReportedExtents );
  connect( QgsProject::instance(), &QgsProject::layersRemoved, this, &KadasMainWindow::removeElevationControllers );
  connect( mLayerTreeViewButton, &QPushButton::clicked, this, &KadasMainWindow::toggleLayerTree );
  connect( mRibbonbarButton, &QPushButton::clicked, this, &KadasMainWindow::toggleFullscreen );
  connect( mRibbonWidget, &QTabWidget::tabBarClicked, this, &KadasMainWindow::endFullscreen );


  QStringList catalogUris = QgsSettings().value( "/kadas/geodatacatalogs" ).toString().split( ";;" );

  for ( const QString &catalogUri : catalogUris )
  {
    QUrlQuery query( QUrl::fromEncoded( "?" + catalogUri.toLocal8Bit() ) );
    QString type = query.queryItemValue( "type" );
    QString url = query.queryItemValue( "url" );
    QMap<QString, QString> params;
    for ( const auto &pair : query.queryItems() )
    {
      params.insert( pair.first, pair.second );
    }
    if ( type == "geoadmin" )
    {
      mCatalogBrowser->addProvider( new KadasGeoAdminRestCatalogProvider( url, mCatalogBrowser, params ) );
    }
    else if ( type == "arcgisrest" )
    {
      mCatalogBrowser->addProvider( new KadasArcGisRestCatalogProvider( url, mCatalogBrowser, params ) );
    }
    else if ( type == "vbs" )
    {
      KadasVBSCatalogProvider *vbsprovider = new KadasVBSCatalogProvider( url, mCatalogBrowser, params );
      mCatalogBrowser->addProvider( vbsprovider );
    }
    else if ( type == "arcgisportal" )
    {
      KadasArcGisPortalCatalogProvider *portalprovider = new KadasArcGisPortalCatalogProvider( url, mCatalogBrowser, params, KadasPortalAuth::ESRI_AUTH_CFG_ID );
      mCatalogBrowser->addProvider( portalprovider );
    }
  }

  connect( mCatalogBrowser, &KadasCatalogBrowser::layerSelected, this, [this]( const QgsMimeDataUtils::Uri &uri, const QString &metadataUrl, const QVariantList &sublayers ) {
    addCatalogLayer( uri, metadataUrl, sublayers );
    // Reveal the layer tree so the freshly added (and selected) layer is visible
    mGeodataTabButton->setChecked( false );
  } );
  // Dragging a catalog entry: reveal the layer tree so it can serve as drop target.
  connect( mCatalogBrowser, &KadasCatalogBrowser::dragStarted, this, [this] { mGeodataTabButton->setChecked( false ); } );

  const QList<QgsLocatorFilter *> filters = lw->locator()->filters();
  for ( QgsLocatorFilter *filter : filters )
  {
    if ( filter->name() == QStringLiteral( "filters" ) )
    {
      // removes the top locator filter (the filter of locator filters)
      lw->locator()->deregisterFilter( filter );
      break;
    }
  }

  lw->setMapCanvas( mMapCanvas );
  lw->locator()->registerFilter( new KadasAlternateGotoLocatorFilter( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasLocalDataSearchFilter( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasLocationSearchFilter( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasMapServerFindSearchProvider( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasPinSearchProvider( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasRemoteDataSearchProvider( mMapCanvas ) );
  lw->locator()->registerFilter( new KadasWorldLocationSearchProvider( mMapCanvas ) );

  mActionShowPythonConsole = new QAction( this );
  QShortcut *pythonConsoleShortcut = new QShortcut( QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_P ), this );
  connect( pythonConsoleShortcut, &QShortcut::activated, kApp, &KadasApplication::showPythonConsole );

  QShortcut *networkLoggerShortcut = new QShortcut( QKeySequence( Qt::Key_F12 ), this );
  connect( networkLoggerShortcut, &QShortcut::activated, kApp, &KadasApplication::showNetworkLogger );
  QShortcut *alternativeNetworkLoggerShortcut = new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_F12 ), this );
  connect( alternativeNetworkLoggerShortcut, &QShortcut::activated, kApp, &KadasApplication::showNetworkLogger );

  // Restore geometry
  restoreGeometry( QgsSettings().value( "/kadas/windowgeometry" ).toByteArray() );
}

bool KadasMainWindow::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mMapCanvas && ev->type() == QEvent::Resize )
  {
    updateWidgetPositions();
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseButtonPress )
  {
    QMouseEvent *e = static_cast<QMouseEvent *>( ev );
    if ( e->button() == Qt::LeftButton )
    {
      mResizePressPos = e->pos();
      // Freeze the map under a snapshot for the whole drag; a single
      // re-render happens on release.
      mLeftPanelHost->beginPanelResize();
    }
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseMove )
  {
    QMouseEvent *e = static_cast<QMouseEvent *>( ev );
    if ( e->buttons() == Qt::LeftButton )
    {
      QPoint delta = e->pos() - mResizePressPos;
      mLayersWidget->setFixedWidth( std::clamp( mLayersWidget->width() + delta.x(), 10, 800 ) );
    }
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseButtonRelease )
  {
    QMouseEvent *e = static_cast<QMouseEvent *>( ev );
    if ( e->button() == Qt::LeftButton )
    {
      mLeftPanelHost->endPanelResize();
      sSettingsLayersWidgetWidth.setValue( mLayersWidget->width() );
    }
  }
  return false;
}

void KadasMainWindow::updateWidgetPositions()
{
  // Make sure +/- buttons have constant distance to upper right corner of map canvas
  int distanceToRightBorder = 9;
  int distanceToTop = 60;
  mZoomInOutFrame->move( mMapCanvas->width() - distanceToRightBorder - mZoomInOutFrame->width(), distanceToTop );

  mHomeButton->move( mMapCanvas->width() - distanceToRightBorder - mHomeButton->height(), distanceToTop + 90 );

  mElevationControllerFrame->move( mMapCanvas->width() - distanceToRightBorder - mElevationControllerFrame->width(), distanceToTop + 150 );

  // Reposition mRibbonbarButton
  mRibbonbarButton->move( 0.5 * mMapCanvas->width() - 0.5 * mRibbonbarButton->width(), 0 );

  // Resize info bar
  double barwidth = 0.5 * mMapCanvas->width();
  double x = 0.5 * mMapCanvas->width() - 0.5 * barwidth;
  double y = mMapCanvas->y();
  mInfoBar->move( x, y );
  mInfoBar->setFixedWidth( barwidth );

  // Move loading label
  mLoadingLabel->move( mMapCanvas->width() - 5 - mLoadingLabel->width(), mMapCanvas->height() - 5 - mLoadingLabel->height() );
}

void KadasMainWindow::mousePressEvent( QMouseEvent *event )
{
  if ( event->buttons() == Qt::LeftButton )
  {
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->position().toPoint() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) )
    {
      mDragStartPos = event->position().toPoint();
    }
  }
}

void KadasMainWindow::mouseMoveEvent( QMouseEvent *event )
{
  if ( event->buttons() == Qt::LeftButton && !mDragStartPos.isNull() && ( mDragStartPos - event->position().toPoint() ).manhattanLength() >= QApplication::startDragDistance() )
  {
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->position().toPoint() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) && button->defaultAction() )
    {
      QMimeData *mimeData = new QMimeData();
      mimeData->setData( "application/qgis-kadas-button", button->defaultAction()->objectName().toLocal8Bit() );
      QDrag *drag = new QDrag( this );
      drag->setMimeData( mimeData );
      drag->setPixmap( button->icon().pixmap( 32, 32 ) );
      drag->setHotSpot( QPoint( 16, 16 ) );
      drag->exec( Qt::CopyAction );
      mDragStartPos = QPoint();
    }
  }
}

void KadasMainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  for ( QgsCustomDropHandler *handler : mCustomDropHandlers )
  {
    if ( handler && handler->canHandleMimeData( event->mimeData() ) )
    {
      event->acceptProposedAction();
      return;
    }
  }

  KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->position().toPoint() ) );
  if ( event->mimeData()->hasFormat( "application/qgis-kadas-button" ) && button && button->objectName().startsWith( "mFavoriteButton" ) )
  {
    event->acceptProposedAction();
  }
  else if ( event->mimeData()->hasFormat( "application/x-vnd.qgis.qgis.uri" ) )
  {
    event->acceptProposedAction();
  }
  else if ( event->mimeData()->hasUrls() )
  {
    event->acceptProposedAction();
  }
}

void KadasMainWindow::dropEvent( QDropEvent *event )
{
  for ( QgsCustomDropHandler *handler : mCustomDropHandlers )
  {
    if ( handler->handleMimeDataV2( event->mimeData() ) )
    {
      return;
    }
  }

  if ( event->mimeData()->hasFormat( "application/qgis-kadas-button" ) )
  {
    QString actionName = QString::fromLocal8Bit( event->mimeData()->data( "application/qgis-kadas-button" ).data() );
    QAction *action = findChild<QAction *>( actionName );
    if ( !action )
    {
      action = mAddedActions.value( actionName, nullptr );
    }
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->position().toPoint() ) );
    if ( action && button && button->objectName().startsWith( "mFavoriteButton" ) )
    {
      button->setEnabled( true );
      setActionToButton( action, button );
      QgsSettings().setValue( "/kadas/favoriteAction/" + button->objectName(), actionName );
    }
  }
  else if ( event->mimeData()->hasFormat( "application/x-vnd.qgis.qgis.uri" ) )
  {
    QgsMimeDataUtils::UriList list = QgsMimeDataUtils::decodeUriList( event->mimeData() );
    if ( !list.isEmpty() )
    {
      QString metadataUrl = event->mimeData()->property( "metadataUrl" ).toString();
      QVariantList sublayers = event->mimeData()->property( "sublayers" ).toList();
      addCatalogLayer( list.front(), metadataUrl, sublayers );
    }
  }
  else
  {
    QSet<QString> mapItemFormats;
    for ( const QByteArray &format : QImageReader::supportedImageFormats() )
    {
      mapItemFormats.insert( QString( "%1" ).arg( QString( format ).toLower() ) );
    }
    mapItemFormats.insert( ".svg" ); // Ensure svg is present
    mapItemFormats.remove( ".tif" ); // Remove tif to ensure it is opened as raster layer

    static QMap<QString, QString> worldFileSuffixes = { { "gif", "gfw" }, { "jpg", "jgw" }, { "jp2", "j2w" }, { "png", "pgw" }, { "tif", "tfw" }, { "tiff", "tfw" } };

    for ( const QUrl &url : event->mimeData()->urls() )
    {
      QString fileName = url.toLocalFile();
      if ( fileName.isEmpty() )
      {
        continue;
      }

      QFileInfo finfo( fileName );
      QString suffix = finfo.suffix();
      bool addAsMapItem = mapItemFormats.contains( suffix );
      // Don't add as map item if a matching world file exists
      if ( worldFileSuffixes.contains( suffix ) )
      {
        QString worldFileName = finfo.dir().absoluteFilePath( QString( "%1.%2" ).arg( finfo.baseName() ).arg( worldFileSuffixes.value( suffix ) ) );
        if ( QFile( worldFileName ).exists() )
        {
          addAsMapItem = false;
        }
      }
      // Don't add as map item if file is a geotiff with georeferencing
      if ( suffix.startsWith( "tif" ) )
      {
        GDALDatasetH ds = GDALOpen( fileName.toUtf8().data(), GA_ReadOnly );
        double gtrans[6] = {};
        CPLErr err = GDALGetGeoTransform( ds, gtrans );
        if ( err == CE_None )
        {
          addAsMapItem = false;
        }
        GDALClose( ds );
      }
      if ( addAsMapItem )
      {
        kApp->addImageItem( fileName );
      }
      else
      {
        bool ok = false;
        KadasAppLayerHandling::openLayer( fileName, ok, true, true );
      }
    }
  }
}

void KadasMainWindow::showEvent( QShowEvent * )
{
  mGpsIntegration->initGui();
}

void KadasMainWindow::closeEvent( QCloseEvent *ev )
{
  if ( !kApp->projectSaveDirty() )
  {
    ev->ignore();
    return;
  }
  kApp->projectClose();
  QgsSettings().setValue( "/kadas/windowgeometry", saveGeometry() );
  emit closed();
}

void KadasMainWindow::restoreFavoriteButton( QToolButton *button )
{
  QString actionName = QgsSettings().value( "/kadas/favoriteAction/" + button->objectName() ).toString();
  if ( actionName.isEmpty() )
  {
    return;
  }

  QAction *action = findChild<QAction *>( actionName );
  if ( action )
  {
    setActionToButton( action, button );
  }
}

void KadasMainWindow::configureButtons()
{
  // Map tab

  setActionToButton( mActionNew, mNewButton, QKeySequence( Qt::CTRL | Qt::Key_N ) );
  connect( mActionNew, &QAction::triggered, this, &KadasMainWindow::showProjectSelectionWidget );

  setActionToButton( mActionOpen, mOpenButton, QKeySequence( Qt::CTRL | Qt::Key_O ) );
  connect( mActionOpen, &QAction::triggered, kApp, [] { kApp->projectOpen(); } );

  setActionToButton( mActionSave, mSaveButton, QKeySequence( Qt::CTRL | Qt::Key_S ) );
  connect( mActionSave, &QAction::triggered, kApp, [] { kApp->projectSave(); } );

  setActionToButton( mActionSaveAs, mSaveAsButton, QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_S ) );
  connect( mActionSaveAs, &QAction::triggered, kApp, [] { kApp->projectSave( QString(), true ); } );

  setActionToButton( mActionPrint, mPrintButton, QKeySequence( Qt::CTRL | Qt::Key_P ) );
  // signal connected by plugin

  setActionToButton( mActionCopy, mCopyButton );
  connect( mActionCopy, &QAction::triggered, kApp, &KadasApplication::saveMapToClipboard );

  connect( mActionSaveMapExtent, &QAction::triggered, kApp, &KadasApplication::saveMapAsImage );
  setActionToButton( mActionSaveMapExtent, mSaveMapExtentButton );

  // View tab
  setActionToButton( mActionZoomLast, mZoomLastButton, QKeySequence( Qt::CTRL | Qt::Key_PageUp ) );
  connect( mActionZoomLast, &QAction::triggered, this, &KadasMainWindow::zoomPrev );
  connect( mMapCanvas, &QgsMapCanvas::zoomLastStatusChanged, mActionZoomLast, &QAction::setEnabled );

  setActionToButton( mActionZoomNext, mZoomNextButton, QKeySequence( Qt::CTRL | Qt::Key_PageDown ) );
  connect( mActionZoomNext, &QAction::triggered, this, &KadasMainWindow::zoomNext );
  connect( mMapCanvas, &QgsMapCanvas::zoomNextStatusChanged, mActionZoomNext, &QAction::setEnabled );

  setActionToButton( mActionBookmarks, mBookmarksButton );
  mBookmarksButton->setMenu( new KadasBookmarksMenu( mMapCanvas, messageBar(), this ) );
  mBookmarksButton->setPopupMode( QToolButton::InstantPopup );

  setActionToButton( mActionNewMapWindow, mNewMapWindowButton, QKeySequence( Qt::CTRL | Qt::Key_W, Qt::CTRL | Qt::Key_N ) );
  connect( mActionNewMapWindow, &QAction::triggered, mMapWidgetManager, qOverload<>( &KadasMapWidgetManager::addMapWidget ) );

  setActionToButton( mAction3D, m3DButton, QKeySequence( Qt::CTRL | Qt::Key_W, Qt::CTRL | Qt::Key_3 ) );
  // signal connected by plugin

  setActionToButton( mActionGrid, mGridButton, QKeySequence( Qt::CTRL | Qt::Key_W, Qt::CTRL | Qt::Key_G ), [this] {
    return new KadasMapToolMapGrid( mMapCanvas, mLayerTreeView, mLayerTreeView->currentLayer() );
  } );

  // Draw tab
  // Three split buttons: Markers, Shapes, Others. Each remembers its last
  // used tool (persisted) and exposes the remaining tools in a drop-down menu.
  static const QgsSettingsEntryString sLastMarkerTool( QStringLiteral( "draw-last-marker-tool" ), KadasSettingsTree::sTreeKadas, QStringLiteral( "draw-marker-circle" ) );
  static const QgsSettingsEntryString sLastShapeTool( QStringLiteral( "draw-last-shape-tool" ), KadasSettingsTree::sTreeKadas, QStringLiteral( "draw-line" ) );
  static const QgsSettingsEntryString sLastOtherTool( QStringLiteral( "draw-last-other-tool" ), KadasSettingsTree::sTreeKadas, QStringLiteral( "mActionPin" ) );

  KadasRibbonSplitButton *markersSplit = new KadasRibbonSplitButton( mToolButtonMarkers, tr( "Markers" ), QIcon( ":/kadas/icons/draw_point" ), &sLastMarkerTool, this );
  const QList<QAction *> markerActions = mRedliningIntegration->markerActions();
  for ( QAction *action : markerActions )
    markersSplit->addAction( action );
  markersSplit->finish();

  KadasRibbonSplitButton *shapesSplit = new KadasRibbonSplitButton( mToolButtonShapes, tr( "Shapes" ), QIcon( ":/kadas/icons/draw_polygon" ), &sLastShapeTool, this );
  const QList<QAction *> shapeActions = mRedliningIntegration->shapeActions();
  for ( QAction *action : shapeActions )
    shapesSplit->addAction( action );
  shapesSplit->finish();

  // Pin tool: checkable, mutually exclusive with the redlining tools.
  mActionPin->setObjectName( QStringLiteral( "mActionPin" ) );
  mRedliningIntegration->actionGroup()->addAction( mActionPin );
  connect( mActionPin, &QAction::toggled, this, [this]( bool active ) {
    if ( active )
    {
      mMapCanvas->unsetMapTool( mapCanvas()->mapTool() );
      QgsMapTool *tool = addPinTool();
      if ( tool )
      {
        tool->setAction( mActionPin );
        connect( tool, &QgsMapTool::deactivated, mActionPin, [this] { mActionPin->setChecked( false ); } );
        mMapCanvas->setMapTool( tool );
      }
      else
      {
        mActionPin->setChecked( false );
      }
    }
    else if ( mMapCanvas->mapTool() && mMapCanvas->mapTool()->action() == mActionPin )
    {
      mMapCanvas->unsetMapTool( mapCanvas()->mapTool() );
    }
  } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_M ), this ), &QShortcut::activated, mActionPin, &QAction::trigger );

  // Image: opens a sub-menu offering "From file" / "From URL". It is a one-shot
  // command (not a checkable tool), so it never becomes the split button's
  // remembered tool nor leaves it stuck in an activated state when the file
  // dialog is cancelled. The sub-menu lives at the gallery-tile level, so the
  // ribbon keeps a single drop-down arrow.
  QAction *actionAddImage = new QAction( QIcon( ":/kadas/icons/picture" ), tr( "Image" ), this );
  actionAddImage->setObjectName( QStringLiteral( "draw-image" ) );
  QMenu *imageMenu = new QMenu( this );
  imageMenu->addAction( tr( "From file…" ), this, &KadasMainWindow::addLocalPicture );
  imageMenu->addAction( tr( "From URL…" ), this, &KadasMainWindow::addRemotePicture );
  actionAddImage->setMenu( imageMenu );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_I ), this ), &QShortcut::activated, this, &KadasMainWindow::addLocalPicture );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_U ), this ), &QShortcut::activated, this, &KadasMainWindow::addRemotePicture );

  KadasRibbonSplitButton *othersSplit = new KadasRibbonSplitButton( mToolButtonOthers, tr( "Others" ), QIcon( ":/kadas/icons/draw_pin" ), &sLastOtherTool, this );
  othersSplit->addAction( mActionPin );
  othersSplit->addAction( actionAddImage );
  othersSplit->addAction( mRedliningIntegration->actionNewText() );
  othersSplit->addAction( mRedliningIntegration->actionNewTextAlongLine() );
  othersSplit->addAction( mRedliningIntegration->actionNewCoordinateCross() );
  othersSplit->finish();

  setActionToButton( mActionGuideGrid, mGuideGridButton, QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_G ), [this] {
    return new KadasMapToolGuideGrid( mMapCanvas, mLayerTreeView, mLayerTreeView->currentLayer() );
  } );

  setActionToButton( mActionBullseye, mBullseyeButton, QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_B ), [this] {
    return new KadasMapToolBullseye( mMapCanvas, mLayerTreeView, mLayerTreeView->currentLayer() );
  } );

  setActionToButton( mActionPaste, mPasteButton, QKeySequence( Qt::CTRL | Qt::Key_V ), [] { return kApp->paste(); } );
  mActionPaste->setEnabled( clipboardHasPastableContent() );

  setActionToButton( mActionDeleteItems, mDeleteItemsButton, QKeySequence(), [this] { return new KadasMapToolDeleteItems( mapCanvas() ); } );

  // Analysis tab
  setActionToButton( mActionDistance, mDistanceButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_D ), [this] {
    return new KadasMapToolMeasure( mapCanvas(), KadasMapToolMeasure::MeasureMode::MeasureLine );
  } );

  setActionToButton( mActionArea, mAreaButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_A ), [this] {
    return new KadasMapToolMeasure( mapCanvas(), KadasMapToolMeasure::MeasureMode::MeasurePolygon );
  } );

  setActionToButton( mActionCircle, mMeasureCircleButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_C ), [this] {
    return new KadasMapToolMeasure( mapCanvas(), KadasMapToolMeasure::MeasureMode::MeasureCircle );
  } );

  setActionToButton( mActionProfile, mProfileButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_P ), [this] { return new KadasMapToolHeightProfile( mapCanvas() ); } );

  setActionToButton( mActionSlope, mSlopeButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_S ), [this] { return new KadasMapToolSlope( mapCanvas() ); } );

  setActionToButton( mActionHillshade, mHillshadeButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_H ), [this] { return new KadasMapToolHillshade( mapCanvas() ); } );

  setActionToButton( mActionViewshed, mViewshedButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_V ), [this] { return new KadasMapToolViewshed( mapCanvas() ); } );
  setActionToButton( mActionMinMax, mMinMaxButton, QKeySequence( Qt::CTRL | Qt::Key_A, Qt::CTRL | Qt::Key_M ), [this] { return new KadasMapToolMinMax( mapCanvas(), mActionViewshed, mActionProfile ); } );

  // GPS tab
  setActionToButton( mActionEnableGPS, mEnableGPSButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_T ) );
  setActionToButton( mActionMoveWithGPS, mMoveWithGPSButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_M ) );

  setActionToButton( mActionDrawWaypoint, mDrawWaypointButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_W ) );
  setActionToButton( mActionDrawRoute, mDrawRouteButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_R ) );
  setActionToButton( mActionImportGPX, mGpxImportButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_I ) );
  setActionToButton( mActionExportGPX, mGpxExportButton, QKeySequence( Qt::CTRL | Qt::Key_G, Qt::CTRL | Qt::Key_E ) );

  // MSS tab
  setActionToButton( mActionMilx, mMilxButton, QKeySequence( Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_S ) );
  setActionToButton( mActionSaveMilx, mSaveMilxButton, QKeySequence( Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_E ) );
  setActionToButton( mActionMilxKmlExport, mMilxKmlExportButton, QKeySequence( Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_K ) );
  setActionToButton( mActionLoadMilx, mLoadMilxButton, QKeySequence( Qt::CTRL | Qt::Key_M, Qt::CTRL | Qt::Key_I ) );

  // Settings tab
  setActionToButton( mActionPluginManager, mPluginManagerButton, QKeySequence( Qt::CTRL | Qt::Key_S, Qt::CTRL | Qt::Key_P ) );
  connect( mActionPluginManager, &QAction::toggled, this, &KadasMainWindow::showPluginManager );
  connect( mRibbonWidget, &QTabWidget::currentChanged, [this] { mActionPluginManager->setChecked( false ); } );

  //help tab
  setActionToButton( mActionHelp, mHelpButton );
  connect( mActionHelp, &QAction::triggered, this, &KadasMainWindow::showHelp );

  setActionToButton( mActionAbout, mAboutButton );
  if ( KadasNewsPopup::isConfigured() )
  {
    setActionToButton( mActionNewsletter, mNewsletterButton );
    connect( mActionNewsletter, &QAction::triggered, this, &KadasMainWindow::showNewsletter );
  }
  else
  {
    mNewsletterButton->hide();
  }

  if ( KadasFeedback::isConfigured() )
  {
    setActionToButton( mActionFeedback, mFeedbackButton );
    connect( mActionFeedback, &QAction::triggered, this, &KadasMainWindow::showFeedback );
  }
  else
  {
    mFeedbackButton->hide();
  }
}

void KadasMainWindow::setActionToButton( QAction *action, QToolButton *button, const QKeySequence &shortcut, const std::function<QgsMapTool *()> &toolFactory )
{
  button->setDefaultAction( action );
  button->setIconSize( QSize( 32, 32 ) );
  if ( toolFactory )
  {
    button->setCheckable( action->isCheckable() );
    if ( button->isCheckable() )
    {
      connect( action, &QAction::toggled, this, [this, toolFactory, action]( bool active ) {
        if ( active )
        {
          mMapCanvas->unsetMapTool( mapCanvas()->mapTool() );
          QgsMapTool *tool = toolFactory();
          if ( tool )
          {
            tool->setAction( action );
            mMapCanvas->setMapTool( tool );
          }
          else
          {
            action->setChecked( false );
          }
        }
        else if ( mMapCanvas->mapTool() && mMapCanvas->mapTool()->action() == action )
        {
          mMapCanvas->unsetMapTool( mapCanvas()->mapTool() );
        }
      } );
    }
    else
    {
      connect( action, &QAction::triggered, this, [this, toolFactory] { mMapCanvas->setMapTool( toolFactory() ); } );
    }
  }
  if ( !shortcut.isEmpty() )
  {
    connect( new QShortcut( shortcut, this ), &QShortcut::activated, action, &QAction::trigger );
  }
}

QWidget *KadasMainWindow::addRibbonTab( const QString &name )
{
  QWidget *widget = new QWidget( mRibbonWidget );
  widget->setLayout( new QHBoxLayout() );
  widget->layout()->setContentsMargins( 0, 0, 0, 0 );
  widget->layout()->setSpacing( 6 );
  widget->layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );
  mRibbonWidget->addTab( widget, name );
  return widget;
}

KadasRibbonButton *KadasMainWindow::addRibbonButton( QWidget *tabWidget )
{
  KadasRibbonButton *button = new KadasRibbonButton();
  button->setObjectName( QUuid::createUuid().toString() );
  button->setMinimumSize( QSize( 0, 80 ) );
  button->setMaximumSize( QSize( 80, 80 ) );
  button->setIconSize( QSize( 32, 32 ) );

  QSizePolicy sizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
  sizePolicy.setHorizontalStretch( 0 );
  sizePolicy.setVerticalStretch( 0 );
  sizePolicy.setHeightForWidth( button->sizePolicy().hasHeightForWidth() );
  button->setSizePolicy( sizePolicy );

  QHBoxLayout *layout = dynamic_cast<QHBoxLayout *>( tabWidget->layout() );
  layout->insertWidget( layout->count() - 1, button );
  return button;
}

void KadasMainWindow::addActionToTab( QAction *action, QWidget *tabWidget )
{
  KadasRibbonButton *button = addRibbonButton( tabWidget );
  button->setText( action->text() );
  setActionToButton( action, button, QKeySequence() );

  if ( action->objectName().isEmpty() )
  {
    action->setObjectName( QUuid::createUuid().toString() );
  }
  mAddedActions.insert( action->objectName(), action );
}

void KadasMainWindow::addMenuButtonToTab( const QString &text, const QIcon &icon, QMenu *menu, QWidget *tabWidget )
{
  KadasRibbonButton *button = addRibbonButton( tabWidget );
  button->setText( text );
  button->setIcon( icon );
  button->setMenu( menu );
  button->setPopupMode( QToolButton::InstantPopup );
}

void KadasMainWindow::removeActionFromTab( QAction *action, QWidget *tabWidget )
{
  for ( QObject *obj : tabWidget->children() )
  {
    KadasRibbonButton *b = dynamic_cast<KadasRibbonButton *>( obj );
    if ( b && b->defaultAction() == action )
    {
      mAddedActions.remove( action->objectName() );
      delete b;
      return;
    }
  }
}

void KadasMainWindow::removeMenuButtonFromTab( QMenu *menu, QWidget *tabWidget )
{
  for ( QObject *obj : tabWidget->children() )
  {
    KadasRibbonButton *b = dynamic_cast<KadasRibbonButton *>( obj );
    if ( b && b->menu() == menu )
    {
      delete b;
      return;
    }
  }
}

QMenu *KadasMainWindow::pluginsMenu()
{
  if ( !mPluginsToolButton )
  {
    mPluginsToolButton = new QToolButton();
    mPluginsToolButton->setObjectName( "mPluginsToolButton" );
    mPluginsToolButton->setMenu( new QMenu() );
    mPluginsToolButton->setPopupMode( QToolButton::InstantPopup );
    mRibbonWidget->setCornerWidget( mPluginsToolButton );
  }
  return mPluginsToolButton->menu();
}

void KadasMainWindow::toggleLayerTree()
{
  if ( mLayersWidget->isVisible() )
  {
    mLeftPanelHost->removePanel( mLayersWidget );
    mLayersWidget->setVisible( false );
    mLayerTreeViewButton->setIcon( QIcon( ":/kadas/icons/layertree_folded" ) );
    mLayerTreeViewButton->setFixedSize( 40, 40 );
  }
  else
  {
    mLeftPanelHost->addPanel( mLayersWidget );
    mLayersWidget->setVisible( true );
    mLayerTreeViewButton->setIcon( QIcon( ":/kadas/icons/leftarrow" ) );
    mLayerTreeViewButton->setFixedSize( 20, 40 );
  }
}

void KadasMainWindow::toggleFullscreen()
{
  if ( !mFullscreen )
  {
    mRibbonWidget->setMaximumHeight( 45 );
    mFavoriteBackgroundWidget->hide();
    mFavoritesSearchBackgroundWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    mFullscreen = true;
    mRibbonbarButton->setIcon( QIcon( ":/kadas/icons/downarrow" ) );
  }
  else
  {
    mRibbonWidget->setMaximumHeight( QWIDGETSIZE_MAX );
    mFavoriteBackgroundWidget->show();
    mFavoritesSearchBackgroundWidget->layout()->setContentsMargins( 0, 10, 0, 10 );
    mFullscreen = false;
    mRibbonbarButton->setIcon( QIcon( ":/kadas/icons/uparrow" ) );
  }
}

void KadasMainWindow::endFullscreen()
{
  if ( mFullscreen )
  {
    toggleFullscreen();
  }
}

void KadasMainWindow::checkOnTheFlyProjection()
{
  mInfoBar->popWidget( mReprojMsgItem.data() );
  QString destAuthId = mMapCanvas->mapSettings().destinationCrs().authid();
  QStringList reprojLayers;
  // Look at legend interface instead of maplayerregistry, to only check layers
  // the user can actually see
  for ( QgsMapLayer *layer : mMapCanvas->layers() )
  {
    if ( layer->type() != Qgis::LayerType::Plugin && !layer->crs().authid().startsWith( "USER:" ) && layer->crs().authid() != destAuthId )
    {
      reprojLayers.append( layer->name() );
    }
  }
  if ( !reprojLayers.isEmpty() )
  {
    //    mReprojMsgItem = new QgsMessageBarItem( tr( "On the fly projection enabled" ), tr( "The following layers are being reprojected to the selected CRS: %1. Performance may suffer." ).arg( reprojLayers.join( ", " ) ), Qgis::Info, 10, this );
    //    mInfoBar->pushItem( mReprojMsgItem.data() );
  }
}

void KadasMainWindow::zoomFull()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mStatusBar->scaleCombo()->blockSignals( true );
  mMapCanvas->zoomToFullExtent();
  mStatusBar->scaleCombo()->blockSignals( false );
}

void KadasMainWindow::zoomIn()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mStatusBar->scaleCombo()->blockSignals( true );
  mMapCanvas->zoomIn();
  mStatusBar->scaleCombo()->blockSignals( false );
}

void KadasMainWindow::zoomNext()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mStatusBar->scaleCombo()->blockSignals( true );
  mMapCanvas->zoomToNextExtent();
  mStatusBar->scaleCombo()->blockSignals( false );
}

void KadasMainWindow::zoomOut()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mStatusBar->scaleCombo()->blockSignals( true );
  mMapCanvas->zoomOut();
  mStatusBar->scaleCombo()->blockSignals( false );
}

void KadasMainWindow::zoomPrev()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mStatusBar->scaleCombo()->blockSignals( true );
  mMapCanvas->zoomToPreviousExtent();
  mStatusBar->scaleCombo()->blockSignals( false );
}

void KadasMainWindow::zoomToLayerExtent()
{
  mLayerTreeView->defaultActions()->zoomToLayers( mMapCanvas );
}

void KadasMainWindow::showSourceSelectDialog( const QString &providerName )
{
  QgsSourceSelectProvider *provider = QgsGui::instance()->sourceSelectProviderRegistry()->providerByName( providerName );
  if ( !provider )
  {
    return;
  }
  QgsAbstractDataSourceWidget *dialog = provider->createDataSourceWidget( this, QgsGuiUtils::ModalDialogFlags, QgsProviderRegistry::WidgetMode::Standalone );
  dialog->setMapCanvas( mMapCanvas );
  dialog->setAttribute( Qt::WA_DeleteOnClose );
  QString sourceProvider = provider->providerKey();
  // TODO
  //  connect(dialog, &QgsAbstractDataSourceWidget::addDatabaseLayers, kApp, &KadasApplication::addDatabaseLayers);
  //  connect(dialog, &QgsAbstractDataSourceWidget::addMeshLayer, kApp, &KadasApplication::addMeshLayer);
  connect( dialog, &QgsAbstractDataSourceWidget::addLayer, kApp, [sourceProvider]( Qgis::LayerType type, const QString &uri, const QString &baseName, const QString &providerKey ) {
    switch ( type )
    {
      case Qgis::LayerType::Raster:
        kApp->addRasterLayer( uri, baseName, providerKey );
        break;
      case Qgis::LayerType::Vector:
        kApp->addVectorLayer( uri, baseName, !providerKey.isEmpty() ? providerKey : sourceProvider );
        break;
      case Qgis::LayerType::VectorTile:
        kApp->addVectorTileLayer( uri, baseName );
        break;
      default:
        break;
    }
  } );
  connect( dialog, &QgsAbstractDataSourceWidget::addLayer, dialog, &QDialog::accept );
  connect( dialog, &QgsAbstractDataSourceWidget::addVectorLayers, kApp, []( const QStringList &layerUris, const QString &enc, const QString &dataSourceType ) {
    kApp->addVectorLayers( layerUris, enc, dataSourceType );
  } );
  connect( dialog, &QgsAbstractDataSourceWidget::addVectorLayers, dialog, &QDialog::accept );
  connect( dialog, &QgsAbstractDataSourceWidget::addRasterLayers, []( const QStringList &layerUris ) { kApp->addRasterLayers( layerUris ); } );
  connect( dialog, &QgsAbstractDataSourceWidget::addRasterLayers, dialog, &QDialog::accept );

  dialog->exec();
}

void KadasMainWindow::setMapScale()
{
  mMapCanvas->zoomScale( mStatusBar->scaleCombo()->scale() );
}

void KadasMainWindow::toggleScaleLock( bool active )
{
  mStatusBar->scaleLockButton()->setIcon( active ? QgsApplication::getThemeIcon( "/locked.svg" ) : QgsApplication::getThemeIcon( "/unlocked.svg" ) );
  mMapCanvas->setScaleLocked( mStatusBar->scaleLockButton()->isChecked() );
  mStatusBar->scaleCombo()->setEnabled( !mStatusBar->scaleLockButton()->isChecked() );
}

void KadasMainWindow::setMapMagnifier( double value )
{
  mMapCanvas->blockSignals( true );
  mMapCanvas->setMagnificationFactor( value / 100. );
  mMapCanvas->blockSignals( false );
}

void KadasMainWindow::resetMagnification()
{
  mStatusBar->magnifierSpinBox()->clear();
}

void KadasMainWindow::showScale( double scale )
{
  mStatusBar->scaleCombo()->setScale( scale );
}

void KadasMainWindow::switchToTabForTool( QgsMapTool *tool )
{
  if ( tool && tool->action() )
  {
    for ( QObject *obj : tool->action()->associatedObjects() )
    {
      QWidget *widget = qobject_cast<QWidget *>( obj );
      if ( dynamic_cast<KadasRibbonButton *>( widget ) )
      {
        for ( int i = 0, n = mRibbonWidget->count(); i < n; ++i )
        {
          if ( mRibbonWidget->widget( i )->findChild<QWidget *>( widget->objectName() ) )
          {
            mRibbonWidget->blockSignals( true );
            mRibbonWidget->setCurrentIndex( i );
            mRibbonWidget->blockSignals( false );
            break;
          }
        }
      }
    }
    // If action is not associated to a kadas button, try with redlining and gpx route editor
    if ( tool->action()->parent() == mRedliningIntegration )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mDrawTab );
      mRibbonWidget->blockSignals( false );
    }
    else if ( tool->action()->parent() == mGpxIntegration )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mGpsTab );
      mRibbonWidget->blockSignals( false );
    }
  }
  // Nothing found, do nothing
}

void KadasMainWindow::showProjectSelectionWidget()
{
  mProjectTemplateDialog->exec();
}

void KadasMainWindow::onLanguageChanged( int idx )
{
  QString locale = mLanguageCombo->itemData( idx ).toString();
  if ( locale.isEmpty() )
  {
    QgsSettings().setValue( "/locale/overrideFlag", false );
    QgsSettings().setValue( "/locale/userLocale", QLocale::system().name() );
  }
  else
  {
    QgsSettings().setValue( "/locale/overrideFlag", true );
    QgsSettings().setValue( "/locale/userLocale", locale );
  }
  QMessageBox::information( this, tr( "Language Changed" ), tr( "The language will be changed at the next program launch." ) );
}

void KadasMainWindow::onDecimalPlacesChanged( int places )
{
  QgsSettings().setValue( "/kadas/measure_decimals", places );
}

void KadasMainWindow::onSnappingChanged( bool enabled )
{
  QgsSettings().setValue( "/kadas/snapping_enabled", enabled );
}

void KadasMainWindow::onNumericInputCheckboxToggled( bool checked )
{
  KadasSettingsTree::settingsShowNumericInput->setValue( checked );
}

void KadasMainWindow::showFavoriteContextMenu( const QPoint &pos )
{
  KadasRibbonButton *button = qobject_cast<KadasRibbonButton *>( QObject::sender() );
  QMenu menu;
  QAction *removeAction = menu.addAction( tr( "Remove" ) );
  if ( menu.exec( button->mapToGlobal( pos ) ) == removeAction )
  {
    QgsSettings().setValue( "/kadas/favoriteAction/" + button->objectName(), "" );
    button->setText( tr( "Favorite" ) );
    button->setIcon( QIcon( ":/kadas/icons/favorite" ) );
    button->setDefaultAction( 0 );
    button->setIconSize( QSize( 16, 16 ) );
    button->setEnabled( false );
  }
}

void KadasMainWindow::addCatalogLayer( const QgsMimeDataUtils::Uri &uri, const QString &metadataUrl, const QVariantList &sublayers, bool atInsertionPoint )
{
  // Track the layers added below, to select the last one afterwards
  QgsMapLayer *addedLayer = nullptr;
  const QMetaObject::Connection addedConnection
    = connect( QgsProject::instance()->layerTreeRegistryBridge(), &QgsLayerTreeRegistryBridge::addedLayersToLayerTree, this, [&addedLayer]( const QList<QgsMapLayer *> &layers ) {
        if ( !layers.isEmpty() )
        {
          addedLayer = layers.last();
        }
      } );

  QString adjustedUri = uri.uri;

  // Adjust layer CRS to project CRS
  QgsCoordinateReferenceSystem testCrs;
  for ( QString c : uri.supportedCrs )
  {
    testCrs.createFromOgcWmsCrs( c );
    if ( testCrs == mMapCanvas->mapSettings().destinationCrs() )
    {
      adjustedUri.replace( QRegularExpression( "crs=[^&]+" ), "crs=" + c );
      QgsDebugMsgLevel( QString( "Changing layer crs to %1, new uri: %2" ).arg( c, adjustedUri ), 2 );
      break;
    }
  }

  // Use the last used image format
  QString lastImageEncoding = QSettings().value( "/Qgis/lastWmsImageEncoding", "image/png" ).toString();
  for ( QString fmt : uri.supportedFormats )
  {
    if ( fmt == lastImageEncoding )
    {
      adjustedUri.replace( QRegularExpression( "format=[^&]+" ), "format=" + fmt );
      QgsDebugMsgLevel( QString( "Changing layer format to %1, new uri: %2" ).arg( fmt, adjustedUri ), 2 );
      break;
    }
  }

  if ( sublayers.size() == 1 )
  {
    // If there is exactly one sublayer, add it directly
    QVariantMap sublayer = sublayers[0].toMap();

    QgsMapLayer *layer = nullptr;
    if ( uri.providerKey == "arcgismapserver" )
    {
      QgsDataSourceUri dataSource( adjustedUri );
      dataSource.removeParam( "layer" );
      dataSource.setParam( "layer", QString::number( sublayer["id"].toInt() ) );
      layer = kApp->addRasterLayer( dataSource.uri( false ), uri.name, uri.providerKey, false, 0, !atInsertionPoint );
    }
    else if ( uri.providerKey == "arcgisfeatureserver" )
    {
      QgsDataSourceUri dataSource( adjustedUri );
      QString urlParameter = QString( "%1/%2" ).arg( dataSource.param( "url" ) ).arg( sublayer["id"].toInt() );
      dataSource.removeParam( "url" );
      dataSource.setParam( "url", urlParameter );
      layer = kApp->addVectorLayer( dataSource.uri( false ), uri.name, uri.providerKey, false, 0, !atInsertionPoint );
    }
    else if ( uri.providerKey == "arcgisvectortileservice" )
    {
      layer = kApp->addVectorTileLayer( adjustedUri, uri.name, false, true, 0, !atInsertionPoint );
    }
    else if ( uri.providerKey == "wms" )
    {
      adjustedUri.replace( QRegularExpression( "layers=[^&]*" ), "layers=" + sublayer["id"].toString() );
      layer = kApp->addRasterLayer( adjustedUri, uri.name, uri.providerKey, false, 0, !atInsertionPoint );
    }

    if ( layer )
    {
      layer->serverProperties()->setMetadataUrls( { QgsServerMetadataUrlProperties::MetadataUrl( metadataUrl ) } );
    }
  }
  else if ( !sublayers.isEmpty() )
  {
    struct Entry
    {
        int id;
        QString name;
        int parentId;
        bool leaf;
        QgsLayerTreeGroup *group;
        int order;
    };
    QMap<int, Entry *> entries;

    // First pass: build structure
    int nToplevel = 0;
    int order = 0;
    for ( int i = 0, n = sublayers.size(); i < n; ++i )
    {
      QVariantMap sublayer = sublayers[i].toMap();
      int id = sublayer["id"].toInt();
      int parentId = sublayer["parentLayerId"].toInt();
      if ( parentId == -1 )
      {
        nToplevel += 1;
      }
      QString name = sublayer["name"].toString();
      if ( entries.contains( parentId ) )
      {
        entries[parentId]->leaf = false;
      }
      entries[id] = new Entry { id, name, parentId, true, nullptr, order++ };
    }
    QList<Entry *> sortedEntries = entries.values();
    std::sort( sortedEntries.begin(), sortedEntries.end(), []( const Entry *a, const Entry *b ) { return a->order < b->order; } );

    QgsLayerTreeGroup *rootGroup = mLayerTreeView->layerTreeModel()->rootGroup();
    // If there are more than one toplevel items, add an extra enclosing group
    if ( nToplevel > 1 )
    {
      int offset = kApp->computeLayerGroupInsertionOffset( rootGroup );
      rootGroup = rootGroup->insertGroup( offset, uri.name );
    }
    int rootInsCount = 0;
    // Second pass: add groups/layers
    for ( Entry *entry : sortedEntries )
    {
      QgsLayerTreeGroup *parent = entries.contains( entry->parentId ) ? entries[entry->parentId]->group : rootGroup;
      if ( !parent )
      {
        parent = rootGroup;
      }
      if ( entry->leaf )
      {
        // This block builds the group hierarchy itself and sets an explicit
        // insertion point for every leaf, so the add*Layer() calls below pass
        // adjustInsertionPoint = false to honor it instead of recomputing the
        // default offset. The drop position (atInsertionPoint) is likewise not
        // threaded here: a multi-sublayer service is placed as its own group,
        // not at a single drop row.
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( parent, parent == rootGroup ? rootInsCount++ : parent->children().count() ) );

        QgsMapLayer *layer = nullptr;
        if ( uri.providerKey == "arcgismapserver" )
        {
          QgsDataSourceUri dataSource( adjustedUri );
          dataSource.removeParam( "layer" );
          dataSource.setParam( "layer", QString::number( entry->id ) );
          layer = kApp->addRasterLayer( dataSource.uri( false ), entry->name, uri.providerKey, false, 0, false );
        }
        else if ( uri.providerKey == "arcgisfeatureserver" )
        {
          QgsDataSourceUri dataSource( adjustedUri );
          QString urlParameter = QString( "%1/%2" ).arg( dataSource.param( "url" ) ).arg( entry->id );
          dataSource.removeParam( "url" );
          dataSource.setParam( "url", urlParameter );
          layer = kApp->addVectorLayer( dataSource.uri( false ), entry->name, uri.providerKey, false, 0, false );
        }
        else if ( uri.providerKey == "arcgisvectortileservice" )
        {
          layer = kApp->addVectorTileLayer( adjustedUri, entry->name, false, true, 0, false );
        }
        else if ( uri.providerKey == "wms" )
        {
          adjustedUri.replace( QRegularExpression( "layers=[^&]*" ), "layers=" + QString::number( entry->id ) );
          layer = kApp->addRasterLayer( adjustedUri, entry->name, uri.providerKey, false, 0, false );
        }

        if ( layer )
        {
          layer->serverProperties()->setMetadataUrls( { QgsServerMetadataUrlProperties::MetadataUrl( metadataUrl ) } );
        }
      }
      else
      {
        entry->group = parent == rootGroup ? parent->addGroup( entry->name ) : parent->addGroup( entry->name );
      }
    }

    qDeleteAll( entries );
  }
  else if ( uri.providerKey == "arcgisvectortileservice" )
  {
    QgsVectorTileLayer *layer = kApp->addVectorTileLayer( adjustedUri, uri.name, false, true, 0, !atInsertionPoint );
    if ( layer )
    {
      layer->serverProperties()->setMetadataUrls( { QgsServerMetadataUrlProperties::MetadataUrl( metadataUrl ) } );
    }
  }
  else
  {
    QgsRasterLayer *layer = kApp->addRasterLayer( adjustedUri, uri.name, uri.providerKey, false, 0, !atInsertionPoint );
    if ( layer )
    {
      layer->serverProperties()->setMetadataUrls( { QgsServerMetadataUrlProperties::MetadataUrl( metadataUrl ) } );
    }
  }
  // Reset insertion point
  QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mLayerTreeView->layerTreeModel()->rootGroup(), 0 ) );

  disconnect( addedConnection );
  if ( addedLayer )
  {
    mLayerTreeView->setCurrentLayer( addedLayer );
  }
}

void KadasMainWindow::checkLayerProjection( QgsMapLayer *layer )
{
  if ( layer->crs().authid().startsWith( "USER:" ) )
  {
    QPushButton *btn = new QPushButton( tr( "Manually set projection" ) );
    btn->setFlat( true );
    QgsMessageBarItem *item
      = new QgsMessageBarItem( tr( "Unknown layer projection" ), tr( "The projection of the layer %1 could not be recognized, its features might be misplaced." ).arg( layer->name() ), btn, Qgis::Warning, messageTimeout() );
    connect( btn, &QPushButton::clicked, [=, this] {
      mInfoBar->popWidget( item );
      kApp->showLayerProperties( layer );
    } );
    mInfoBar->pushItem( item );
  }
}

void KadasMainWindow::checkLayerTemporalCapabilities( QgsMapLayer *layer )
{
  if ( !layer->dataProvider() )
    return;

  QgsDataProviderTemporalCapabilities *temporalCapabilities = layer->dataProvider()->temporalCapabilities();

  if ( !temporalCapabilities )
    return;

  if ( !temporalCapabilities->hasTemporalCapabilities() )
    return;

  if ( !layer->temporalProperties() )
    return;

  if ( layer->temporalProperties()->isActive() )
    return;

  layer->temporalProperties()->setIsActive( true );
}

// Needs to be done after adding a catalog layer to ensure that the
// parameter IgnoreReportedLayerExtents is activated
void KadasMainWindow::checkWMSLayerIgnoreReportedExtents( QgsMapLayer *layer )
{
  QgsDataProvider *provider = layer->dataProvider();

  if ( !provider )
    return;

  if ( layer->providerType().toLower() != "wms" )
    return;

  QgsDataSourceUri uri = provider->uri();
  uri.setParam( QStringLiteral( "IgnoreReportedLayerExtents" ), QStringLiteral( "1" ) );
  provider->setUri( uri );
}

void KadasMainWindow::layerTreeViewDoubleClicked( const QModelIndex & /*index*/ )
{
  QgsMapLayer *layer = mLayerTreeView->currentLayer();
  if ( layer )
  {
    kApp->showLayerProperties( layer );
  }
}

int KadasMainWindow::messageTimeout() const
{
  return QgsSettings().value( QStringLiteral( "qgis/messageTimeout" ), 5 ).toInt();
}

QgsMapTool *KadasMainWindow::addPinTool()
{
  KadasAnnotationItemController *controller = KadasAnnotationControllerRegistry::instance()->controllerFor( KadasPinAnnotationItem::itemTypeId() );
  QgsAnnotationLayer *layer = KadasAnnotationLayerRegistry::getOrCreateAnnotationLayer( KadasAnnotationLayerRegistry::StandardLayer::PinsLayer );
  if ( !controller || !layer )
    return nullptr;
  auto *tool = new KadasMapToolEditAnnotationItem( mapCanvas(), controller, layer );
  tool->setMultipart( false );
  return tool;
}

void KadasMainWindow::addLocalPicture()
{
  // Choosing Image switches intent away from the previous tool: deactivate it
  // now so that cancelling the dialog leaves no tool active (a map click then
  // does nothing instead of, e.g., dropping another pin).
  if ( QgsMapTool *tool = mMapCanvas->mapTool() )
    mMapCanvas->unsetMapTool( tool );

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  for ( const QByteArray &format : QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }
  formats.insert( "*.svg" ); // Ensure svg is present

  QString filter = QString( "Images (%1)" ).arg( QStringList( formats.values() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Image" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );

  QPair<QString, QgsAnnotationLayer *> pair = kApp->addImageItem( filename );
  mMapCanvas->setMapTool( new KadasMapToolEditAnnotationItem( mapCanvas(), pair.second, pair.first ) );
}

void KadasMainWindow::addRemotePicture()
{
  // See addLocalPicture(): deactivate the previous tool so cancelling leaves no
  // active tool.
  if ( QgsMapTool *tool = mMapCanvas->mapTool() )
    mMapCanvas->unsetMapTool( tool );

  QDialog dialog;
  QGridLayout *layout = new QGridLayout();
  dialog.setLayout( layout );

  QLineEdit *urlLineEdit = new QLineEdit();
  QLabel *statusLabel = new QLabel();
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  connect( bbox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );

  layout->addWidget( new QLabel( "Image URL:" ), 0, 0, 1, 1 );
  layout->addWidget( urlLineEdit, 0, 1, 1, 1 );
  layout->addWidget( statusLabel, 1, 0, 1, 2 );
  layout->addWidget( bbox, 2, 0, 1, 2 );

  while ( dialog.exec() == QDialog::Accepted )
  {
    QString url = urlLineEdit->text().simplified();
    if ( url.isEmpty() )
    {
      continue;
    }
    statusLabel->setText( tr( "Downloading..." ) );
    dialog.setEnabled( false );

    QNetworkRequest request = QNetworkRequest( QUrl( url ) );

    QgsBlockingNetworkRequest newReq;
    const QgsBlockingNetworkRequest::ErrorCode errorCode = newReq.get( request, false );

    if ( errorCode != QgsBlockingNetworkRequest::NoError )
    {
      statusLabel->setText( tr( "Unable to download image (%1)." ).arg( newReq.errorMessage() ) );
      dialog.setEnabled( true );
      continue;
    }

    QTemporaryFile tempfile;
    tempfile.setAutoRemove( true );
    if ( !tempfile.open() )
    {
      statusLabel->setText( tr( "Unable to save downloaded image" ) );
      dialog.setEnabled( true );
      continue;
    }

    tempfile.write( newReq.reply().content() );
    tempfile.flush();

    QPair<QString, QgsAnnotationLayer *> pair = kApp->addImageItem( tempfile.fileName() );
    mMapCanvas->setMapTool( new KadasMapToolEditAnnotationItem( mapCanvas(), pair.second, pair.first ) );
    break;
  }
}

void KadasMainWindow::setElevationControllerRangeFromHeightmap()
{
  for ( KadasMapWidget *mapWidget : mMapWidgetManager->mapWidgets() )
  {
    mapWidget->setElevationController();
  }

  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    delete mElevationController;
    mElevationController = nullptr;

    return;
  }


  if ( !mElevationController )
  {
    mElevationController = new QgsElevationControllerWidget( this );
    KadasMapWidget::moveElevationControllerLabelsToLeft( mElevationController );
    connect( mElevationController, &QgsElevationControllerWidget::rangeChanged, this, &KadasMainWindow::setCanvasZRange );
    mElevationControllerFrame->layout()->addWidget( mElevationController );
  }


  QgsRasterLayer *rl = qobject_cast< QgsRasterLayer * >( layer );
  QgsRasterBandStats stats = rl->dataProvider()->bandStatistics( 1, Qgis::RasterBandStatistic::Min | Qgis::RasterBandStatistic::Max );

  mElevationController->setRangeLimits( QgsDoubleRange( stats.minimumValue, stats.maximumValue ) );
  mElevationController->setRange( QgsDoubleRange( stats.minimumValue, stats.maximumValue ) );
}

void KadasMainWindow::removeElevationControllers()
{
  bool toRemove = false;
  QgsLayerTree *rootNode = QgsProject::instance()->layerTreeRoot();
  for ( QgsMapLayer *layer : rootNode->layerOrder() )
  {
    QgsRasterLayer *rasterLayer = qobject_cast<QgsRasterLayer *>( layer );
    if ( rasterLayer )
    {
      if ( rasterLayer->elevationProperties()->hasElevation() )
      {
        toRemove = true;
        break;
      }
    }
  }


  if ( mElevationController )
  {
    delete mElevationController;
    mElevationController = nullptr;
  }
  for ( KadasMapWidget *mapWidget : mMapWidgetManager->mapWidgets() )
  {
    mapWidget->removeElevationController();
  }
}

void KadasMainWindow::addCustomDropHandler( QgsCustomDropHandler *handler )
{
  mCustomDropHandlers.append( handler );
}

void KadasMainWindow::removeCustomDropHandler( QgsCustomDropHandler *handler )
{
  mCustomDropHandlers.removeAll( handler );
}

void KadasMainWindow::showPluginManager( bool show )
{
  if ( show )
    mPluginManager->adjustSize();

  mPluginManager->setVisible( show );
}

void KadasMainWindow::showAuthenticatedUser( const QString &user )
{
  mLabelUsername->setText( user.isEmpty() ? "" : QString( "<small>%1<br />%2</small>" ).arg( tr( "Authenticated as:" ), user ) );
}

void KadasMainWindow::updateBgLayerZoomResolutions() const
{
  QList<double> resolutions;
  const QList<QgsMapLayer *> layers = mMapCanvas->layers();
  for ( auto it = layers.rbegin(), itEnd = layers.rend(); it != itEnd; ++it )
  {
    QgsMapLayer *layer = *it;

    QgsRasterLayer *rasterLayer = dynamic_cast<QgsRasterLayer *>( layer );
    if ( !rasterLayer )
    {
      continue;
    }

    QgsRasterDataProvider *currentProvider = rasterLayer->dataProvider();
    if ( !currentProvider )
    {
      continue;
    }

    // layer must not be reprojected
    if ( currentProvider->crs() != mMapCanvas->mapSettings().destinationCrs() )
    {
      continue;
    }

    if ( currentProvider->name().compare( "wms", Qt::CaseInsensitive ) == 0 )
    {
      //property 'resolutions' for wmts layers
      resolutions = rasterLayer->dataProvider()->nativeResolutions();
    }
    else if ( currentProvider->name().compare( "gdal", Qt::CaseInsensitive ) == 0 )
    {
      QgsRectangle extent = currentProvider->extent();

      GDALDatasetH ds = Kadas::gdalOpenForLayer( rasterLayer );
      GDALRasterBandH band = GDALGetRasterBand( ds, 1 );
      int count = GDALGetOverviewCount( band );
      for ( int i = 0; i < count; ++i )
      {
        GDALRasterBandH overview = GDALGetOverview( band, i );
        int xDim = GDALGetRasterBandXSize( overview );
        resolutions.append( extent.width() / xDim );
      }
      GDALClose( ds );
    }
    if ( !resolutions.isEmpty() )
    {
      break;
    }
  }
  if ( !resolutions.isEmpty() )
  {
    mMapCanvas->setZoomResolutions( resolutions );

    QStringList scales;
    double refRes = 0.0254 / mMapCanvas->mapSettings().outputDpi();
    for ( double resolution : resolutions )
    {
      double scale = resolution / refRes;
      scales.append( QgsScaleComboBox::toString( scale ) );
    }
    mStatusBar->scaleCombo()->updateScales( scales );
  }
  else
  {
    mStatusBar->scaleCombo()->updateScales( QStringList() ); // default scales
  }
}

void KadasMainWindow::showHelp() const
{
  mHelpViewer->showHelp();
}

void KadasMainWindow::showNewsletter()
{
  KadasNewsPopup::showIfNewsAvailable( true );
}

void KadasMainWindow::showFeedback()
{
  KadasFeedback::show();
}

void KadasMainWindow::toggleIgnoreDpiScale()
{
  QMessageBox::information( this, tr( "Font scaling setting changed" ), tr( "The font scaling change will be applied at the next program launch." ) );
  QgsSettings().setValue( "/kadas/ignore_dpi_scale", mCheckboxIgnoreSystemScaling->isChecked() );
}

void KadasMainWindow::setCanvasZRange( const QgsDoubleRange &range )
{
  if ( mMapCanvas )
  {
    mMapCanvas->setZRange( range );
  }
}
