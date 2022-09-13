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

#include <QBuffer>
#include <QClipboard>
#include <QDesktopServices>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QImageReader>
#include <QMessageBox>
#include <QSplashScreen>
#include <QStatusBar>
#include <QUrlQuery>
#include <quazip/quazipfile.h>

#include <qgis/qgsauthguiutils.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsdataitem.h>
#include <qgis/qgsgdalutils.h>
#include <qgis/qgsguiutils.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemapcanvasbridge.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayoutundostack.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmessageoutput.h>
#include <qgis/qgsmessageviewer.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgspointcloudlayer.h>
#include <qgis/qgsprintlayout.h>
#include <qgis/qgslayeritem.h>
#include <qgis/qgslayoutguiutils.h>
#include <qgis/qgslayoutmanager.h>
#include <qgis/qgspluginlayerregistry.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsproviderregistry.h>
#include <qgis/qgsproviderutils.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasterlayerproperties.h>
#include <qgis/qgssublayersdialog.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectortilelayer.h>
#include <qgis/qgsvectortilelayerproperties.h>
#include <qgis/qgsvectorlayerproperties.h>
#include <qgis/qgszipitem.h>
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
#include <kadas/app/kadaspluginlayerproperties.h>
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
  return subLayerDef.split( QgsDataProvider::sublayerSeparator() );
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

    auto newLayer = std::make_unique<QgsVectorLayer> ( composedURI, layer->name(), QStringLiteral( "ogr" ), options );
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
  delete mProjectTempDir;

  for ( QgsPluginLayerType *layerType : mKadasPluginLayerTypes )
  {
    pluginLayerRegistry()->removePluginLayerType( layerType->name() );
  }
}

void KadasApplication::init()
{
  QgsApplication::init();

  mProjectTempDir = new QTemporaryDir();
  mProjectTempDir->setAutoRemove( true );

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
  translator->load( QString( "Kadas_%1" ).arg( translation() ), translationsPath );
  QApplication::instance()->installTranslator( translator );

  // Install crash reporter
  KadasCrashRpt::install();

  QgsApplication::initQgis();

  QgsCoordinateTransform::setCustomMissingRequiredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::GridDetails & grid )
  {
    mMainWindow->messageBar()->pushWarning( tr( "Transform unavailable" ), tr( "Transform between %1 and %2 requires missing grid %3." ).arg( sourceCrs.authid() ).arg( destinationCrs.authid() ).arg( grid.shortName ) );
  } );

  QgsCoordinateTransform::setCustomMissingPreferredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & /*preferredOperation*/,
      const QgsDatumTransform::TransformDetails & /*availableOperation*/ )
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
      const QgsDatumTransform::TransformDetails & /*desired*/ )
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

  // Set help path
  // See qgis/CMakeLists.txt QGIS_VERSION_INT
  int QGIS_VERSION_MAJOR = Qgis::versionInt() / 10000;
  int QGIS_VERSION_MINOR = ( Qgis::versionInt() - QGIS_VERSION_MAJOR * 10000 ) / 100;
  settings.setValue( QStringLiteral( "help/helpSearchPath" ), QString( "https://docs.qgis.org/%1.%2/en/docs/user_manual" ).arg( QGIS_VERSION_MAJOR ).arg( QGIS_VERSION_MINOR ) );

  // Ensure network access manager uses the correct proxy settings
  QgsNetworkAccessManager::instance()->setupDefaultProxyAndCache();

  Kadas::importSslCertificates();


  // Add token injector
  QgsNetworkAccessManager::setRequestPreprocessor( injectAuthToken );

  // Add network request logger
  QgsNetworkAccessManager::instance()->setRequestPreprocessor( []( QNetworkRequest * req )
  {
    QgsDebugMsg( QString( "Network request: %1" ).arg( req->url().toString() ) );
  } );

  // Create main window
  QSplashScreen splash( QPixmap( ":/kadas/splash" ) );
  splash.show();
  mMainWindow = new KadasMainWindow();
  mMainWindow->init();

  connect( mMainWindow->layerTreeView(), &QgsLayerTreeView::currentLayerChanged, this, &KadasApplication::onActiveLayerChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &KadasApplication::onMapToolChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::layersChanged, this, &KadasApplication::updateWmtsZoomResolutions );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &KadasApplication::updateWmtsZoomResolutions );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &KadasApplication::unsetMapTool );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::extentsChanged, this, &KadasApplication::extentChanged );
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
  mKadasPluginLayerTypes.append( new KadasItemLayerType() );
  mKadasPluginLayerTypes.append( new KadasMilxLayerType() );
  mKadasPluginLayerTypes.append( new KadasBullseyeLayerType( mMainWindow->actionBullseye() ) );
  mKadasPluginLayerTypes.append( new KadasGuideGridLayerType( mMainWindow->actionGuideGrid() ) );
  mKadasPluginLayerTypes.append( new KadasMapGridLayerType( mMainWindow->actionMapGrid() ) );

  for ( QgsPluginLayerType *layerType : mKadasPluginLayerTypes )
  {
    pluginLayerRegistry()->addPluginLayerType( layerType );
  }

  // Load python support
  mPythonInterface = new KadasPluginInterfaceImpl( this );
  loadPythonSupport();

  mMainWindow->show();
  splash.finish( mMainWindow );
  processEvents();

  // Setup layout item widgets
  QgsLayoutGuiUtils::registerGuiForKnownItemTypes( mMainWindow->mapCanvas() );

  // Init KadasItemLayerRegistry
  KadasItemLayerRegistry::init();

  // Extract portal token if necessary before loading startup project
  QString tokenUrl = settings.value( "/kadas/portalTokenUrl" ).toString();
  if ( !tokenUrl.isEmpty() )
  {
    QNetworkRequest req = QNetworkRequest( QUrl( tokenUrl ) );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( reply, &QNetworkReply::finished, this, &KadasApplication::extractPortalToken );
  }
  else
  {
    loadStartupProject();
  }
}

void KadasApplication::extractPortalToken()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( reply->error() == QNetworkReply::NoError )
  {
    QList<QByteArray> setCookieFields = reply->rawHeader( "Set-Cookie" ).split( ';' );
    QgsDebugMsg( QString( "Set-Cookie header: %1" ).arg( QString::fromUtf8( reply->rawHeader( "Set-Cookie" ) ) ) );
    if ( setCookieFields.length() > 0 && setCookieFields[0].startsWith( "esri_auth=" ) )
    {
      QJsonDocument esriAuth = QJsonDocument::fromJson( QUrl::fromPercentEncoding( setCookieFields[0] ).toUtf8().mid( 10 ) );
      QString username = esriAuth.object()["email"].toString().replace( QRegExp( "@.*$" ), "" );
      QgsDebugMsg( QString( "Extracted username from Set-Cookie: %1" ).arg( username ) );
      mMainWindow->showAuthenticatedUser( username );

      QString token = esriAuth.object()["token"].toString();
      QgsDebugMsg( QString( "Extracted token from Set-Cookie: %1" ).arg( token ) );
      if ( !token.isEmpty() )
      {
        QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
        QString cookie = QString( "esri_auth=\"token\": \"%1\"" ).arg( token );
        QStringList cookieUrls = QgsSettings().value( "/iamauth/cookieurls", "" ).toString().split( ";" );
        for ( const QString &url : cookieUrls )
        {
          QgsDebugMsg( QString( "Setting cookie for url %1: %2" ).arg( url, cookie ) );
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url );
        }
      }
    }
    else
    {
      QVariantMap listData = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
      mMainWindow->showAuthenticatedUser( listData["user"].toString() );
      QgsDebugMsg( QString( "Extracted username: %1" ).arg( listData["user"].toString() ) );
      QString cookie = listData["esri_auth"].toString();
      if ( !cookie.isEmpty() )
      {
        QgsDebugMsg( QString( "Extracted cookie: %1" ).arg( cookie ) );
        QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
        QStringList cookieUrls = QgsSettings().value( "/iamauth/cookieurls", "" ).toString().split( ";" );
        for ( const QString &url : cookieUrls )
        {
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url );
        }
      }
    }
  }
  loadStartupProject();
}

void KadasApplication::loadStartupProject()
{
  mMainWindow->catalogBrowser()->reload();

  QgsSettings settings;

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
    if ( !onlineTestUrl.isEmpty() )
    {
      QEventLoop eventLoop;
      QTimer timeout;
      QNetworkReply *reply = QgsNetworkAccessManager::instance()->head( QNetworkRequest( onlineTestUrl ) );
      QObject::connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
      QObject::connect( &timeout, &QTimer::timeout, &eventLoop, &QEventLoop::quit );
      timeout.start( 10000 );
      eventLoop.exec();

      if ( reply->error() == QNetworkReply::NoError && timeout.isActive() && reply->isFinished() )
      {
        settings.setValue( "/kadas/isOffline", false );

        QString projectTemplate = settings.value( "/kadas/onlineDefaultProject" ).toString();
        QString projectTemplateUrl = settings.value( "/kadas/onlineDefaultProjectUrl" ).toString();
        if ( !projectTemplate.isEmpty() )
        {
          projectTemplate = QDir( Kadas::projectTemplatesPath() ).absoluteFilePath( projectTemplate );
          projectCreateFromTemplate( projectTemplate, QString() );
        }
        else if ( !projectTemplateUrl.isEmpty() )
        {
          QEventLoop eventLoop;
          QTimer timeout;
          QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( QNetworkRequest( projectTemplateUrl ) );
          QObject::connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
          QObject::connect( &timeout, &QTimer::timeout, &eventLoop, &QEventLoop::quit );
          timeout.start( 10000 );
          eventLoop.exec();
          QByteArray response = reply->readAll();
          QJsonDocument doc = QJsonDocument::fromJson( response );
          if ( doc.isObject() )
          {
            QJsonArray results = doc.object().value( "results" ).toArray();
            if ( !results.isEmpty() )
            {
              QJsonObject result = results[0].toObject();
              QString baseUrl = projectTemplateUrl.replace( QRegularExpression( "/rest/search/?\?.*$" ), "/rest/content/items/" );
              QUrl url( baseUrl + result["id"].toString() + "/data" );
              url.setFragment( result["title"].toString() + ".qgz" );
              projectCreateFromTemplate( QString(), url );
            }
          }

        }
      }
      else
      {
        settings.setValue( "/kadas/isOffline", true );

        QString projectTemplate = settings.value( "/kadas/offlineDefaultProject" ).toString();
        if ( !projectTemplate.isEmpty() )
        {
          projectTemplate = QDir( Kadas::projectTemplatesPath() ).absoluteFilePath( projectTemplate );
          projectCreateFromTemplate( projectTemplate, QString() );
        }
      }
      delete reply;
    }


  }

  updateWindowTitle();

  QObject::connect( this, &QApplication::lastWindowClosed, this, &QApplication::quit );
  mAutosaveTimer.setSingleShot( true );
  connect( &mAutosaveTimer, &QTimer::timeout, this, &KadasApplication::autosave );
}

QgsRasterLayer *KadasApplication::addRasterLayer( const QString &uri, const QString &layerName, const QString &providerKey, bool quiet, int insOffset, bool adjustInsertionPoint ) const
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
    if ( adjustInsertionPoint )
    {
      QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
      insOffset += computeLayerGroupInsertionOffset( rootGroup );
      QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
    }
    QgsProject::instance()->addMapLayer( layer );
    if ( adjustInsertionPoint )
    {
      QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
    }
  }
  else
  {
    if ( layer->providerType() == QLatin1String( "gdal" ) && !layer->subLayers().empty() )
    {
      QList<QgsMapLayer *> subLayers = showGDALSublayerSelectionDialog( layer );
      if ( adjustInsertionPoint )
      {
        QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
        insOffset += computeLayerGroupInsertionOffset( rootGroup );
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
      }
      QgsProject::instance()->addMapLayers( subLayers );
      if ( adjustInsertionPoint )
      {
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
      }

      // The first layer loaded is not useful in that case. The user can select it in the list if he wants to load it.
      delete layer;
      layer = !subLayers.isEmpty() ? qobject_cast< QgsRasterLayer * > ( subLayers.at( 0 ) ) : nullptr;
    }
    else
    {
      if ( !quiet )
      {
        QString title = tr( "Invalid Layer" );
        QgsError error = layer->error();
        mMainWindow->messageBar()->pushMessage( title, error.message( QgsErrorMessage::Text ),
                                                Qgis::Critical, mMainWindow->messageTimeout() );
      }
      delete layer;
      layer = nullptr;
    }
  }

  return layer;
}

QgsVectorLayer *KadasApplication::addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey, bool quiet, int insOffset, bool adjustInsertionPoint )  const
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
    if ( !QgsAuthGuiUtils::isDisabled( mMainWindow->messageBar() ) )
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
      if ( adjustInsertionPoint )
      {
        QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
        insOffset += computeLayerGroupInsertionOffset( rootGroup );
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
      }
      QgsProject::instance()->addMapLayers( subLayers );
      if ( adjustInsertionPoint )
      {
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
      }

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

      if ( adjustInsertionPoint )
      {
        QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
        insOffset += computeLayerGroupInsertionOffset( rootGroup );
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
      }
      QgsProject::instance()->addMapLayer( layer );
      if ( adjustInsertionPoint )
      {
        QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
      }
    }
  }
  else
  {
    if ( !quiet )
    {
      QString title = tr( "Invalid Layer" );
      QgsError error = layer->error();
      mMainWindow->messageBar()->pushMessage( title, error.message( QgsErrorMessage::Text ),
                                              Qgis::Critical, mMainWindow->messageTimeout() );
    }
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

void KadasApplication::addRasterLayers( const QStringList &layerUris, bool quiet )  const
{
  if ( layerUris.empty() )
  {
    return;
  }

  // this is messy since some files in the list may be rasters and others may
  // be ogr layers. We'll set returnValue to false if one or more layers fail
  // to load.
  for ( const QString &src : layerUris )
  {
    QString errMsg;

    if ( QgsRasterLayer::isValidRasterFileName( src, errMsg ) )
    {
      QFileInfo myFileInfo( src );

      // set the layer name to the file base name unless provided explicitly
      QString layerName;
      const QVariantMap uriDetails = QgsProviderRegistry::instance()->decodeUri( QStringLiteral( "gdal" ), src );
      if ( !uriDetails[ QStringLiteral( "layerName" ) ].toString().isEmpty() )
      {
        layerName = uriDetails[ QStringLiteral( "layerName" ) ].toString();
      }
      else
      {
        layerName = QgsProviderUtils::suggestLayerNameFromFilePath( src );
      }

      // try to create the layer
      QgsRasterLayer *layer = addRasterLayer( src, layerName, QStringLiteral( "gdal" ), quiet );

      if ( layer && layer->isValid() )
      {
        //only allow one copy of a ai grid file to be loaded at a
        //time to prevent the user selecting all adfs in 1 dir which
        //actually represent 1 coverage,

        if ( myFileInfo.fileName().endsWith( QLatin1String( ".adf" ), Qt::CaseInsensitive ) )
        {
          break;
        }
      }
      // if layer is invalid addLayerPrivate() will show the error

    } // valid raster filename
    else
    {
      // Issue message box warning unless we are loading from cmd line since
      // non-rasters are passed to this function first and then successfully
      // loaded afterwards (see main.cpp)
      if ( !quiet )
      {
        QString msg = tr( "%1 is not a supported raster data source" ).arg( src );
        if ( !errMsg.isEmpty() )
          msg += '\n' + errMsg;

        mMainWindow->messageBar()->pushMessage( tr( "Unsupported Data Source" ), msg, Qgis::MessageLevel::Critical );
      }
    }
  }
}

QgsVectorTileLayer *KadasApplication::addVectorTileLayer( const QString &url, const QString &baseName, bool quiet )
{
  QgsSettings settings;

  QString base( baseName );

  if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
  {
    base = QgsMapLayer::formatLayerName( base );
  }

  QgsDebugMsgLevel( "completeBaseName: " + base, 2 );

  // create the layer
  std::unique_ptr<QgsVectorTileLayer> layer( new QgsVectorTileLayer( url, base ) );

  if ( !layer || !layer->isValid() )
  {
    if ( !quiet )
    {
      QString msg = tr( "%1 is not a valid or recognized data source." ).arg( url );
      mMainWindow->messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
    }

    // since the layer is bad, stomp on it
    return nullptr;
  }
  bool ok = false;
  QString error = layer->loadDefaultStyle( ok );
  if ( !ok )
    mMainWindow->messageBar()->pushMessage( tr( "Error loading style" ), error, Qgis::MessageLevel::Warning );
  error = layer->loadDefaultMetadata( ok );
  if ( !ok )
    mMainWindow->messageBar()->pushMessage( tr( "Error loading layer metadata" ), error, Qgis::MessageLevel::Warning );

  QgsProject::instance()->addMapLayer( layer.get() );

  return layer.release();
}

QgsPointCloudLayer *KadasApplication::addPointCloudLayer( const QString &uri, const QString &baseName, const QString &providerKey, bool quiet )
{
  QgsSettings settings;

  QString base( baseName );

  if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
  {
    base = QgsMapLayer::formatLayerName( base );
  }

  QgsDebugMsgLevel( "completeBaseName: " + base, 2 );

  // create the layer
  std::unique_ptr<QgsPointCloudLayer> layer( new QgsPointCloudLayer( uri, base, providerKey ) );

  if ( !layer || !layer->isValid() )
  {
    if ( !quiet )
    {
      QString msg = tr( "%1 is not a valid or recognized data source." ).arg( uri );
      mMainWindow->messageBar()->pushMessage( tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
    }

    // since the layer is bad, stomp on it
    return nullptr;
  }
  bool ok = false;
  layer->loadDefaultStyle( ok );
  layer->loadDefaultMetadata( ok );

  QgsProject::instance()->addMapLayer( layer.get() );

  return layer.release();
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
    item->setup( attachedPath, 0.5, 0.5, 0, 64 );
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

KadasItemLayer *KadasApplication::selectPasteTargetItemLayer( const QList<KadasMapItem *> &items )
{
  QDialog dialog;
  dialog.setWindowTitle( tr( "Select layer" ) );
  dialog.setLayout( new QVBoxLayout() );
  dialog.layout()->setMargin( 2 );
  dialog.layout()->addWidget( new QLabel( tr( "Select layer to paste items to:" ) ) );
  KadasLayerSelectionWidget *layerSelectionWidget = new KadasLayerSelectionWidget( mMainWindow->mapCanvas(), mMainWindow->layerTreeView(), [&]( QgsMapLayer * layer )
  {
    if ( !dynamic_cast<KadasItemLayer *>( layer ) )
    {
      return false;
    }
    for ( const KadasMapItem *item : items )
    {
      if ( !static_cast<KadasItemLayer *>( layer )->acceptsItem( item ) )
      {
        return false;
      }
    }
    return true;
  } );
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

bool KadasApplication::projectCreateFromTemplate( const QString &templateFile, const QUrl &templateUrl )
{
  if ( !templateUrl.isEmpty() )
  {
    QNetworkRequest request = QNetworkRequest( templateUrl );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( request );
    QEventLoop loop;
    connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    loop.exec( QEventLoop::ExcludeUserInputEvents );
    QTextStream( stdout ) << "xxx " << reply->request().url().toString() << " " << reply->error() << Qt::endl;
    if ( reply->error() != QNetworkReply::NoError )
    {
      QgsDebugMsg( QString( "Could not read %1" ).arg( templateUrl.toString() ) );
      QMessageBox::critical( mMainWindow, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return false;
    }
    QString projectFileName = templateUrl.fragment();

    QByteArray data = reply->readAll();
    QBuffer buf( &data );
    QuaZip zip( &buf );
    zip.open( QuaZip::mdUnzip );
    if ( !zip.setCurrentFile( projectFileName, QuaZip::csInsensitive ) )
    {
      QgsDebugMsg( QString( "Could not find file %1 in archive %2" ).arg( projectFileName, templateUrl.toString() ) );
      QMessageBox::critical( mMainWindow, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return false;
    }
    QuaZipFile zipFile( &zip );

    QFile unzipFile( mProjectTempDir->filePath( projectFileName ) );
    if ( zipFile.open( QIODevice::ReadOnly ) && unzipFile.open( QIODevice::WriteOnly ) )
    {
      unzipFile.write( zipFile.readAll() );
    }
    else
    {
      QgsDebugMsg( QString( "Could not extract file %1 from archive %2 to dir %3" ).arg( projectFileName, templateUrl.toString(), mProjectTempDir->path() ) );
      QMessageBox::critical( mMainWindow, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return false;
    }
    if ( projectOpen( unzipFile.fileName() ) )
    {
      QgsProject::instance()->setFileName( QString() );
      return true;
    }
  }
  else if ( !templateFile.isEmpty() && projectOpen( templateFile ) )
  {
    QgsProject::instance()->setFileName( QString() );
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

  // Add default print templates if none are loaded
  bool haveLayouts = false;
  for ( const QString &file : QgsProject::instance()->attachedFiles() )
  {
    if ( file.endsWith( ".qpt" ) )
    {
      haveLayouts = true;
      break;
    }
  }
  if ( !haveLayouts )
  {
    addDefaultPrintTemplates();
  }

  // Ensure WGS84 ellipsoid
  QgsProject::instance()->setEllipsoid( "WGS84" );

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
        QMessageBox::Yes | QMessageBox::Cancel | QMessageBox::No );
    if ( response == QMessageBox::Yes )
    {
      return projectSave();
    }
    else if ( response == QMessageBox::No )
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
  mMainWindow->mapCanvas()->refresh();

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
    // Deletes on close
    ( new KadasAttributeTableDialog( vlayer, mMainWindow->mapCanvas(), mMainWindow->messageBar(), mMainWindow ) )->show();
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
    dialog.findChild<QLineEdit *>( "mSearchLineEdit" )->setHidden( true );
    QList<int> panelIndices;
    panelIndices << dialogPanelIndex( "mOptsPage_Server", stackedWidget );
    std::sort( panelIndices.begin(), panelIndices.end() );
    for ( int i = panelIndices.length() - 1; i >= 0; --i )
    {
      optionsWidget->item( panelIndices[i] )->setHidden( true );
    }

    dialog.exec();
  }
  else if ( layer->type() == QgsMapLayerType::VectorLayer )
  {
    QgsVectorLayerProperties dialog( mainWindow()->mapCanvas(), mainWindow()->messageBar(), static_cast<QgsVectorLayer *>( layer ), mMainWindow );
    // Omit some panels
    QStackedWidget *stackedWidget = dialog.findChild<QStackedWidget *>( "mOptionsStackedWidget" );
    QListWidget *optionsWidget = dialog.findChild<QListWidget *>( "mOptionsListWidget" );
    dialog.findChild<QLineEdit *>( "mSearchLineEdit" )->setHidden( true );
    QList<int> panelIndices;
    panelIndices << dialogPanelIndex( "mOptsPage_AttributesForm", stackedWidget );
    panelIndices << dialogPanelIndex( "mOptsPage_AuxiliaryStorage", stackedWidget );
    panelIndices << dialogPanelIndex( "mOptsPage_Variables", stackedWidget );
    panelIndices << dialogPanelIndex( "mOptsPage_DataDependencies", stackedWidget );
    panelIndices << dialogPanelIndex( "mOptsPage_Server", stackedWidget );
    std::sort( panelIndices.begin(), panelIndices.end() );
    for ( int i = panelIndices.length() - 1; i >= 0; --i )
    {
      optionsWidget->item( panelIndices[i] )->setHidden( true );
    }

    for ( QgsMapLayerConfigWidgetFactory *factory : mMapLayerPanelFactories )
    {
      dialog.addPropertiesPageFactory( factory );
    }
    dialog.exec();
  }
  else if ( layer->type() == QgsMapLayerType::VectorTileLayer )
  {
    QgsVectorTileLayerProperties dialog( static_cast<QgsVectorTileLayer *>( layer ), mainWindow()->mapCanvas(), mainWindow()->messageBar(), mMainWindow );
    dialog.exec();
  }
  else if ( qobject_cast<KadasPluginLayer *>( layer ) )
  {
    KadasPluginLayerProperties dialog( static_cast<KadasPluginLayer *>( layer ), mainWindow()->mapCanvas(), mainWindow() );
    for ( QgsMapLayerConfigWidgetFactory *factory : mMapLayerPanelFactories )
    {
      dialog.addPropertiesPageFactory( factory );
    }
    dialog.exec();
  }
}

int KadasApplication::dialogPanelIndex( const QString &name, QStackedWidget *stackedWidget )
{
  QWidget *widget = stackedWidget->findChild<QWidget *>( name );
  return stackedWidget->indexOf( widget );
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
  if ( QgsProject::instance()->isDirty() )
  {
    if ( !QgsProject::instance()->fileName().isEmpty() )
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
    else
    {
      mMainWindow->statusBar()->showMessage( tr( "Unsaved project from template, autosave disabled" ), 3000 );
    }
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
    KadasItemLayer *layer = kApp->selectPasteTargetItemLayer( items );
    if ( !layer )
    {
      qDeleteAll( items );
      return nullptr;
    }
    else if ( items.size() == 1 )
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
    KadasItemLayer *layer = kApp->selectPasteTargetItemLayer( items );
    if ( !layer )
    {
      qDeleteAll( items );
      return nullptr;
    }
    else if ( items.size() == 1 )
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

void KadasApplication::extentChanged()
{
  // allow symbols in the legend update their preview if they use map units
  const QgsMapCanvas *mapCanvas = mMainWindow->mapCanvas();
  mMainWindow->layerTreeView()->layerTreeModel()->setLegendMapViewData(
    mapCanvas->mapUnitsPerPixel(),
    static_cast< int >( std::round( mapCanvas->mapSettings().outputDpi() ) ), mapCanvas->scale()
  );
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
  KadasCanvasContextMenu( mMainWindow->mapCanvas(), mapPos ).exec( mMainWindow->mapCanvas()->mapToGlobal( screenPos ) );
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
      name.remove( name.split( QgsDataProvider::sublayerSeparator() )[0] );
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
  if ( path.isEmpty() )
  {
    return path;
  }

  static DataSourceMigrations dataSourceMap = dataSourceMigrationMap();

  // Try as file
  QString normPath = path.toLower().replace( "\\", "/" );
  auto it = dataSourceMap.files.find( normPath );
  if ( it != dataSourceMap.files.end() )
  {
    return it.value();
  }

  // Try as wms/wmts
  QUrlQuery query( path );
  if ( !query.isEmpty() )
  {
    QString url = query.queryItemValue( "url" );
    QString layers = query.queryItemValue( "layers" );
    auto itUrl = dataSourceMap.wms.find( url );
    if ( itUrl != dataSourceMap.wms.end() )
    {
      auto itId = itUrl.value().find( layers );
      if ( itId != itUrl.value().end() )
      {
        QUrlQuery newParams( itId.value().first );
        for ( auto keyVal : newParams.queryItems() )
        {
          query.removeAllQueryItems( keyVal.first );
          query.addQueryItem( keyVal.first, keyVal.second );
        }
        for ( const QString &delParam : itId.value().second.split( "," ) )
        {
          query.removeAllQueryItems( delParam );
        }
        return query.toString();
      }
    }
  }

  // Try as ams
  QgsDataSourceUri uri( path );
  if ( uri.hasParam( "url" ) )
  {
    auto it = dataSourceMap.ams.find( uri.param( "url" ) );
    if ( it != dataSourceMap.ams.end() )
    {
      QUrlQuery newParams( it.value() );
      for ( auto keyVal : newParams.queryItems() )
      {
        uri.removeParam( keyVal.first );
        uri.setParam( keyVal.first, keyVal.second );
      }
      return uri.uri();
    }
  }

  // Perform string replacements
  QString newPath = path;
  for ( const QPair<QString, QString> &entry : dataSourceMap.strings )
  {
    newPath.replace( entry.first, entry.second, Qt::CaseInsensitive );
  }

  // Return unchanged
  return newPath;
}

KadasApplication::DataSourceMigrations KadasApplication::dataSourceMigrationMap() const
{
  DataSourceMigrations dataSourceMigrations;
  QString migrationsFilename = qgetenv( "KADAS_DATASOURCE_MIGRATIONS" );
  if ( migrationsFilename.isEmpty() )
  {
    migrationsFilename = "datasource_migrations.json";
  }
  QFile migrationsFile( QDir( Kadas::pkgDataPath() ).absoluteFilePath( migrationsFilename ) );
  if ( migrationsFile.open( QIODevice::ReadOnly ) )
  {
    QJsonDocument doc = QJsonDocument::fromJson( migrationsFile.readAll() );

    QJsonArray fileEntries = doc.object()["files"].toArray();
    for ( int i = 0, n = fileEntries.size(); i < n; ++i )
    {
      QJsonObject entry = fileEntries.at( i ).toObject();
      dataSourceMigrations.files.insert( entry.value( "old" ).toString(), entry.value( "new" ).toString() );
    }

    QJsonArray wmsEntries = doc.object()["wms"].toArray();
    for ( int i = 0, n = wmsEntries.size(); i < n; ++i )
    {
      QJsonObject entry = wmsEntries.at( i ).toObject();
      QString old_url = entry.value( "old_url" ).toString();
      QString old_ident = entry.value( "old_ident" ).toString();
      QString new_params = entry.value( "new_params" ).toString();
      QString del_params = entry.value( "del_params" ).toString();
      auto it = dataSourceMigrations.wms.find( old_url );
      if ( it == dataSourceMigrations.wms.end() )
      {
        it = dataSourceMigrations.wms.insert( old_url, QMap<QString, QPair<QString, QString>>() );
      }
      it.value()[old_ident] = qMakePair( new_params, del_params );
    }

    QJsonArray amsEntries = doc.object()["ams"].toArray();
    for ( int i = 0, n = amsEntries.size(); i < n; ++i )
    {
      QJsonObject entry = amsEntries.at( i ).toObject();
      dataSourceMigrations.ams.insert( entry.value( "old_url" ).toString(), entry.value( "new_params" ).toString() );
    }

    QJsonArray stringEntries = doc.object()["strings"].toArray();
    for ( int i = 0, n = stringEntries.size(); i < n; ++i )
    {
      QJsonObject entry = stringEntries.at( i ).toObject();
      dataSourceMigrations.strings.append( qMakePair( entry.value( "old" ).toString(), entry.value( "new" ).toString() ) );
    }
  }
  return dataSourceMigrations;
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

void KadasApplication::showPythonConsole()
{
  mPythonIntegration->showConsole();
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


void KadasApplication::injectAuthToken( QNetworkRequest *request )
{
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

  QUrl url = request->url();
  QgsDebugMsg( QString( "injectAuthToken: got url %1" ).arg( url.url() ) );
  // Extract the token from the esri_auth cookie, if such cookie exists in the pool
  QList<QNetworkCookie> cookies = nam->cookieJar()->cookiesForUrl( request->url() );
  for ( const QNetworkCookie &cookie : cookies )
  {
    QgsDebugMsg( QString( "injectAuthToken: got cookie %1 for url %2" ).arg( QString::fromUtf8( cookie.toRawForm() ) ).arg( url.url() ) );
    QByteArray data = QUrl::fromPercentEncoding( cookie.toRawForm() ).toLocal8Bit();
    if ( data.startsWith( "esri_auth=" ) )
    {
      QRegExp tokenRe( "\"token\":\\s*\"([A-Za-z0-9-_\\.]+)\"" );
      if ( tokenRe.indexIn( QString( data ) ) != -1 )
      {
        QUrlQuery query( url );
        if ( query.hasQueryItem( "token" ) )
        {
          continue;
        }
        query.addQueryItem( "token", tokenRe.cap( 1 ) );
        url.setQuery( query );
        request->setUrl( url );
        QgsDebugMsg( QString( "injectAuthToken: url altered to %1" ).arg( url.toString() ) );
        break;
      }
    }
  }
}

int KadasApplication::computeLayerGroupInsertionOffset( QgsLayerTreeGroup *group ) const
{
  // Set insertion point above the topmost raster or vector layer or group
  int pos = 0;
  for ( int i = 0, n = group->children().size(); i < n; ++i )
  {
    QgsLayerTreeNode *node = group->children()[i];
    if ( node->nodeType() == QgsLayerTreeNode::NodeLayer )
    {
      QgsLayerTreeLayer *layerNode = static_cast<QgsLayerTreeLayer *>( node );
      if ( layerNode->layer()->type() == QgsMapLayerType::RasterLayer || layerNode->layer()->type() == QgsMapLayerType::VectorLayer )
      {
        pos = i;
        break;
      }
    }
    else if ( node->nodeType() == QgsLayerTreeNode::NodeGroup )
    {
      pos = i;
      break;
    }
  }
  return pos;
}
