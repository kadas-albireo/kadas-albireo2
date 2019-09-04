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
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>

//#include <qgis/qgsdecorationgrid.h> // TODO
#include <qgis/qgsgui.h>
#include <qgis/qgslayertreemapcanvasbridge.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreeviewdefaultactions.h>
#include <qgis/qgsmaptool.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssnappingutils.h>
#include <qgis/qgssourceselectproviderregistry.h>
#include <qgis/qgssourceselectprovider.h>

#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadascoordinatedisplayer.h>
#include <kadas/gui/kadasmapcanvasitem.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/kadasprojecttemplateselectiondialog.h>

#include <kadas/gui/catalog/kadasarcgisrestcatalogprovider.h>
#include <kadas/gui/catalog/kadasgeoadminrestcatalogprovider.h>
#include <kadas/gui/catalog/kadasvbscatalogprovider.h>

#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/gui/mapitems/kadaspictureitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>

#include <kadas/gui/mapitemeditors/kadassymbolattributeseditor.h>

#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/maptools/kadasmaptooldeleteitems.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptoolheightprofile.h>
#include <kadas/gui/maptools/kadasmaptoolhillshade.h>
#include <kadas/gui/maptools/kadasmaptoolmeasure.h>
#include <kadas/gui/maptools/kadasmaptoolslope.h>
#include <kadas/gui/maptools/kadasmaptoolviewshed.h>

#include <kadas/gui/search/kadascoordinatesearchprovider.h>
#include <kadas/gui/search/kadaslocationsearchprovider.h>
#include <kadas/gui/search/kadaslocaldatasearchprovider.h>
#include <kadas/gui/search/kadaspinsearchprovider.h>
#include <kadas/gui/search/kadasremotedatasearchprovider.h>
#include <kadas/gui/search/kadasworldlocationsearchprovider.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasgpsintegration.h>
#include <kadas/app/kadaslayertreeviewmenuprovider.h>
#include <kadas/app/kadasredliningintergration.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmapwidgetmanager.h>
#include <kadas/app/milx/kadasmilxintegration.h>

KadasMainWindow::KadasMainWindow( QSplashScreen *splash )
{
  KadasWindowBase::setupUi( this );

  QWidget *topWidget = new QWidget();
  KadasTopWidget::setupUi( topWidget );
  setMenuWidget( topWidget );

  QWidget *statusWidget = new QWidget();
  KadasStatusWidget::setupUi( statusWidget );
  statusBar()->addPermanentWidget( statusWidget, 1 );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );

  mGpsIntegration = new KadasGpsIntegration( this );
  mMapWidgetManager = new KadasMapWidgetManager( mMapCanvas, this );
//  mDecorationGrid = new QgsDecorationGrid(); // TODO
  mSearchWidget->init( mMapCanvas );

  mLayersWidget->setVisible( false );
  mLayersWidget->resize( qMax( 10, qMin( 800, QSettings().value( "/kadas/layersWidgetWidth", 200 ).toInt() ) ), mLayersWidget->height() );
  mGeodataBox->setCollapsed( false );
  mLayersBox->setCollapsed( false );

  // The MilX plugin enables the tab, if the plugin is enabled
  mRibbonWidget->setTabEnabled( mRibbonWidget->indexOf( mMssTab ), false );
  // The Globe plugin enables the action, if the plugin is enabled
  mAction3D->setEnabled( false );

  mLanguageCombo->addItem( tr( "System language" ), "" );
  mLanguageCombo->addItem( "English", "en" );
  mLanguageCombo->addItem( "Deutsch", "de" );
  mLanguageCombo->addItem( QString( "Fran%1ais" ).arg( QChar( 0x00E7 ) ), "fr" );
  mLanguageCombo->addItem( "Italiano", "it" );
  QString userLocale = QSettings().value( "/locale/userLocale" ).toString();
  if ( userLocale.isEmpty() )
  {
    mLanguageCombo->setCurrentIndex( 0 );
  }
  else
  {
    int idx = mLanguageCombo->findData( userLocale.left( 2 ).toLower() );
    if ( idx >= 0 )
    {
      mLanguageCombo->setCurrentIndex( idx );
    }
  }
  connect( mLanguageCombo, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasMainWindow::onLanguageChanged );

  mSpinBoxDecimalPlaces->setValue( QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt() );
  connect( mSpinBoxDecimalPlaces, qOverload<int> ( &QSpinBox::valueChanged ), this, &KadasMainWindow::onDecimalPlacesChanged );

  mSnappingCheckbox->setChecked( QSettings().value( "/Qgis/snapping/enabled", false ).toBool() );
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
  mAddServiceButton->setMenu( addServiceMenu );

  mMapCanvas->installEventFilter( this );
  mLayersWidgetResizeHandle->installEventFilter( this );

  mCoordinateDisplayer = new KadasCoordinateDisplayer( mDisplayCRSButton, mCoordinateLineEdit, mHeightLineEdit, mHeightUnitCombo, mMapCanvas, this );
  mCRSSelectionButton->setMapCanvas( mMapCanvas );

  connect( mScaleComboBox, &QgsScaleComboBox::scaleChanged, this, &KadasMainWindow::setMapScale );

  mNumericInputCheckbox->setChecked( QSettings().value( "/kadas/showNumericInput", false ).toBool() );
  connect( mNumericInputCheckbox, &QCheckBox::toggled, this, &KadasMainWindow::onNumericInputCheckboxToggled );

  QgsLayerTreeModel *model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
  model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
  model->setFlag( QgsLayerTreeModel::AllowNodeRename );
  model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
  model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
  model->setAutoCollapseLegendNodes( 1 );

  mLayerTreeView->setModel( model );
  mLayerTreeView->setMenuProvider( new KadasLayerTreeViewMenuProvider( mLayerTreeView ) );

  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemAdded, this, &KadasMainWindow::addMapCanvasItem );
  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemWillBeRemoved, this, &KadasMainWindow::removeMapCanvasItem );

  QgsSnappingConfig snappingConfig;
  snappingConfig.setMode( QgsSnappingConfig::AllLayers );
  snappingConfig.setType( QgsSnappingConfig::Vertex );
  int snappingRadius = QSettings().value( "/Qgis/snapping/radius", 10 ).toInt();
  snappingConfig.setTolerance( snappingRadius );
  snappingConfig.setUnits( QgsTolerance::Pixels );
  mMapCanvas->snappingUtils()->setConfig( snappingConfig );

  mPluginsToolButton->setMenu( new QMenu() );
  mPluginsToolButton->setPopupMode( QToolButton::InstantPopup );
  mPluginsToolButton->setFixedHeight( 45 );
  mPluginsWidget->hide();

  // Redlining
  KadasRedliningIntegration *redlining = new KadasRedliningIntegration( mToolButtonRedliningNewObject, this );

  // Route editor
  // TODO
//  mGpsRouteEditor = new QgsGPSRouteEditor( this, mActionDrawWaypoint, mActionDrawRoute );

  // Milx
  KadasMilxIntegration::MilxUi milxUi;
  milxUi.mRibbonWidget = mRibbonWidget;
  milxUi.mMssTab = mMssTab;
  milxUi.mActionMilx = mActionMilx;
  milxUi.mActionSaveMilx = mActionSaveMilx;
  milxUi.mActionLoadMilx = mActionLoadMilx;
  milxUi.mSymbolSizeSlider = mSymbolSizeSlider;
  milxUi.mLineWidthSlider = mLineWidthSlider;
  milxUi.mWorkModeCombo = mWorkModeCombo;
  KadasMilxIntegration *milx = new KadasMilxIntegration( milxUi );

  configureButtons();


  mCatalogBrowser->reload();
  connect( mRefreshCatalogButton, &QToolButton::clicked, mCatalogBrowser, &KadasCatalogBrowser::reload );

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
  connect( mMapCanvas, &QgsMapCanvas::renderStarting, &mLoadingTimer, qOverload<> ( &QTimer::start ) );
  connect( mMapCanvas, &QgsMapCanvas::mapCanvasRefreshed, &mLoadingTimer, &QTimer::stop );
  connect( mMapCanvas, &QgsMapCanvas::mapCanvasRefreshed, mLoadingLabel, &QLabel::hide );
  connect( &mLoadingTimer, &QTimer::timeout, mLoadingLabel, &QLabel::show );
  connect( mRibbonWidget, &QTabWidget::currentChanged, [this] { mMapCanvas->unsetMapTool( mMapCanvas->mapTool() ); } );   // Clear tool when changing active kadas tab
  connect( mZoomInButton, &QPushButton::clicked, this, &KadasMainWindow::zoomIn );
  connect( mZoomOutButton, &QPushButton::clicked, this, &KadasMainWindow::zoomOut );
  connect( mHomeButton, &QPushButton::clicked, this, &KadasMainWindow::zoomFull );
  connect( kApp->clipboard(), &KadasClipboard::dataChanged, [this] { mActionPaste->setEnabled( !kApp->clipboard()->isEmpty() ); } );
  connect( QgsProject::instance(), &QgsProject::layerWasAdded, this, &KadasMainWindow::checkLayerProjection );
  connect( mLayerTreeViewButton, &QPushButton::clicked, this, &KadasMainWindow::toggleLayerTree );

  QStringList catalogUris = QSettings().value( "/kadas/geodatacatalogs" ).toString().split( ";;" );
  for ( const QString &catalogUri : catalogUris )
  {
    QUrlQuery query( QUrl::fromEncoded( "?" + catalogUri.toLocal8Bit() ) );
    QString type = query.queryItemValue( "type" );
    QString url = query.queryItemValue( "url" );
    if ( type == "geoadmin" )
    {
      mCatalogBrowser->addProvider( new KadasGeoAdminRestCatalogProvider( url, mCatalogBrowser ) );
    }
    else if ( type == "arcgisrest" )
    {
      mCatalogBrowser->addProvider( new KadasArcGisRestCatalogProvider( url, mCatalogBrowser ) );
    }
    else if ( type == "vbs" )
    {
      mCatalogBrowser->addProvider( new KadasVBSCatalogProvider( url, mCatalogBrowser ) );
    }
  }

  mSearchWidget->addSearchProvider( new KadasCoordinateSearchProvider( mMapCanvas ) );
  mSearchWidget->addSearchProvider( new KadasLocationSearchProvider( mMapCanvas ) );
  mSearchWidget->addSearchProvider( new KadasLocalDataSearchProvider( mMapCanvas ) );
  mSearchWidget->addSearchProvider( new KadasPinSearchProvider( mMapCanvas ) );
  mSearchWidget->addSearchProvider( new KadasRemoteDataSearchProvider( mMapCanvas ) );
  mSearchWidget->addSearchProvider( new KadasWorldLocationSearchProvider( mMapCanvas ) );
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
    }
  }
  else if ( obj == mLayersWidgetResizeHandle && ev->type() == QEvent::MouseMove )
  {
    QMouseEvent *e = static_cast<QMouseEvent *>( ev );
    if ( e->buttons() == Qt::LeftButton )
    {
      QPoint delta = e->pos() - mResizePressPos;
      mLayersWidget->resize( qMax( 10, qMin( 800, mLayersWidget->width() + delta.x() ) ), mLayersWidget->height() );
      QSettings().setValue( "/kadas/layersWidgetWidth", mLayersWidget->width() );
      mLayerTreeViewButton->move( mLayersWidget->width(), mLayerTreeViewButton->y() );
    }
  }
  return false;
}

void KadasMainWindow::updateWidgetPositions()
{
  // Make sure +/- buttons have constant distance to upper right corner of map canvas
  int distanceToRightBorder = 9;
  int distanceToTop = 20;
  mZoomInOutFrame->move( mMapCanvas->width() - distanceToRightBorder - mZoomInOutFrame->width(), distanceToTop );

  mHomeButton->move( mMapCanvas->width() - distanceToRightBorder - mHomeButton->height(), distanceToTop + 90 );

  // Resize mLayersWidget and mLayerTreeViewButton
  int distanceToTopBottom = 40;
  int layerTreeHeight = mMapCanvas->height() - 2 * distanceToTopBottom;
  mLayerTreeViewButton->setGeometry( mLayerTreeViewButton->pos().x(), distanceToTopBottom, mLayerTreeViewButton->width(), layerTreeHeight );
  mLayersWidget->setGeometry( mLayersWidget->pos().x(), distanceToTopBottom, mLayersWidget->width(), layerTreeHeight );

  // Resize info bar
  double barwidth = 0.5 * mMapCanvas->width();
  double x = 0.5 * mMapCanvas->width() - 0.5 * barwidth;
  double y = mMapCanvas->y();
  mInfoBar->move( x, y );
  mInfoBar->setFixedWidth( barwidth );

  // Move loading label
  mLoadingLabel->move( mMapCanvas->width() - 5 - mLoadingLabel->width(), mMapCanvas->height() - 5 - mLoadingLabel->height() );

  // Move plugins button
  mPluginsToolButton->move( this->width() - mPluginsToolButton->width(), 0 );
}

void KadasMainWindow::mousePressEvent( QMouseEvent *event )
{
  if ( event->buttons() == Qt::LeftButton )
  {
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->pos() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) )
    {
      mDragStartPos = event->pos();
    }
  }
  // QgisApp::mousePressEvent( event ); TODO
}

void KadasMainWindow::mouseMoveEvent( QMouseEvent *event )
{
  if ( event->buttons() == Qt::LeftButton && !mDragStartPos.isNull() && ( mDragStartPos - event->pos() ).manhattanLength() >= QApplication::startDragDistance() )
  {
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->pos() ) );
    if ( button && !button->objectName().startsWith( "mFavoriteButton" ) )
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
//  QgisApp::mouseMoveEvent( event ); TODO
}

void KadasMainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->pos() ) );
  if ( event->mimeData()->hasFormat( "application/qgis-kadas-button" ) && button && button->objectName().startsWith( "mFavoriteButton" ) )
  {
    event->acceptProposedAction();
  }
  else
  {
//    QgisApp::dragEnterEvent( event ); TODO
  }
}

void KadasMainWindow::dropEvent( QDropEvent *event )
{
  if ( event->mimeData()->hasFormat( "application/qgis-kadas-button" ) )
  {
    QString actionName = QString::fromLocal8Bit( event->mimeData()->data( "application/qgis-kadas-button" ).data() );
    QAction *action = findChild<QAction *> ( actionName );
    if ( !action )
    {
      action = mAddedActions.value( actionName, nullptr );
    }
    KadasRibbonButton *button = dynamic_cast<KadasRibbonButton *>( childAt( event->pos() ) );
    if ( action && button && button->objectName().startsWith( "mFavoriteButton" ) )
    {
      button->setEnabled( true );
      setActionToButton( action, button );
      QSettings().setValue( "/kadas/favoriteAction/" + button->objectName(), actionName );
    }
  }
  else
  {
//    QgisApp::dropEvent( event ); // TODO
  }
}

void KadasMainWindow::showEvent( QShowEvent * )
{
  mGpsIntegration->initGui();
}

void KadasMainWindow::restoreFavoriteButton( QToolButton *button )
{
  QString actionName = QSettings().value( "/kadas/favoriteAction/" + button->objectName() ).toString();
  if ( actionName.isEmpty() )
  {
    return;
  }

  QAction *action = findChild<QAction *> ( actionName );
  if ( action )
  {
    setActionToButton( action, button );
  }
}

void KadasMainWindow::configureButtons()
{
  // Map tab

  setActionToButton( mActionNew, mNewButton, QKeySequence( Qt::CTRL + Qt::Key_N ) );
  connect( mActionNew, &QAction::triggered, this, &KadasMainWindow::showProjectSelectionWidget );

  setActionToButton( mActionOpen, mOpenButton, QKeySequence( Qt::CTRL + Qt::Key_O ) );
  connect( mActionOpen, &QAction::triggered, kApp, [] { kApp->projectOpen(); } );

  setActionToButton( mActionSave, mSaveButton, QKeySequence( Qt::CTRL + Qt::Key_S ) );
  connect( mActionSave, &QAction::triggered, kApp, [] { kApp->projectSave(); } );

  setActionToButton( mActionSaveAs, mSaveAsButton, QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_S ) );
  connect( mActionSaveAs, &QAction::triggered, kApp, [] { kApp->projectSave( QString(), true ); } );

  setActionToButton( mActionPrint, mPrintButton, QKeySequence( Qt::CTRL + Qt::Key_P ) );
  // signal connected by plugin

  setActionToButton( mActionCopy, mCopyButton );
  connect( mActionCopy, &QAction::triggered, kApp, &KadasApplication::saveMapToClipboard );

  connect( mActionSaveMapExtent, &QAction::triggered, kApp, &KadasApplication::saveMapAsImage );
  setActionToButton( mActionSaveMapExtent, mSaveMapExtentButton );

  QMenu *kmlMenu = new QMenu();

  QAction *actionExportKml = new QAction( tr( "KML Export" ) );
  connect( actionExportKml, &QAction::triggered, kApp, &KadasApplication::exportToKml );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_K ), this ), &QShortcut::activated, actionExportKml, &QAction::trigger );
  kmlMenu->addAction( actionExportKml );

  QAction *actionImportKml = new QAction( tr( "KML Import" ) );
  connect( actionImportKml, &QAction::triggered, kApp, &KadasApplication::importFromKml );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_I, Qt::CTRL + Qt::Key_K ), this ), &QShortcut::activated, actionImportKml, &QAction::trigger );
  kmlMenu->addAction( actionImportKml );

  mKMLButton->setIcon( QIcon( ":/kadas/icons/kml" ) );
  mKMLButton->setMenu( kmlMenu );
  mKMLButton->setPopupMode( QToolButton::InstantPopup );

  // View tab
  setActionToButton( mActionZoomLast, mZoomLastButton, QKeySequence( Qt::CTRL + Qt::Key_PageUp ) );
  connect( mActionZoomLast, &QAction::triggered, this, &KadasMainWindow::zoomPrev );
  connect( mMapCanvas, &QgsMapCanvas::zoomLastStatusChanged, mActionZoomLast, &QAction::setEnabled );

  setActionToButton( mActionZoomNext, mZoomNextButton, QKeySequence( Qt::CTRL + Qt::Key_PageDown ) );
  connect( mActionZoomNext, &QAction::triggered, this, &KadasMainWindow::zoomNext );
  connect( mMapCanvas, &QgsMapCanvas::zoomNextStatusChanged, mActionZoomNext, &QAction::setEnabled );

  setActionToButton( mActionNewMapWindow, mNewMapWindowButton, QKeySequence( Qt::CTRL + Qt::Key_W, Qt::CTRL + Qt::Key_N ) );
  connect( mActionNewMapWindow, &QAction::triggered, mMapWidgetManager, qOverload<> ( &KadasMapWidgetManager::addMapWidget ) );

  setActionToButton( mAction3D, m3DButton, QKeySequence( Qt::CTRL + Qt::Key_W, Qt::CTRL + Qt::Key_3 ) );
  // signal connected by plugin

  setActionToButton( mActionGrid, mGridButton, QKeySequence( Qt::CTRL + Qt::Key_W, Qt::CTRL + Qt::Key_G ) );
//  connect( mActionGrid, &QAction::triggered, mDecorationGrid, &QgsDecorationGrid::run ); // TODO

  // Draw tab
  setActionToButton( mActionPin, mPinButton, QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_M ), [this] { return addPinTool(); } );

  setActionToButton( mActionAddImage, mAddImageButton, QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_I ), [this] { return addPictureTool(); } );

  setActionToButton( mActionGuideGrid, mGuideGridButton, QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_G ), nullptr );

  setActionToButton( mActionBullseye, mBullseyeButton, QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_B ), nullptr );

  setActionToButton( mActionPaste, mPasteButton, QKeySequence( Qt::CTRL + Qt::Key_V ) );
  connect( mActionPaste, &QAction::triggered, kApp, &KadasApplication::paste );
  mActionPaste->setEnabled( !kApp->clipboard()->isEmpty() );

  setActionToButton( mActionDeleteItems, mDeleteItemsButton, QKeySequence(), [this] { return new KadasMapToolDeleteItems( mMapCanvas ); } );

  // Analysis tab
  setActionToButton( mActionDistance, mDistanceButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_D ), [this] { return new KadasMapToolMeasure( mMapCanvas, KadasMapToolMeasure::MeasureLine ); } );

  setActionToButton( mActionArea, mAreaButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_A ), [this] { return new KadasMapToolMeasure( mMapCanvas, KadasMapToolMeasure::MeasurePolygon ); } );

  setActionToButton( mActionCircle, mMeasureCircleButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_C ), [this] { return new KadasMapToolMeasure( mMapCanvas, KadasMapToolMeasure::MeasureCircle ); } );

  setActionToButton( mActionAzimuth, mAzimuthButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_B ), [this] { return new KadasMapToolMeasure( mMapCanvas, KadasMapToolMeasure::MeasureAzimuth ); } );

  setActionToButton( mActionProfile, mProfileButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_P ), [this] { return new KadasMapToolHeightProfile( mMapCanvas ); } );

  setActionToButton( mActionSlope, mSlopeButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_S ), [this] { return new KadasMapToolSlope( mMapCanvas ); } );

  setActionToButton( mActionHillshade, mHillshadeButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_H ), [this] { return new KadasMapToolHillshade( mMapCanvas ); } );

  setActionToButton( mActionViewshed, mViewshedButton, QKeySequence( Qt::CTRL + Qt::Key_A, Qt::CTRL + Qt::Key_V ), [this] { return new KadasMapToolViewshed( mMapCanvas ); } );

  // GPS tab
  setActionToButton( mActionDrawWaypoint, mDrawWaypointButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_W ) );
  setActionToButton( mActionDrawRoute, mDrawRouteButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_R ) );

  setActionToButton( mActionEnableGPS, mEnableGPSButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_T ) );
  connect( mActionEnableGPS, &QAction::triggered, mGpsIntegration, &KadasGpsIntegration::enableGPS );

  setActionToButton( mActionMoveWithGPS, mMoveWithGPSButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_M ) );
  connect( mActionMoveWithGPS, &QAction::triggered, mGpsIntegration, &KadasGpsIntegration::moveWithGPS );

  setActionToButton( mActionImportGPX, mGpxImportButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_I ) );
  connect( mActionImportGPX, &QAction::triggered, kApp, &KadasApplication::importFromGpx );

  setActionToButton( mActionExportGPX, mGpxExportButton, QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_E ) );
  connect( mActionExportGPX, &QAction::triggered, kApp, &KadasApplication::exportToGpx );

  // MSS tab
  setActionToButton( mActionMilx, mMilxButton, QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_S ) );
  setActionToButton( mActionSaveMilx, mSaveMilxButton, QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_E ) );
  setActionToButton( mActionLoadMilx, mLoadMilxButton, QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_I ) );
  setActionToButton( mActionImportOVL, mOvlButton, QKeySequence( Qt::CTRL + Qt::Key_M, Qt::CTRL + Qt::Key_O ) );

  //help tab
  setActionToButton( mActionHelp, mHelpButton );
  setActionToButton( mActionAbout, mAboutButton );
}

void KadasMainWindow::setActionToButton( QAction *action, QToolButton *button, const QKeySequence &shortcut, const std::function<QgsMapTool*() > &toolFactory )
{
  button->setDefaultAction( action );
  button->setIconSize( QSize( 32, 32 ) );
  if ( toolFactory )
  {
    button->setCheckable( true );
    connect( action, &QAction::triggered, [this, toolFactory, action]
    {
      QgsMapTool *tool = toolFactory();
      if ( tool )
      {
        connect( tool, &QgsMapTool::deactivated, tool, &QObject::deleteLater );
        tool->setAction( action );
        mMapCanvas->setMapTool( tool );
      }
    } );
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

QMenu *KadasMainWindow::pluginsMenu()
{
  // Only show the button if it is actually needed
  mPluginsWidget->show();
  return mPluginsToolButton->menu();
}

void KadasMainWindow::toggleLayerTree()
{
  bool visible = mLayersWidget->isVisible();
  mLayersWidget->setVisible( !visible );

  if ( !visible )
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/kadas/icons/layertree_unfolded" ) );
    mLayerTreeViewButton->move( mLayersWidget->size().width(), mLayerTreeViewButton->y() );
  }
  else
  {
    mLayerTreeViewButton->setIcon( QIcon( ":/kadas/icons/layertree_folded" ) );
    mLayerTreeViewButton->move( 0, mLayerTreeViewButton->y() );
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
    if ( /*layer->type() != QgsMapLayer::RedliningLayer &&*/ layer->type() != QgsMapLayerType::PluginLayer && !layer->crs().authid().startsWith( "USER:" ) && layer->crs().authid() != destAuthId )    // TODO
    {
      reprojLayers.append( layer->name() );
    }
  }
  if ( !reprojLayers.isEmpty() )
  {
    mReprojMsgItem = new QgsMessageBarItem( tr( "On the fly projection enabled" ), tr( "The following layers are being reprojected to the selected CRS: %1. Performance may suffer." ).arg( reprojLayers.join( ", " ) ), Qgis::Info, 10, this );
    mInfoBar->pushItem( mReprojMsgItem.data() );
  }
}

void KadasMainWindow::zoomFull()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mScaleComboBox->blockSignals( true );
  mMapCanvas->zoomToFullExtent();
  mScaleComboBox->blockSignals( false );
}

void KadasMainWindow::zoomIn()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mScaleComboBox->blockSignals( true );
  mMapCanvas->zoomIn();
  mScaleComboBox->blockSignals( false );
}

void KadasMainWindow::zoomNext()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mScaleComboBox->blockSignals( true );
  mMapCanvas->zoomToNextExtent();
  mScaleComboBox->blockSignals( false );
}

void KadasMainWindow::zoomOut()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mScaleComboBox->blockSignals( true );
  mMapCanvas->zoomOut();
  mScaleComboBox->blockSignals( false );
}

void KadasMainWindow::zoomPrev()
{
  // Block scale combobox signals, as the scale changed signals redundantly changes the map extent
  mScaleComboBox->blockSignals( true );
  mMapCanvas->zoomToPreviousExtent();
  mScaleComboBox->blockSignals( false );
}

void KadasMainWindow::zoomToLayerExtent()
{
  mLayerTreeView->defaultActions()->zoomToLayer( mMapCanvas );
}

void KadasMainWindow::showSourceSelectDialog( const QString &providerName )
{
  QgsSourceSelectProvider *provider = QgsGui::instance()->sourceSelectProviderRegistry()->providerByName( providerName );
  if ( !provider )
  {
    return;
  }
  QgsAbstractDataSourceWidget *dialog = provider->createDataSourceWidget();
  dialog->setMapCanvas( mMapCanvas );
  dialog->setAttribute( Qt::WA_DeleteOnClose );
  // TODO
//  connect(dialog, &QgsAbstractDataSourceWidget::addDatabaseLayers, kApp, &KadasApplication::addDatabaseLayers);
//  connect(dialog, &QgsAbstractDataSourceWidget::addMeshLayer, kApp, &KadasApplication::addMeshLayer);
  connect( dialog, &QgsAbstractDataSourceWidget::addRasterLayer, kApp, &KadasApplication::addRasterLayer );
  connect( dialog, &QgsAbstractDataSourceWidget::addVectorLayer, kApp, &KadasApplication::addVectorLayer );
  connect( dialog, &QgsAbstractDataSourceWidget::addVectorLayers, kApp, &KadasApplication::addVectorLayers );

  dialog->exec();
}

void KadasMainWindow::setMapScale()
{
  mMapCanvas->zoomScale( 1.0 / mScaleComboBox->scale() );
}

void KadasMainWindow::showScale( double scale )
{
  mScaleComboBox->setScale( 1.0 / scale );
}

void KadasMainWindow::switchToTabForTool( QgsMapTool *tool )
{
  if ( tool && tool->action() )
  {
    for ( QWidget *widget : tool->action()->associatedWidgets() )
    {
      if ( dynamic_cast<KadasRibbonButton *>( widget ) )
      {
        for ( int i = 0, n = mRibbonWidget->count(); i < n; ++i )
        {
          if ( mRibbonWidget->widget( i )->findChild<QWidget *> ( widget->objectName() ) )
          {
            mRibbonWidget->blockSignals( true );
            mRibbonWidget->setCurrentIndex( i );
            mRibbonWidget->blockSignals( false );
            break;
          }
        }
      }
    }
    // TODO
#if 0
    // If action is not associated to a kadas button, try with redlining and gpx route editor
    if ( tool->action()->parent() == mRedlining )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mDrawTab );
      mRibbonWidget->blockSignals( false );
    }
    else if ( tool->action()->parent() == mGpsRouteEditor )
    {
      mRibbonWidget->blockSignals( true );
      mRibbonWidget->setCurrentWidget( mGpsTab );
      mRibbonWidget->blockSignals( false );
    }
#endif
  }
  // Nothing found, do nothing
}

void KadasMainWindow::showProjectSelectionWidget()
{
  KadasProjectTemplateSelectionDialog dialog( this );
  if ( dialog.exec() == QDialog::Accepted && !dialog.selectedTemplate().isEmpty() )
  {
    kApp->projectCreateFromTemplate( dialog.selectedTemplate() );
  }
}

void KadasMainWindow::onLanguageChanged( int idx )
{
  QString locale = mLanguageCombo->itemData( idx ).toString();
  if ( locale.isEmpty() )
  {
    QSettings().setValue( "/locale/overrideFlag", false );
  }
  else
  {
    QSettings().setValue( "/locale/overrideFlag", true );
    QSettings().setValue( "/locale/userLocale", locale );
  }
  QMessageBox::information( this, tr( "Language Changed" ), tr( "The language will be changed at the next program launch." ) );
}

void KadasMainWindow::onDecimalPlacesChanged( int places )
{
  QSettings().setValue( "/Qgis/measure/decimalplaces", places );
}

void KadasMainWindow::onSnappingChanged( bool enabled )
{
  QSettings().setValue( "/Qgis/snapping/enabled", enabled );
}

void KadasMainWindow::onNumericInputCheckboxToggled( bool checked )
{
  QSettings().setValue( "/kadas/showNumericInput", checked );
}

void KadasMainWindow::showFavoriteContextMenu( const QPoint &pos )
{
  KadasRibbonButton *button = qobject_cast<KadasRibbonButton *> ( QObject::sender() );
  QMenu menu;
  QAction *removeAction = menu.addAction( tr( "Remove" ) );
  if ( menu.exec( button->mapToGlobal( pos ) ) == removeAction )
  {
    QSettings().setValue( "/kadas/favoriteAction/" + button->objectName(), "" );
    button->setText( tr( "Favorite" ) );
    button->setIcon( QIcon( ":/kadas/kadas/favorit.png" ) );
    button->setDefaultAction( 0 );
    button->setIconSize( QSize( 16, 16 ) );
    button->setEnabled( false );
  }
}

void KadasMainWindow::addMapCanvasItem( const KadasMapItem *item )
{
  KadasMapCanvasItem *canvasItem = new KadasMapCanvasItem( item, mMapCanvas );
  Q_UNUSED( canvasItem );  //item is already added automatically to canvas scene
}

void KadasMainWindow::removeMapCanvasItem( const KadasMapItem *item )
{
  for ( QGraphicsItem *canvasItem : mMapCanvas->items() )
  {
    if ( dynamic_cast<KadasMapCanvasItem *>( canvasItem ) && static_cast<KadasMapCanvasItem *>( canvasItem )->mapItem() == item )
    {
      delete canvasItem;
    }
  }
}

void KadasMainWindow::checkLayerProjection( QgsMapLayer *layer )
{
  if ( layer->crs().authid().startsWith( "USER:" ) )
  {
    QPushButton *btn = new QPushButton( tr( "Manually set projection" ) );
    btn->setFlat( true );
    QgsMessageBarItem *item = new QgsMessageBarItem(
      tr( "Unknown layer projection" ),
      tr( "The projection of the layer %1 could not be recognized, its and features might be misplaced." ).arg( layer->name() ),
      btn, Qgis::Warning, messageTimeout() );
    connect( btn, &QPushButton::clicked, [ = ]
    {
      mInfoBar->popWidget( item );
      kApp->showLayerProperties( layer );
    } );
    mInfoBar->pushItem( item );
  }
}

int KadasMainWindow::messageTimeout() const
{
  return QSettings().value( QStringLiteral( "qgis/messageTimeout" ), 5 ).toInt();
}

QgsMapTool *KadasMainWindow::addPinTool()
{
  KadasMapToolCreateItem::ItemFactory factory = [this]
  {
    KadasSymbolItem *item = new KadasSymbolItem( mapCanvas()->mapSettings().destinationCrs() );
    item->setFilePath( ":/kadas/icons/pin_red", 0.5, 1.0 );
    item->setEditorFactory( KadasSymbolAttributesEditor::factory );
    return item;
  };
  return new KadasMapToolCreateItem( mapCanvas(), factory, kApp->getOrCreateItemLayer( tr( "Pins" ) ) );
}

QgsMapTool *KadasMainWindow::addPictureTool()
{
  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  for ( const QByteArray &format : QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }
  formats.insert( "*.svg" );  // Ensure svg is present

  QString filter = QString( "Images (%1)" ).arg( QStringList( formats.toList() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Image" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    return nullptr;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  QString errMsg;
  if ( filename.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    KadasSymbolItem *item = new KadasSymbolItem( mapCanvas()->mapSettings().destinationCrs() );
    item->setFilePath( filename );
    item->setPosition( mapCanvas()->extent().center() );
    return new KadasMapToolEditItem( mapCanvas(), item, kApp->getOrCreateItemLayer( tr( "SVG graphics" ) ) );
  }
  else
  {
    KadasPictureItem *item = new KadasPictureItem( mapCanvas()->mapSettings().destinationCrs() );
    item->setFilePath( filename, mapCanvas()->extent().center() );
    return new KadasMapToolEditItem( mapCanvas(), item, kApp->getOrCreateItemLayer( tr( "Pictures" ) ) );
  }
}
