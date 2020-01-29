/***************************************************************************
    kadasapplication.h
    ------------------
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

#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QImageReader>
#include <QMessageBox>
#include <QSplashScreen>
#include <QStatusBar>

#include <qgis/qgsauthguiutils.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsdataitem.h>
#include <qgis/qgsguiutils.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemapcanvasbridge.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayoutapputils.h>
#include <qgis/qgslayoutundostack.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmessageoutput.h>
#include <qgis/qgsmessageviewer.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsprintlayout.h>
#include <qgis/qgslayoutmanager.h>
#include <qgis/qgspluginlayerregistry.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsproviderregistry.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasterlayerproperties.h>
#include <qgis/qgssublayersdialog.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectorlayerproperties.h>
#include <qgis/qgsziputils.h>

#include <kadas/core/kadas.h>
#include <kadas/gui/kadasattributetabledialog.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadaspointitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadaspictureitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptooledititemgroup.h>
#include <kadas/gui/maptools/kadasmaptoolpan.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadascanvascontextmenu.h>
#include <kadas/app/kadascrashrpt.h>
#include <kadas/app/kadashandlebadlayers.h>
#include <kadas/app/kadasitemlayerproperties.h>
#include <kadas/app/kadaslayoutdesignermanager.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmessagelogviewer.h>
#include <kadas/app/kadasplugininterfaceimpl.h>
#include <kadas/app/kadasprojectmigration.h>
#include <kadas/app/kadaspythonintegration.h>
#include <kadas/app/bullseye/kadasbullseyelayer.h>
#include <kadas/app/guidegrid/kadasguidegridlayer.h>
#include <kadas/app/mapgrid/kadasmapgridlayer.h>


static QStringList splitSubLayerDef( const QString &subLayerDef )
{
  return subLayerDef.split( QgsDataProvider::SUBLAYER_SEPARATOR );
}

static void setupVectorLayer( const QString &vectorLayerPath,
                              const QStringList &sublayers,
                              QgsVectorLayer *&layer,
                              const QString &providerKey,
                              QgsVectorLayer::LayerOptions options )
{
  //set friendly name for datasources with only one layer
  QgsSettings settings;
  QStringList elements = splitSubLayerDef( sublayers.at( 0 ) );
  QString rawLayerName = elements.size() >= 2 ? elements.at( 1 ) : QString();
  QString subLayerNameFormatted = rawLayerName;
  if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
  {
    subLayerNameFormatted =  QgsMapLayer::formatLayerName( subLayerNameFormatted );
  }

  if ( elements.size() >= 4 && layer->name().compare( rawLayerName, Qt::CaseInsensitive ) != 0
       && layer->name().compare( subLayerNameFormatted, Qt::CaseInsensitive ) != 0 )
  {
    layer->setName( QStringLiteral( "%1 %2" ).arg( layer->name(), rawLayerName ) );
  }

  // Systematically add a layername= option to OGR datasets in case
  // the current single layer dataset becomes layer a multi-layer one.
  // Except for a few select extensions, known to be always single layer dataset.
  QFileInfo fi( vectorLayerPath );
  QString ext = fi.suffix().toLower();
  if ( providerKey == QLatin1String( "ogr" ) &&
       ext != QLatin1String( "shp" ) &&
       ext != QLatin1String( "mif" ) &&
       ext != QLatin1String( "tab" ) &&
       ext != QLatin1String( "csv" ) &&
       ext != QLatin1String( "geojson" ) &&
       ! vectorLayerPath.contains( QStringLiteral( "layerid=" ) ) &&
       ! vectorLayerPath.contains( QStringLiteral( "layername=" ) ) )
  {
    auto uriParts = QgsProviderRegistry::instance()->decodeUri(
                      layer->providerType(), layer->dataProvider()->dataSourceUri() );
    QString composedURI( uriParts.value( QStringLiteral( "path" ) ).toString() );
    composedURI += "|layername=" + rawLayerName;

    auto newLayer = qgis::make_unique<QgsVectorLayer> ( composedURI, layer->name(), QStringLiteral( "ogr" ), options );
    if ( newLayer && newLayer->isValid() )
    {
      delete layer;
      layer = newLayer.release();
    }
  }
}


KadasApplication *KadasApplication::instance()
{
  return qobject_cast<KadasApplication *> ( QCoreApplication::instance() );
}

bool KadasApplication::isRunningFromBuildDir()
{
  return QFile::exists( QDir( applicationDirPath() ).absoluteFilePath( ".kadasbuilddir" ) );
}

KadasApplication::KadasApplication( int &argc, char **argv )
  : QgsApplication( argc, argv, true )
{}

KadasApplication::~KadasApplication()
{
  delete mMainWindow;
}

void KadasApplication::init()
{
  QgsApplication::init();

  // Translations
  QString translationsPath;
  if ( isRunningFromBuildDir() )
  {
    translationsPath = QDir( applicationDirPath() ).absoluteFilePath( "../locale" );
  }
  else
  {
    translationsPath = QDir( Kadas::pkgDataPath() ).absoluteFilePath( "locale" );
  }
  QTranslator *translator = new QTranslator( this );
  translator->load( QString( "%1_%2" ).arg( Kadas::KADAS_RELEASE_NAME, translation() ), translationsPath );
  QApplication::instance()->installTranslator( translator );

  // Install crash reporter
  KadasCrashRpt crashReporter;
  crashReporter.install();

  QgsApplication::initQgis();

  QgsCoordinateTransform::setCustomMissingRequiredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::GridDetails & grid )
  {
    mMainWindow->messageBar()->pushWarning( tr( "Transform unavailable" ), tr( "Transform between %1 and %2 requires missing grid %3." ).arg( sourceCrs.authid() ).arg( destinationCrs.authid() ).arg( grid.shortName ) );
  } );

  QgsCoordinateTransform::setCustomMissingPreferredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & preferredOperation,
      const QgsDatumTransform::TransformDetails & availableOperation )
  {
    mMainWindow->messageBar()->pushWarning( tr( "Preferred transform unavailable" ), tr( "Preferred transform between %1 and %2 unavailable." ).arg( sourceCrs.authid() ).arg( destinationCrs.authid() ) );
  } );

  QgsCoordinateTransform::setCustomCoordinateOperationCreationErrorHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QString & error )
  {
    mMainWindow->messageBar()->pushWarning( tr( "Transform unavailable" ), tr( "Transform between %1 and %2 unavailable: %3." ).arg( sourceCrs.authid() ).arg( destinationCrs.authid() ).arg( error ) );
  } );

  QgsCoordinateTransform::setCustomMissingGridUsedByContextHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & desired )
  {
    mMainWindow->messageBar()->pushWarning( tr( "Transform unavailable" ), tr( "Transform between %1 and %2 unavailable." ).arg( sourceCrs.authid() ).arg( destinationCrs.authid() ) );
  } );

  // Setup application style
  setWindowIcon( QIcon( ":/kadas/logo" ) );
  QFile styleSheet( ":/stylesheet" );
  if ( styleSheet.open( QIODevice::ReadOnly ) )
  {
    setStyleSheet( QString::fromLocal8Bit( styleSheet.readAll() ) );
  }

  // Create / migrate settings
  QgsSettings settings;
  QFile srcSettings;
  bool settingsEmpty = false;
  if ( settings.value( "timestamp", 0 ).toInt() > 0 )
  {
    QgsDebugMsg( "Patching settings" );
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_patch.ini" ) );
  }
  else
  {
    QgsDebugMsg( "Copying full settings" );
    settingsEmpty = true;
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_full.ini" ) );
  }
  if ( srcSettings.exists() )
  {
    QgsSettings newSettings( srcSettings.fileName(), QSettings::IniFormat );
    QString timestamp = settings.value( "timestamp", "0" ).toString();
    QString newtimestamp = newSettings.value( "timestamp" ).toString();
    if ( settingsEmpty || newtimestamp > timestamp )
    {
      settings.setValue( "timestamp", newtimestamp );
      // Merge new settings to old settings
      for ( const QString &group : newSettings.childGroups() )
      {
        newSettings.beginGroup( group );
        settings.beginGroup( group );
        for ( const QString &key : newSettings.childKeys() )
        {
          settings.setValue( key, newSettings.value( key ) );
        }
        newSettings.endGroup();
        settings.endGroup();
      }
    }
    settings.sync();
  }

  // Look for certificates in <appDataDir>/certificates to add to the SSL socket CA certificate database
  QDir certDir( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "certificates" ) );
  QgsDebugMsg( QString( "Looking for certificates in %1" ).arg( certDir.absolutePath() ) );
  for ( const QString &certFilename : certDir.entryList( QStringList() << "*.pem", QDir::Files ) )
  {
    QFile certFile( certDir.absoluteFilePath( certFilename ) );
    if ( certFile.open( QIODevice::ReadOnly ) )
    {
      QgsDebugMsg( QString( "Reading certificate file %1" ).arg( certFile.fileName() ) );
      QByteArray pem = certFile.readAll();
      QList<QSslCertificate> certs = QSslCertificate::fromData( pem, QSsl::Pem );
      QgsDebugMsg( QString( "Adding %1 certificates" ).arg( certs.size() ) );
      for ( const QSslCertificate &cert : certs )
      {
        QSslSocket::addDefaultCaCertificate( cert );
      }
    }
  }

  // Create main window
  QSplashScreen splash( QPixmap( ":/kadas/splash" ) );
  splash.show();
  mMainWindow = new KadasMainWindow( &splash );
  mMainWindow->init();

  connect( mMainWindow->layerTreeView(), &QgsLayerTreeView::currentLayerChanged, this, &KadasApplication::onActiveLayerChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &KadasApplication::onMapToolChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::layersChanged, this, &KadasApplication::updateWmtsZoomResolutions );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &KadasApplication::updateWmtsZoomResolutions );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &KadasApplication::unsetMapTool );
  connect( QgsProject::instance(), &QgsProject::dirtySet, this, &KadasApplication::projectDirtySet );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasApplication::updateWindowTitle );
  connect( QgsProject::instance(), &QgsProject::projectSaved, this, &KadasApplication::updateWindowTitle );
  // Unset any active tool before writing project to ensure that any pending edits are committed
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasApplication::unsetMapToolOnSave );
  connect( this, &KadasApplication::focusChanged, this, &KadasApplication::onFocusChanged );
  connect( this, &QApplication::aboutToQuit, this, &KadasApplication::cleanup );

  QgsLayerTreeModel *layerTreeModel = mMainWindow->layerTreeView()->layerTreeModel();
  connect( layerTreeModel->rootGroup(), &QgsLayerTreeNode::addedChildren, QgsProject::instance(), &QgsProject::setDirty );
  connect( layerTreeModel->rootGroup(), &QgsLayerTreeNode::removedChildren, QgsProject::instance(), &QgsProject::setDirty );
  connect( layerTreeModel->rootGroup(), &QgsLayerTreeNode::visibilityChanged, QgsProject::instance(), &QgsProject::setDirty );

  mMapToolPan = new KadasMapToolPan( mMainWindow->mapCanvas() );
  mMainWindow->mapCanvas()->setMapTool( mMapToolPan );

  QgsMessageOutput::setMessageOutputCreator( messageOutputViewer );
  mMessageLogViewer = new KadasMessageLogViewer( mMainWindow );

  QgsProject::instance()->setBadLayerHandler( new KadasHandleBadLayersHandler );
  QgsPathResolver::setPathPreprocessor( [this]( const QString & path ) { return migrateDatasource( path ); } );

  // Register plugin layers
  pluginLayerRegistry()->addPluginLayerType( new KadasItemLayerType() );
  pluginLayerRegistry()->addPluginLayerType( new KadasMilxLayerType() );
  pluginLayerRegistry()->addPluginLayerType( new KadasBullseyeLayerType( mMainWindow->actionBullseye() ) );
  pluginLayerRegistry()->addPluginLayerType( new KadasGuideGridLayerType( mMainWindow->actionGuideGrid() ) );
  pluginLayerRegistry()->addPluginLayerType( new KadasMapGridLayerType( mMainWindow->actionMapGrid() ) );

  // Load python support
  mPythonInterface = new KadasPluginInterfaceImpl( this );
  loadPythonSupport();

  mMainWindow->show();
  splash.finish( mMainWindow );
  processEvents();

  // Setup layout item widgets
  QgsLayoutAppUtils::registerGuiForKnownItemTypes( mMainWindow->mapCanvas() );

  // Open startup project
  QgsProject::instance()->setDirty( false );
  if ( arguments().size() >= 2 && QFile::exists( arguments()[1] ) )
  {
    projectOpen( arguments()[1] );
    QgsProject::instance()->setDirty( false );
  }
  else
  {
    // Perform online/offline check to select default template
    QString onlineTestUrl = settings.value( "/kadas/onlineTestUrl" ).toString();
    QString projectTemplate;
    if ( !onlineTestUrl.isEmpty() )
    {
      QEventLoop eventLoop;
      QTimer timeout;
      QNetworkReply *reply = QgsNetworkAccessManager::instance()->head( QNetworkRequest( onlineTestUrl ) );
      QObject::connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
      QObject::connect( &timeout, &QTimer::timeout, &eventLoop, &QEventLoop::quit );
      timeout.start( 10000 );
      eventLoop.exec();

      if ( reply->error() == QNetworkReply::NoError && timeout.isActive() )
      {
        projectTemplate = settings.value( "/kadas/onlineDefaultProject" ).toString();
        settings.setValue( "/kadas/isOffline", false );
      }
      else
      {
        projectTemplate = settings.value( "/kadas/offlineDefaultProject" ).toString();
        settings.setValue( "/kadas/isOffline", true );
      }
      delete reply;
    }

    if ( !projectTemplate.isEmpty() )
    {
      projectTemplate = QDir( Kadas::projectTemplatesPath() ).absoluteFilePath( projectTemplate );
      projectCreateFromTemplate( projectTemplate );
    }
  }

  // TODO: QgsApplication::setMaxThreads( QgsSettings().value( "/Qgis/max_threads", -1 ).toInt() );

  updateWindowTitle();

  QObject::connect( this, &QApplication::lastWindowClosed, this, &QApplication::quit );
  mAutosaveTimer.setSingleShot( true );
  connect( &mAutosaveTimer, &QTimer::timeout, this, &KadasApplication::autosave );
}

QgsRasterLayer *KadasApplication::addRasterLayer( const QString &uri, const QString &layerName, const QString &providerKey ) const
{
  QgsSettings settings;
  QString baseName = settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() ? QgsMapLayer::formatLayerName( layerName ) : layerName;
  QgsDebugMsg( "Creating new raster layer using " + uri + " with baseName " + baseName + " and providerKey " + providerKey );

  QgsRasterLayer *layer = nullptr;
  if ( !providerKey.isEmpty() && uri.endsWith( QLatin1String( ".adf" ), Qt::CaseInsensitive ) )
  {
    QString dirName = QFileInfo( uri ).path();
    layer = new QgsRasterLayer( dirName, QFileInfo( dirName ).completeBaseName(), QStringLiteral( "gdal" ) );
  }
  else if ( providerKey.isEmpty() )
  {
    layer = new QgsRasterLayer( uri, baseName );
  }
  else
  {
    layer = new QgsRasterLayer( uri, baseName, providerKey );
  }

  QgsDebugMsg( QStringLiteral( "Constructed new layer" ) );

  if ( layer->isValid() )
  {
    QgsProject::instance()->addMapLayer( layer );
  }
  else
  {
    if ( layer->providerType() == QLatin1String( "gdal" ) && !layer->subLayers().empty() )
    {
      QList<QgsMapLayer *> subLayers = showGDALSublayerSelectionDialog( layer );
      QgsProject::instance()->addMapLayers( subLayers );

      // The first layer loaded is not useful in that case. The user can select it in the list if he wants to load it.
      delete layer;
      layer = !subLayers.isEmpty() ? qobject_cast< QgsRasterLayer * > ( subLayers.at( 0 ) ) : nullptr;
    }
    else
    {
      QString title = tr( "Invalid Layer" );
      QgsError error = layer->error();
      mMainWindow->messageBar()->pushMessage( title, error.message( QgsErrorMessage::Text ),
                                              Qgis::Critical, mMainWindow->messageTimeout() );
      delete layer;
      layer = nullptr;
    }
  }

  return layer;
}

QgsVectorLayer *KadasApplication::addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey )  const
{
  QgsSettings settings;
  QString baseName = settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() ? QgsMapLayer::formatLayerName( layerName ) : layerName;
  QgsDebugMsg( "Creating new vector layer using " + uri + " with baseName " + baseName + " and providerKey " + providerKey );

  // if the layer needs authentication, ensure the master password is set
  bool authok = true;
  QRegExp rx( "authcfg=([a-z]|[A-Z]|[0-9]){7}" );
  if ( rx.indexIn( uri ) != -1 )
  {
    authok = false;
    if ( !QgsAuthGuiUtils::isDisabled( mMainWindow->messageBar(), mMainWindow->messageTimeout() ) )
    {
      authok = kApp->authManager()->setMasterPassword( true );
    }
  }

  // create the layer
  bool isVsiCurl = uri.startsWith( QLatin1String( "/vsicurl" ), Qt::CaseInsensitive );
  QString scheme = QUrl( uri ).scheme();
  bool isRemoteUrl = scheme.startsWith( QStringLiteral( "http" ) ) || scheme == QStringLiteral( "ftp" );

  QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
  // Default style is loaded later in this method
  options.loadDefaultStyle = false;
  if ( isVsiCurl || isRemoteUrl )
  {
    mMainWindow->messageBar()->pushInfo( tr( "Remote layer" ), tr( "Loading %1, please wait..." ).arg( uri ) );
    QApplication::setOverrideCursor( Qt::WaitCursor );
    processEvents();
  }
  QgsVectorLayer *layer = new QgsVectorLayer( uri, baseName, providerKey, options );
  if ( isVsiCurl || isRemoteUrl )
  {
    QApplication::restoreOverrideCursor( );
  }

  if ( authok && layer->isValid() )
  {
    QStringList sublayers = layer->dataProvider()->subLayers();
    QgsDebugMsg( QStringLiteral( "got valid layer with %1 sublayers" ).arg( sublayers.count() ) );

    // If the newly created layer has more than 1 layer of data available, we show the
    // sublayers selection dialog so the user can select the sublayers to actually load.
    if ( sublayers.count() > 1 && !uri.contains( QStringLiteral( "layerid=" ) ) && !uri.contains( QStringLiteral( "layername=" ) ) )
    {
      QList<QgsMapLayer *> subLayers = showOGRSublayerSelectionDialog( layer );
      QgsProject::instance()->addMapLayers( subLayers );

      // The first layer loaded is not useful in that case. The user can select it in the list if he wants to load it.
      delete layer;
      layer = !subLayers.isEmpty() ? qobject_cast< QgsVectorLayer * > ( subLayers.at( 0 ) ) : nullptr;
    }
    else
    {
      //set friendly name for datasources with only one layer
      if ( !sublayers.isEmpty() )
      {
        setupVectorLayer( uri, sublayers, layer, providerKey, options );
      }

      QgsProject::instance()->addMapLayer( layer );
    }
  }
  else
  {
    QString title = tr( "Invalid Layer" );
    QgsError error = layer->error();
    mMainWindow->messageBar()->pushMessage( title, error.message( QgsErrorMessage::Text ),
                                            Qgis::Critical, mMainWindow->messageTimeout() );
    delete layer;
    layer = nullptr;
  }

  return layer;
}

void KadasApplication::addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType ) const
{
  for ( QString uri : layerUris )
  {
    uri = uri.trimmed();
    QString baseName;
    if ( dataSourceType == QLatin1String( "file" ) )
    {
      QString srcWithoutLayername( uri );
      int posPipe = srcWithoutLayername.indexOf( '|' );
      if ( posPipe >= 0 )
      {
        srcWithoutLayername.resize( posPipe );
      }
      baseName = QFileInfo( srcWithoutLayername ).completeBaseName();

      // if needed prompt for zipitem layers
      QString vsiPrefix = QgsZipItem::vsiPrefix( uri );
      if ( ! uri.startsWith( QLatin1String( "/vsi" ), Qt::CaseInsensitive ) &&
           ( vsiPrefix == QLatin1String( "/vsizip/" ) || vsiPrefix == QLatin1String( "/vsitar/" ) ) )
      {
        if ( showZipSublayerSelectionDialog( uri ) )
        {
          continue;
        }
      }
    }
    else if ( dataSourceType == QLatin1String( "database" ) )
    {
      // Try to extract the database name and use it as base name
      // sublayers names (if any) will be appended to the layer name
      QVariantMap parts = QgsProviderRegistry::instance()->decodeUri( QStringLiteral( "ogr" ), uri );
      if ( parts.value( QStringLiteral( "layerName" ) ).isValid() )
      {
        baseName = parts.value( QStringLiteral( "layerName" ) ).toString();
      }
      else
      {
        baseName = uri;
      }
    }
    else     //directory //protocol
    {
      baseName = QFileInfo( uri ).completeBaseName();
    }
    addVectorLayer( uri, baseName, QStringLiteral( "ogr" ) );
  }
}

QPair<KadasMapItem *, KadasItemLayerRegistry::StandardLayer> KadasApplication::addImageItem( const QString &filename ) const
{
  QString attachedPath = QgsProject::instance()->createAttachedFile( QFileInfo( filename ).fileName() );
  QFile( attachedPath ).remove();
  QFile( filename ).copy( attachedPath );
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  QString errMsg;
  QgsCoordinateReferenceSystem crs( "EPSG:3857" );
  QgsCoordinateTransform crst( mMainWindow->mapCanvas()->mapSettings().destinationCrs(), crs, QgsProject::instance()->transformContext() );
  if ( filename.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    KadasSymbolItem *item = new KadasSymbolItem( crs );
    item->setFilePath( attachedPath );
    item->setPosition( KadasItemPos::fromPoint( crst.transform( mMainWindow->mapCanvas()->extent().center() ) ) );
    return qMakePair( item, KadasItemLayerRegistry::SymbolsLayer );
  }
  else
  {
    KadasPictureItem *item = new KadasPictureItem( crs );
    item->setup( attachedPath, KadasItemPos::fromPoint( crst.transform( mMainWindow->mapCanvas()->extent().center() ) ) );
    return qMakePair( item, KadasItemLayerRegistry::PicturesLayer );
  }
}

KadasItemLayer *KadasApplication::selectItemLayer()
{
  QDialog dialog;
  dialog.setWindowTitle( tr( "Select layer" ) );
  dialog.setLayout( new QVBoxLayout() );
  dialog.layout()->setMargin( 2 );
  dialog.layout()->addWidget( new QLabel( tr( "Select layer to paste items to:" ) ) );
  KadasLayerSelectionWidget *layerSelectionWidget = new KadasLayerSelectionWidget( mMainWindow->mapCanvas(), mMainWindow->layerTreeView(), []( QgsMapLayer * layer ) { return dynamic_cast<KadasItemLayer *>( layer ); } );
  dialog.layout()->addWidget( layerSelectionWidget );
  QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  connect( buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );
  connect( buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject );
  dialog.layout()->addWidget( buttonBox );
  if ( dialog.exec() == QDialog::Accepted )
  {
    return dynamic_cast<KadasItemLayer *>( layerSelectionWidget->getSelectedLayer() );
  }
  else
  {
    return nullptr;
  }
}

bool KadasApplication::projectNew( bool askToSave )
{
  if ( askToSave && !projectSaveDirty() )
  {
    return false;
  }

  projectClose();
  return true;
}

bool KadasApplication::projectCreateFromTemplate( const QString &templateFile )
{
  if ( projectOpen( templateFile ) )
  {
    QgsProject::instance()->setFileName( QString() );
    addDefaultPrintTemplates();
    return true;
  }
  return false;
}

bool KadasApplication::projectOpen( const QString &projectFile )
{
  if ( !projectSaveDirty() )
  {
    return false;
  }
  QString fileName = projectFile;
  if ( fileName.isEmpty() )
  {
    QgsSettings settings;
    QString lastUsedDir = settings.value( "UI/lastProjectDir", QDir::homePath() ).toString();
    fileName = QFileDialog::getOpenFileName(
                 mMainWindow, tr( "Choose a KADAS Project" ), lastUsedDir, tr( "KADAS project files" ) + " (*.qgs *.qgz)"
               );
    if ( fileName.isEmpty() )
    {
      return false;
    }
    settings.setValue( "UI/lastProjectDir", QFileInfo( fileName ).absolutePath() );
  }

  projectClose();

  QString openFileName = fileName;
  QFileInfo finfo( fileName );
  QString autosaveFile( finfo.dir().absoluteFilePath( QString( "~%1" ).arg( finfo.fileName() ) ) );
  if ( QFile::exists( autosaveFile ) && QFileInfo( autosaveFile ).lastModified() > finfo.lastModified() )
  {
    QMessageBox::StandardButton response = QMessageBox::question( mMainWindow, tr( "Project recovery" ), tr( "A more recent automatic backup of the project exists. Open the backup instead?" ) );
    if ( response == QMessageBox::Yes )
    {
      openFileName = autosaveFile;
    }
  }

  QApplication::setOverrideCursor( Qt::WaitCursor );
  mMainWindow->mapCanvas()->freeze( true );
  bool autoSetupOnFirstLayer = mMainWindow->layerTreeMapCanvasBridge()->autoSetupOnFirstLayer();
  mMainWindow->layerTreeMapCanvasBridge()->setAutoSetupOnFirstLayer( false );

  QgsProjectDirtyBlocker dirtyBlocker( QgsProject::instance() );

  QStringList filesToAttach;
  QString migratedFileName = KadasProjectMigration::migrateProject( openFileName, filesToAttach );

  QString attachResolverId = QgsPathResolver::setPathPreprocessor( [filesToAttach]( const QString & path )
  {
    if ( filesToAttach.contains( path ) )
    {
      QString attachedFile = QgsProject::instance()->createAttachedFile( QFileInfo( path ).fileName() );
      QFile( attachedFile ).remove();
      QFile( path ).copy( attachedFile );
      return attachedFile;
    }
    return path;
  } );
  bool success = QgsProject::instance()->read( migratedFileName );
  QgsPathResolver::removePathPreprocessor( attachResolverId );

  if ( success )
  {
    emit projectRead();

    if ( migratedFileName != openFileName )
    {
      mMainWindow->messageBar()->pushMessage( tr( "Project migrated" ), tr( "The project was created with an older version of KADAS and automatically migrated." ) );
      QFileInfo finfo( fileName );
      QgsProject::instance()->setFileName( QDir( finfo.path() ).absoluteFilePath( finfo.baseName() + ".qgz" ) );
      QgsProject::instance()->setDirty( true );
      updateWindowTitle();
      QFile( migratedFileName ).remove();
    }
    else if ( openFileName != fileName )
    {
      QgsProject::instance()->setFileName( fileName );
      QgsProject::instance()->setDirty( true );
      updateWindowTitle();
    }
  }

  mMainWindow->layerTreeMapCanvasBridge()->setCanvasLayers();
  mMainWindow->layerTreeMapCanvasBridge()->setAutoSetupOnFirstLayer( autoSetupOnFirstLayer );
  mMainWindow->mapCanvas()->freeze( false );
  mMainWindow->mapCanvas()->refresh();
  QApplication::restoreOverrideCursor();

  if ( !success )
  {
    QMessageBox::critical( mMainWindow, tr( "Unable to open project" ), QgsProject::instance()->error() );
  }

  return success;
}

void KadasApplication::projectClose()
{
  cleanupAutosave();

  emit projectWillBeClosed();

  mMainWindow->mapCanvas()->freeze( true );

  unsetMapTool();

  // Avoid unnecessary layer changed handling for each layer removed - instead,
  // defer the handling until we've removed all layers
  mBlockActiveLayerChanged = true;
  QgsProject::instance()->clear();
  mBlockActiveLayerChanged = false;

  KadasLayoutDesignerManager::instance()->closeAllDesigners();

  mMainWindow->mapCanvas()->clearExtentHistory();

  KadasMapCanvasItemManager::clear();

  // clear out any stuff from project
  mMainWindow->mapCanvas()->setLayers( QList<QgsMapLayer *>() );
  mMainWindow->mapCanvas()->clearCache();
  mMainWindow->mapCanvas()->freeze( false );

  onActiveLayerChanged( currentLayer() );
}

bool KadasApplication::projectSaveDirty()
{
  if ( QgsProject::instance()->isDirty() )
  {
    QMessageBox::StandardButton response = QMessageBox::question(
        mMainWindow, tr( "Save Project" ), tr( "Do you want to save the current project?" ),
        QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard );
    if ( response == QMessageBox::Save )
    {
      return projectSave();
    }
    else if ( response == QMessageBox::Discard )
    {
      cleanupAutosave();
    }
    else if ( response == QMessageBox::Cancel )
    {
      return false;
    }
  }
  return true;
}

bool KadasApplication::projectSave( const QString &fileName, bool promptFileName )
{
  if ( ( QgsProject::instance()->fileName().isNull() && fileName.isEmpty() ) || promptFileName )
  {
    QgsSettings settings;
    QString lastUsedDir = settings.value( QStringLiteral( "UI/lastProjectDir" ), QDir::homePath() ).toString();

    QString path = QFileDialog::getSaveFileName(
                     mMainWindow, tr( "Choose a KADAS Project" ), lastUsedDir, tr( "Kadas project files" ) + " (*.qgz)"
                   );
    if ( path.isEmpty() )
    {
      return false;
    }
    settings.setValue( QStringLiteral( "UI/lastProjectDir" ), QFileInfo( path ).absolutePath() ) ;
    if ( !path.endsWith( ".qgz", Qt::CaseInsensitive ) )
    {
      path += ".qgz";
    }

    QgsProject::instance()->setFileName( path );
  }
  else if ( !fileName.isEmpty() )
  {
    QgsProject::instance()->setFileName( fileName );
  }

  mMainWindow->mapCanvas()->freeze( true );

  // Extract print layouts and save then as attached files
  const QList<QgsPrintLayout *> layouts = QgsProject::instance()->layoutManager()->printLayouts();
  QgsReadWriteContext context;
  context.setPathResolver( QgsProject::instance()->pathResolver() );
  for ( QgsPrintLayout *layout : layouts )
  {
    QString templateName = QgsProject::instance()->createAttachedFile( layout->name().replace( " ", "_" ) + ".qpt" );
    if ( layout->saveAsTemplate( templateName, context ) )
    {
      QgsProject::instance()->layoutManager()->removeLayout( layout );
    }
  }
  bool success = QgsProject::instance()->write();

  mMainWindow->mapCanvas()->freeze( false );

  if ( success )
  {
    mMainWindow->messageBar()->pushMessage( tr( "Project saved" ), "", Qgis::Info, mMainWindow->messageTimeout() );
    cleanupAutosave();
  }
  else
  {
    QMessageBox::critical( mMainWindow,
                           tr( "Unable to save project %1" ).arg( QDir::toNativeSeparators( QgsProject::instance()->fileName() ) ),
                           QgsProject::instance()->error() );
  }
  return success;
}

void KadasApplication::addDefaultPrintTemplates()
{
  QDir printTemplatesDir( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "print_templates" ) );
  for ( const QString &entry : printTemplatesDir.entryList( QStringList( "*.qpt" ), QDir::Files | QDir::NoDotAndDotDot ) )
  {
    QString attachedFile = QgsProject::instance()->createAttachedFile( entry );
    QFile( attachedFile ).remove();
    QFile( printTemplatesDir.absoluteFilePath( entry ) ).copy( attachedFile );
  }
}

void KadasApplication::saveMapAsImage()
{
  QPair< QString, QString> fileAndFilter = QgsGuiUtils::getSaveAsImageName( mMainWindow, tr( "Choose an Image File" ) );
  if ( fileAndFilter.first.isEmpty() )
  {
    return;
  }
  mMainWindow->mapCanvas()->saveAsImage( fileAndFilter.first, nullptr, fileAndFilter.second );
  mMainWindow->messageBar()->pushMessage( tr( "Map image saved to %1" ).arg( QFileInfo( fileAndFilter.first ).fileName() ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
}

void KadasApplication::saveMapToClipboard()
{
  QImage image( mMainWindow->mapCanvas()->size(), QImage::Format_ARGB32 );
  image.fill( QColor( 255, 255, 255, 1 ) );
  QPainter painter( &image );
  mMainWindow->mapCanvas()->render( &painter );
  QApplication::clipboard()->setImage( image );
  mMainWindow->messageBar()->pushMessage( tr( "Map image saved to clipboard" ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
}

void KadasApplication::showLayerAttributeTable( QgsMapLayer *layer )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( vlayer )
  {
    ( new KadasAttributeTableDialog( vlayer, mMainWindow->mapCanvas(), mMainWindow ) )->show();
    // Deletes on close
  }
}

void KadasApplication::showLayerProperties( QgsMapLayer *layer )
{
  if ( !layer )
    return;

  if ( layer->type() == QgsMapLayerType::RasterLayer )
  {
    QgsRasterLayerProperties dialog( layer, mainWindow()->mapCanvas(), mMainWindow );
    // Omit some panels
    QStackedWidget *stackedWidget = dialog.findChild<QStackedWidget *>( "mOptionsStackedWidget" );
    QListWidget *optionsWidget = dialog.findChild<QListWidget *>( "mOptionsListWidget" );
    QList<int> panelIndices;
    panelIndices << hideDialogPanel( "mOptsPage_Rendering", stackedWidget );
    panelIndices << hideDialogPanel( "mOptsPage_Server", stackedWidget );
    std::sort( panelIndices.begin(), panelIndices.end() );
    for ( int i = panelIndices.length() - 1; i >= 0; --i )
    {
      delete optionsWidget->item( panelIndices[i] );
    }

    dialog.exec();
  }
  else if ( layer->type() == QgsMapLayerType::VectorLayer )
  {
    QgsVectorLayerProperties dialog( mainWindow()->mapCanvas(), mainWindow()->messageBar(), static_cast<QgsVectorLayer *>( layer ), mMainWindow );
    // Omit some panels
    QStackedWidget *stackedWidget = dialog.findChild<QStackedWidget *>( "mOptionsStackedWidget" );
    QListWidget *optionsWidget = dialog.findChild<QListWidget *>( "mOptionsListWidget" );
    QList<int> panelIndices;
    panelIndices << hideDialogPanel( "mOptsPage_AttributesForm", stackedWidget );
    panelIndices << hideDialogPanel( "mOptsPage_AuxiliaryStorage", stackedWidget );
    panelIndices << hideDialogPanel( "mOptsPage_Variables", stackedWidget );
    panelIndices << hideDialogPanel( "mOptsPage_DataDependencies", stackedWidget );
    panelIndices << hideDialogPanel( "mOptsPage_Server", stackedWidget );
    std::sort( panelIndices.begin(), panelIndices.end() );
    for ( int i = panelIndices.length() - 1; i >= 0; --i )
    {
      delete optionsWidget->item( panelIndices[i] );
    }

    for ( QgsMapLayerConfigWidgetFactory *factory : mMapLayerPanelFactories )
    {
      dialog.addPropertiesPageFactory( factory );
    }
    dialog.exec();
  }
  else if ( qobject_cast<KadasItemLayer *>( layer ) )
  {
    KadasItemLayerProperties dialog( static_cast<KadasItemLayer *>( layer ), mainWindow() );
    for ( QgsMapLayerConfigWidgetFactory *factory : mMapLayerPanelFactories )
    {
      dialog.addPropertiesPageFactory( factory );
    }
    dialog.exec();
  }
}

int KadasApplication::hideDialogPanel( const QString &name, QStackedWidget *stackedWidget )
{
  QWidget *widget = stackedWidget->findChild<QWidget *>( name );
  int index = stackedWidget->indexOf( widget );
  widget->hide();
  return index;
}

void KadasApplication::showLayerInfo( const QgsMapLayer *layer )
{
  if ( layer && !layer->metadataUrl().isEmpty() )
  {
    QDesktopServices::openUrl( layer->metadataUrl() );
  }
}

void KadasApplication::showMessageLog()
{
  mMessageLogViewer->exec();
  mMessageLogViewer->hide();
}

QgsMapLayer *KadasApplication::currentLayer() const
{
  return mMainWindow->layerTreeView()->currentLayer();
}

void KadasApplication::registerMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory )
{
  mMapLayerPanelFactories << factory;
}

void KadasApplication::unregisterMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory )
{
  mMapLayerPanelFactories.removeAll( factory );
}

QgsPrintLayout *KadasApplication::createNewPrintLayout( const QString &title )
{
  QString t = title;
  if ( t.isEmpty() )
  {
    t = QgsProject::instance()->layoutManager()->generateUniqueTitle( QgsMasterLayoutInterface::PrintLayout );
  }
  //create new layout object
  QgsPrintLayout *layout = new QgsPrintLayout( QgsProject::instance() );
  layout->setName( t );
  layout->initializeDefaults();
  if ( QgsProject::instance()->layoutManager()->addLayout( layout ) )
  {
    emit printLayoutAdded( layout );
    return layout;
  }
  else
  {
    return nullptr;
  }
}

bool KadasApplication::deletePrintLayout( QgsPrintLayout *layout )
{
  emit printLayoutWillBeRemoved( layout );
  return QgsProject::instance()->layoutManager()->removeLayout( layout );
}

QList<QgsPrintLayout *> KadasApplication::printLayouts() const
{
  return QgsProject::instance()->layoutManager()->printLayouts();
}

void KadasApplication::showLayoutDesigner( QgsPrintLayout *layout )
{
  KadasLayoutDesignerManager::instance()->openDesigner( layout );
}

void KadasApplication::displayMessage( const QString &message, Qgis::MessageLevel level )
{
  mMainWindow->messageBar()->pushMessage( message, level, mMainWindow->messageTimeout() );
}

void KadasApplication::projectDirtySet()
{
  updateWindowTitle();
  mAutosaveTimer.start( 5000 );
}

void KadasApplication::autosave()
{
  if ( !QgsProject::instance()->fileName().isEmpty() && QgsProject::instance()->isDirty() )
  {
    mAutosaving = true;
    mMainWindow->statusBar()->showMessage( tr( "Autosaving project..." ), 3000 );
    QString prevFilename = QgsProject::instance()->fileName();
    QFileInfo finfo( prevFilename );
    QgsProject::instance()->setFileName( finfo.dir().absoluteFilePath( QString( "~%1" ).arg( finfo.fileName() ) ) );
    QgsProject::instance()->write();
    // Immediately remove the backup created by QgsProject::write
    QFile( QgsProject::instance()->fileName() + "~" ).remove();
    QgsProject::instance()->setFileName( prevFilename );
    QgsProject::instance()->setDirty();
    mAutosaveTimer.stop(); // Stop timer triggered by projectDirtyChanged()
    mAutosaving = false;
  }
}

void KadasApplication::cleanupAutosave()
{
  mAutosaveTimer.stop();
  QFileInfo finfo( QgsProject::instance()->fileName() );
  if ( finfo.exists() )
  {
    QFile( finfo.dir().absoluteFilePath( QString( "~%1" ).arg( finfo.fileName() ) ) ).remove();
  }
}

void KadasApplication::onActiveLayerChanged( QgsMapLayer *layer )
{
  if ( mBlockActiveLayerChanged )
  {
    return;
  }
  mMainWindow->mapCanvas()->setCurrentLayer( layer );
  emit activeLayerChanged( layer );
}

void KadasApplication::onFocusChanged( QWidget * /*old*/, QWidget *now )
{
  // If nothing has focus, ensure map canvas receives it
  if ( !now )
  {
    mMainWindow->mapCanvas()->setFocus();
  }
}

void KadasApplication::onMapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool )
{
  if ( oldTool )
  {
    disconnect( oldTool, &QgsMapTool::messageEmitted, this, &KadasApplication::displayMessage );
//    disconnect( oldTool, SIGNAL( messageDiscarded() ), this, SLOT( removeMapToolMessage() ) );
    if ( dynamic_cast<KadasMapToolPan *>( oldTool ) )
    {
      disconnect( static_cast<KadasMapToolPan *>( oldTool ), &KadasMapToolPan::itemPicked, this, &KadasApplication::handleItemPicked );
      disconnect( static_cast<KadasMapToolPan *>( oldTool ), &KadasMapToolPan::contextMenuRequested, this, &KadasApplication::showCanvasContextMenu );
    }
    if ( oldTool != mMapToolPan )
    {
      // Always delete unset tool, except for pan tool
      oldTool->deleteLater();
    }
  }
  // Automatically return to pan tool if no tool is active
  if ( !newTool )
  {
    mMainWindow->mapCanvas()->setMapTool( mMapToolPan );
    return;
  }

  if ( newTool )
  {
    connect( newTool, &QgsMapTool::messageEmitted, this, &KadasApplication::displayMessage );
    if ( dynamic_cast<KadasMapToolPan *>( newTool ) )
    {
      connect( static_cast<KadasMapToolPan *>( newTool ), &KadasMapToolPan::itemPicked, this, &KadasApplication::handleItemPicked );
      connect( static_cast<KadasMapToolPan *>( newTool ), &KadasMapToolPan::contextMenuRequested, this, &KadasApplication::showCanvasContextMenu );
    }
  }
}

void KadasApplication::unsetMapToolOnSave()
{
  if ( !mAutosaving )
  {
    unsetMapTool();
  }
}

QgsMapTool *KadasApplication::paste( QgsPointXY *mapPos )
{
  QgsMapCanvas *canvas = mMainWindow->mapCanvas();
  QgsPointXY pastePos = mapPos ? *mapPos : mMainWindow->mapCanvas()->center();
  QgsCoordinateReferenceSystem mapCrs = canvas->mapSettings().destinationCrs();
  if ( KadasClipboard::instance()->hasFormat( KADASCLIPBOARD_ITEMSTORE_MIME ) )
  {
    KadasItemLayer *layer = kApp->selectItemLayer();
    QList<KadasMapItem *> items;
    QList<QgsPointXY> itemPos;
    QgsPointXY center;
    for ( const KadasMapItem *item : KadasClipboard::instance()->storedMapItems() )
    {
      QgsCoordinateTransform crst( item->crs(), mapCrs, QgsProject::instance() );
      QgsPointXY pos = crst.transform( item->position() );
      itemPos.append( pos );
      items.append( item->clone() );
      center += QgsVector( pos.x(), pos.y() );
    }
    center /= itemPos.size();
    for ( int i = 0, n = items.size(); i < n; ++i )
    {
      QgsCoordinateTransform crst( mapCrs, items[i]->crs(), QgsProject::instance() );
      QgsPointXY pos = pastePos + QgsVector( itemPos[i].x() - center.x(), itemPos[i].y() - center.y() );
      items[i]->setPosition( KadasItemPos::fromPoint( crst.transform( pos ) ) );
    }
    if ( items.size() == 1 )
    {
      return new KadasMapToolEditItem( canvas, items.front(), layer );
    }
    else
    {
      return new KadasMapToolEditItemGroup( canvas, items, layer );
    }
  }
  else if ( KadasClipboard::instance()->hasFormat( KADASCLIPBOARD_FEATURESTORE_MIME ) )
  {
    KadasItemLayer *layer = kApp->selectItemLayer();
    QList<KadasMapItem *> items;
    const QgsFeatureStore &featureStore = KadasClipboard::instance()->getStoredFeatures();
    for ( const QgsFeature &feature : featureStore.features() )
    {
      if ( feature.geometry().type() == QgsWkbTypes::PointGeometry )
      {
        KadasPointItem *item = new KadasPointItem( featureStore.crs() );
        item->addPartFromGeometry( *feature.geometry().constGet() );
        items.append( item );
      }
      else if ( feature.geometry().type() == QgsWkbTypes::LineGeometry )
      {
        KadasLineItem *item = new KadasLineItem( featureStore.crs() );
        item->addPartFromGeometry( *feature.geometry().constGet() );
        items.append( item );
      }
      else if ( feature.geometry().type() == QgsWkbTypes::PolygonGeometry )
      {
        KadasPolygonItem *item = new KadasPolygonItem( featureStore.crs() );
        item->addPartFromGeometry( *feature.geometry().constGet() );
        items.append( item );
      }
    }
    if ( items.size() == 1 )
    {
      return new KadasMapToolEditItem( canvas, items.front(), layer );
    }
    else
    {
      return new KadasMapToolEditItemGroup( canvas, items, layer );
    }
  }
  else if ( KadasClipboard::instance()->hasFormat( "image/svg+xml" ) )
  {
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    QString filename = QgsProject::instance()->createAttachedFile( "pasted_image.svg" );
    QFile file( filename );
    if ( file.open( QIODevice::WriteOnly ) )
    {
      file.write( mimeData->data( "image/svg+xml" ) );
      file.close();
      KadasSymbolItem *item = new KadasSymbolItem( QgsCoordinateReferenceSystem( "EPSG:3857" ) );
      QgsCoordinateTransform crst( mapCrs, item->crs(), QgsProject::instance() );
      item->setup( filename, 0.5, 0.5 );
      item->setPosition( KadasItemPos::fromPoint( crst.transform( pastePos ) ) );
      return new KadasMapToolEditItem( canvas, item, KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::SymbolsLayer ) );
    }
  }
  else
  {
    const QMimeData *mimeData = KadasClipboard::instance()->mimeData();
    if ( mimeData && mimeData->hasImage() )
    {
      QImage image = qvariant_cast<QImage>( mimeData->imageData() );
      QString filename = QgsProject::instance()->createAttachedFile( "pasted_image.png" );
      image.save( filename );
      KadasPictureItem *item = new KadasPictureItem( QgsCoordinateReferenceSystem( "EPSG:3857" ) );
      QgsCoordinateTransform crst( mapCrs, item->crs(), QgsProject::instance() );
      item->setup( filename, KadasItemPos::fromPoint( crst.transform( pastePos ) ) );
      return new KadasMapToolEditItem( canvas, item, KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::PicturesLayer ) );
    }
  }
  return nullptr;
}

void KadasApplication::unsetMapTool()
{
  if ( mMainWindow->mapCanvas()->mapTool() != mMapToolPan )
  {
    mMainWindow->mapCanvas()->unsetMapTool( mMainWindow->mapCanvas()->mapTool() );
  }
}

void KadasApplication::handleItemPicked( const KadasFeaturePicker::PickResult &result )
{
  if ( qobject_cast<KadasItemLayer *> ( result.layer ) )
  {
    KadasItemLayer *layer = static_cast<KadasItemLayer *>( result.layer );
    QgsMapTool *tool = new KadasMapToolEditItem( mMainWindow->mapCanvas(), result.itemId, layer );
    mMainWindow->mapCanvas()->setMapTool( tool );
  }
}

void KadasApplication::showCanvasContextMenu( const QPoint &screenPos, const QgsPointXY &mapPos )
{
  KadasCanvasContextMenu( mMainWindow->mapCanvas(), screenPos, mapPos ).exec( mMainWindow->mapCanvas()->mapToGlobal( screenPos ) );
}

void KadasApplication::updateWindowTitle()
{
  QString fileName = QFileInfo( QgsProject::instance()->fileName() ).baseName();
  if ( fileName.isEmpty() )
  {
    fileName = tr( "<New Project>" );
  }
  QString modified;
  if ( QgsProject::instance()->isDirty() )
  {
    modified = " *";
  }
  QString online = QgsSettings().value( "/kadas/isOffline" ).toBool() ? tr( "Offline" ) : tr( "Online" );
  QString title = QString( "%1%2 - %3 [%4]" ).arg( fileName, modified, Kadas::KADAS_FULL_RELEASE_NAME, online );
  mMainWindow->setWindowTitle( title );
}

void KadasApplication::cleanup()
{
  if ( mPythonIntegration )
  {
    mPythonIntegration->unloadAllPlugins();
  }
}

QList<QgsMapLayer *> KadasApplication::showGDALSublayerSelectionDialog( QgsRasterLayer *layer ) const
{
  QList< QgsMapLayer * > result;
  if ( !layer )
    return result;

  QStringList sublayers = layer->subLayers();
  QgsDebugMsg( QStringLiteral( "raster has %1 sublayers" ).arg( layer->subLayers().size() ) );

  if ( sublayers.empty() )
    return result;

  QgsSettings settings;

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Gdal, QStringLiteral( "gdal" ), mMainWindow );
  chooseSublayersDialog.setShowAddToGroupCheckbox( true );

  QgsSublayersDialog::LayerDefinitionList layers;
  QStringList names;
  names.reserve( sublayers.size() );
  layers.reserve( sublayers.size() );
  for ( int i = 0; i < sublayers.size(); i++ )
  {
    // simplify raster sublayer name - should add a function in gdal provider for this?
    // code is copied from QgsGdalLayerItem::createChildren
    QString name = sublayers[i];
    QString path = layer->source();
    // if netcdf/hdf use all text after filename
    // for hdf4 it would be best to get description, because the subdataset_index is not very practical
    if ( name.startsWith( QLatin1String( "netcdf" ), Qt::CaseInsensitive ) ||
         name.startsWith( QLatin1String( "hdf" ), Qt::CaseInsensitive ) )
    {
      name = name.mid( name.indexOf( path ) + path.length() + 1 );
    }
    else if ( name.startsWith( QLatin1String( "GPKG" ), Qt::CaseInsensitive ) )
    {
      const auto parts { name.split( ':' ) };
      if ( parts.count() >= 3 )
      {
        name = parts.at( parts.count( ) - 1 );
      }
    }
    else
    {
      // remove driver name and file name
      name.remove( name.split( QgsDataProvider::SUBLAYER_SEPARATOR )[0] );
      name.remove( path );
    }
    // remove any : or " left over
    if ( name.startsWith( ':' ) )
      name.remove( 0, 1 );

    if ( name.startsWith( '\"' ) )
      name.remove( 0, 1 );

    if ( name.endsWith( ':' ) )
      name.chop( 1 );

    if ( name.endsWith( '\"' ) )
      name.chop( 1 );

    names << name;

    QgsSublayersDialog::LayerDefinition def;
    def.layerId = i;
    def.layerName = name;
    layers << def;
  }

  chooseSublayersDialog.populateLayerTable( layers );

  if ( chooseSublayersDialog.exec() )
  {
    // create more informative layer names, containing filename as well as sublayer name
    QRegExp rx( "\"(.*)\"" );
    QString uri, name;

    QgsLayerTreeGroup *group = nullptr;
    bool addToGroup = settings.value( QStringLiteral( "/qgis/openSublayersInGroup" ), true ).toBool();

    const auto constSelection = chooseSublayersDialog.selection();
    if ( !constSelection.isEmpty() && addToGroup )
    {
      group = QgsProject::instance()->layerTreeRoot()->insertGroup( 0, layer->name() );
    }
    for ( const QgsSublayersDialog::LayerDefinition &def : constSelection )
    {
      int i = def.layerId;
      if ( rx.indexIn( sublayers[i] ) != -1 )
      {
        uri = rx.cap( 1 );
        name = sublayers[i];
        name.replace( uri, QFileInfo( uri ).completeBaseName() );
      }
      else
      {
        name = names[i];
      }

      QgsRasterLayer *rlayer = new QgsRasterLayer( sublayers[i], name );
      if ( rlayer && rlayer->isValid() )
      {
        QgsProject::instance()->addMapLayer( rlayer, !addToGroup );
        if ( addToGroup )
        {
          group->addLayer( rlayer );
        }
      }
    }
  }
  return result;
}

QList<QgsMapLayer *> KadasApplication::showOGRSublayerSelectionDialog( QgsVectorLayer *layer ) const
{
  QList<QgsMapLayer *> result;
  QStringList sublayers = layer->dataProvider()->subLayers();

  QgsSublayersDialog::LayerDefinitionList list;
  QMap< QString, int > mapLayerNameToCount;
  bool uniqueNames = true;
  int lastLayerId = -1;
  const auto constSublayers = sublayers;
  for ( const QString &sublayer : constSublayers )
  {
    // OGR provider returns items in this format:
    // <layer_index>:<name>:<feature_count>:<geom_type>

    QStringList elements = splitSubLayerDef( sublayer );
    if ( elements.count() >= 4 )
    {
      QgsSublayersDialog::LayerDefinition def;
      def.layerId = elements[0].toInt();
      def.layerName = elements[1];
      def.count = elements[2].toInt();
      def.type = elements[3];
      if ( lastLayerId != def.layerId )
      {
        int count = ++mapLayerNameToCount[def.layerName];
        if ( count > 1 || def.layerName.isEmpty() )
        {
          uniqueNames = false;
        }
        lastLayerId = def.layerId;
      }
      list << def;
    }
    else
    {
      QgsDebugMsg( "Unexpected output from OGR provider's subLayers()! " + sublayer );
    }
  }

  // Check if the current layer uri contains the

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Ogr, QStringLiteral( "ogr" ), mMainWindow );
  chooseSublayersDialog.setShowAddToGroupCheckbox( true );
  chooseSublayersDialog.populateLayerTable( list );

  if ( !chooseSublayersDialog.exec() )
  {
    return result;
  }

  QString name = layer->name();

  auto uriParts = QgsProviderRegistry::instance()->decodeUri(
                    layer->providerType(), layer->dataProvider()->dataSourceUri() );
  QString uri( uriParts.value( QStringLiteral( "path" ) ).toString() );

  // The uri must contain the actual uri of the vectorLayer from which we are
  // going to load the sublayers.
  QString fileName = QFileInfo( uri ).baseName();
  const auto constSelection = chooseSublayersDialog.selection();
  for ( const QgsSublayersDialog::LayerDefinition &def : constSelection )
  {
    QString layerGeometryType = def.type;
    QString composedURI = uri;
    if ( uniqueNames )
    {
      composedURI += "|layername=" + def.layerName;
    }
    else
    {
      // Only use layerId if there are ambiguities with names
      composedURI += "|layerid=" + QString::number( def.layerId );
    }

    if ( !layerGeometryType.isEmpty() )
    {
      composedURI += "|geometrytype=" + layerGeometryType;
    }

    QgsDebugMsg( "Creating new vector layer using " + composedURI );
    QString name = fileName + " " + def.layerName;
    QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
    options.loadDefaultStyle = false;
    QgsVectorLayer *layer = new QgsVectorLayer( composedURI, name, QStringLiteral( "ogr" ), options );
    if ( layer && layer->isValid() )
    {
      result << layer;
    }
    else
    {
      QString msg = tr( "%1 is not a valid or recognized data source" ).arg( composedURI );
      mMainWindow->messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, Qgis::Critical, mMainWindow->messageTimeout() );
      delete layer;
    }
  }
  return result;
}

bool KadasApplication::showZipSublayerSelectionDialog( const QString &path ) const
{
  QVector<QgsDataItem *> childItems;
  QgsSettings settings;

  QgsDebugMsg( "askUserForZipItemLayers( " + path + ')' );

  // if scanZipBrowser == no: skip to the next file
  if ( settings.value( QStringLiteral( "qgis/scanZipInBrowser2" ), "basic" ).toString() == QLatin1String( "no" ) )
  {
    return false;
  }

  QgsZipItem zipItem( nullptr, path, path );
  zipItem.populate( true );
  QgsDebugMsg( QStringLiteral( "Path= %1 got zipitem with %2 children" ).arg( path ).arg( zipItem.rowCount() ) );

  // if 1 or 0 child found, exit so a normal item is created by gdal or ogr provider
  if ( zipItem.rowCount() <= 1 )
  {
    return false;
  }

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog( QgsSublayersDialog::Vsifile, QStringLiteral( "vsi" ), mMainWindow );
  QgsSublayersDialog::LayerDefinitionList layers;

  for ( int i = 0; i < zipItem.children().size(); i++ )
  {
    QgsDataItem *item = zipItem.children().at( i );
    QgsLayerItem *layerItem = qobject_cast<QgsLayerItem *> ( item );
    if ( !layerItem )
    {
      continue;
    }

    QgsDebugMsgLevel( QStringLiteral( "item path=%1 provider=%2" ).arg( item->path(), layerItem->providerKey() ), 2 );

    QgsSublayersDialog::LayerDefinition def;
    def.layerId = i;
    def.layerName = item->name();
    if ( layerItem->providerKey() == QLatin1String( "gdal" ) )
    {
      def.type = tr( "Raster" );
    }
    else if ( layerItem->providerKey() == QLatin1String( "ogr" ) )
    {
      def.type = tr( "Vector" );
    }
    layers << def;
  }

  chooseSublayersDialog.populateLayerTable( layers );

  if ( chooseSublayersDialog.exec() )
  {
    const auto constSelection = chooseSublayersDialog.selection();
    for ( const QgsSublayersDialog::LayerDefinition &def : constSelection )
    {
      childItems << zipItem.children().at( def.layerId );
    }
  }

  // add childItems
  const auto constChildItems = childItems;
  for ( QgsDataItem *item : constChildItems )
  {
    QgsLayerItem *layerItem = qobject_cast<QgsLayerItem *> ( item );
    if ( !layerItem )
    {
      continue;
    }

    QgsDebugMsg( QStringLiteral( "item path=%1 provider=%2" ).arg( item->path(), layerItem->providerKey() ) );
    if ( layerItem->providerKey() == QLatin1String( "gdal" ) )
    {
      addRasterLayer( item->path(), QFileInfo( item->name() ).completeBaseName(), QString() );
    }
    else if ( layerItem->providerKey() == QLatin1String( "ogr" ) )
    {
      addVectorLayers( QStringList( item->path() ), QStringLiteral( "System" ), QStringLiteral( "file" ) );
    }
  }

  return true;
}

QString KadasApplication::migrateDatasource( const QString &path ) const
{
  static QMap<QString, QString> dataSourceMap = dataSourceMigrationMap();
  QString normPath = path.toLower().replace( "\\", "/" );
  return dataSourceMap.value( normPath, path );
}

QMap<QString, QString> KadasApplication::dataSourceMigrationMap() const
{
  QMap<QString, QString> dataSourceMap;
  QString migrationsFilename = qgetenv( "KADAS_DATASOURCE_MIGRATIONS" );
  if ( migrationsFilename.isEmpty() )
  {
    migrationsFilename = "datasource_migrations.json";
  }
  QFile migrationsFile( QDir( Kadas::pkgDataPath() ).absoluteFilePath( migrationsFilename ) );
  if ( migrationsFile.open( QIODevice::ReadOnly ) )
  {
    QJsonDocument doc = QJsonDocument::fromJson( migrationsFile.readAll() );
    QJsonArray entries = doc.array();
    for ( int i = 0, n = entries.size(); i < n; ++i )
    {
      QJsonObject entry = entries.at( i ).toObject();
      dataSourceMap.insert( entry.value( "old" ).toString(), entry.value( "new" ).toString() );
    }
  }
  return dataSourceMap;
}

void KadasApplication::loadPythonSupport()
{
  //QgsDebugMsg("Python support library's instance() symbol resolved.");
  mPythonIntegration = new KadasPythonIntegration( this );
  mPythonIntegration->initPython( mPythonInterface, true );
  if ( !mPythonIntegration->isEnabled() )
  {
    mMainWindow->messageBar()->pushCritical( tr( "Python unavailable" ), tr( "Failed to load python support" ) );
  }
  else
  {
    QgsPythonRunner::setInstance( new KadasPythonRunner( mPythonIntegration ) );
    mPythonIntegration->restorePlugins();
  }
}

void KadasApplication::updateWmtsZoomResolutions() const
{
  QList<double> resolutions;
  for ( QgsMapLayer *layer : mMainWindow->mapCanvas()->layers() )
  {
    QgsRasterLayer *rasterLayer = dynamic_cast<QgsRasterLayer *>( layer );
    if ( !rasterLayer )
    {
      continue;
    }

    QgsRasterDataProvider *currentProvider = rasterLayer->dataProvider();
    if ( !currentProvider || currentProvider->name().compare( "wms", Qt::CaseInsensitive ) != 0 )
    {
      continue;
    }

    //wmts must not be reprojected
    if ( currentProvider->crs() != mMainWindow->mapCanvas()->mapSettings().destinationCrs() )
    {
      continue;
    }

    //property 'resolutions' for wmts layers
    resolutions = rasterLayer->dataProvider()->nativeResolutions();
    if ( !resolutions.isEmpty() )
    {
      break;
    }
  }
  mMainWindow->mapCanvas()->setZoomResolutions( resolutions );
}

QgsMessageOutput *KadasApplication::messageOutputViewer()
{
  if ( QThread::currentThread() == kApp->thread() )
  {
    return new QgsMessageViewer( kApp->mainWindow() );
  }
  else
  {
    return new QgsMessageOutputConsole();
  }
}
