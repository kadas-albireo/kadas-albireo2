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
#include <qgis/qgsdatumtransformdialog.h>
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
#include <qgis/qgstaskmanager.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectortilelayer.h>
#include <qgis/qgsvectortilelayerproperties.h>
#include <qgis/qgsvectorlayerproperties.h>
#include <qgis/qgszipitem.h>
#include <qgis/qgsziputils.h>
#include <qgis/qgsdockablewidgethelper.h>

#include "kadas/core/kadas.h"
#include "kadas/gui/kadasattributetabledialog.h"
#include "kadas/gui/kadasclipboard.h"
#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/kadaslayerselectionwidget.h"
#include "kadas/gui/kadasmapcanvasitemmanager.h"
#include "kadas/gui/kadasprojectmigration.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/mapitems/kadaspictureitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"
#include "kadas/gui/maptools/kadasmaptooledititem.h"
#include "kadas/gui/maptools/kadasmaptooledititemgroup.h"
#include "kadas/gui/maptools/kadasmaptoolpan.h"
#include "kadas/gui/milx/kadasmilxlayer.h"
#include "kadasapplication.h"
#include "kadasapplayerhandling.h"
#include "kadascanvascontextmenu.h"
#ifdef WITH_CRASHREPORT
#include "kadascrashrpt.h"
#endif
#include "kadashandlebadlayers.h"
#include "kadaspluginlayerproperties.h"
#include "kadaslayoutdesignermanager.h"
#include "kadaslayerrefreshmanager.h"
#include "kadasmainwindow.h"
#include "kadasmapwidgetmanager.h"
#include "kadasmessagelogviewer.h"
#include "kadasnewspopup.h"
#include "kadasplugininterfaceimpl.h"
#include "kadaspluginmanager.h"
#include "kadaspythonintegration.h"
#include "kadasbullseyelayer.h"
#include "kadasguidegridlayer.h"
#include "kadasmapgridlayer.h"


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

  QgsSettings settings;

  // Translations
  QString locale = QLocale::system().name();
  if ( settings.value( "/locale/overrideFlag", false ).toBool() )
  {
    locale = settings.value( "/locale/userLocale", locale ).toString();
  }
  else
  {
    settings.setValue( "/locale/userLocale", locale );
  }
  KadasApplication::setTranslation( locale );

  QTranslator *translator = new QTranslator( this );
  QString qm_file = QString( "kadas_%1" ).arg( translation() );
  if (! translator->load( qm_file, QStringLiteral( ":/i18n/" ) ) )
    qWarning() << QString( "Could not load translation %1" ).arg( qm_file );
  QApplication::instance()->installTranslator( translator );

  mProjectTempDir = new QTemporaryDir();
  mProjectTempDir->setAutoRemove( true );


#ifdef WITH_CRASHREPORT
  // Install crash reporter
  KadasCrashRpt::install();
#endif

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
  QFile srcSettings;
  bool settingsEmpty = false;
  if ( settings.value( "timestamp", 0 ).toInt() > 0 )
  {
    QgsDebugMsgLevel( "Patching settings" , 1 );
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_patch.ini" ) );
  }
  else
  {
    QgsDebugMsgLevel( "Copying full settings" , 1 );
    settingsEmpty = true;
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_full.ini" ) );
  }
  if ( srcSettings.exists() )
  {
    QgsDebugMsgLevel( QString( "Reading settings from %1" ).arg( srcSettings.fileName() ), 1 );
    QgsSettings newSettings( srcSettings.fileName(), QSettings::IniFormat );
    QString timestamp = settings.value( "timestamp", "0" ).toString();
    QString newtimestamp = newSettings.value( "timestamp" ).toString();
    if ( settingsEmpty || newtimestamp > timestamp )
    {
      settings.setValue( "timestamp", newtimestamp );
      // Merge new settings to old settings
      mergeChildSettingsGroups( settings, newSettings );
    }
    settings.sync();
  }
  else
  {
    QgsDebugMsgLevel( QString( "Could not find settings settings file %1" ).arg( srcSettings.fileName() ), 1 );
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
    QgsDebugMsgLevel( QString( "Network request: %1" ).arg( req->url().toString() ) , 2 );
  } );

  // Create main window
  QSplashScreen splash( QPixmap( ":/kadas/splash" ) );
  splash.show();
  mMainWindow = new KadasMainWindow();
  mMainWindow->init();

  connect( mMainWindow->layerTreeView(), &QgsLayerTreeView::currentLayerChanged, this, &KadasApplication::onActiveLayerChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &KadasApplication::onMapToolChanged );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &KadasApplication::unsetMapTool );
  connect( mMainWindow->mapCanvas(), &QgsMapCanvas::extentsChanged, this, &KadasApplication::extentChanged );
  connect( QgsProject::instance(), &QgsProject::dirtySet, this, &KadasApplication::projectDirtySet );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasApplication::updateWindowTitle );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasApplication::restoreAttributeTables );
  connect( QgsProject::instance(), &QgsProject::projectSaved, this, &KadasApplication::updateWindowTitle );
  // Unset any active tool before writing project to ensure that any pending edits are committed
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasApplication::unsetMapToolOnSave );
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasApplication::saveAttributeTableDocks );
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

  QgsDockableWidgetHelper::sAddTabifiedDockWidgetFunction = [](Qt::DockWidgetArea dockArea, QDockWidget* dock, const QStringList& tabSiblings, bool raiseTab)
  {
    // If we want to add tabified dock widgets as QGIS does, we need to implement this
    // KadasApplication::instance()->addTabifiedDockWidget(dockArea, dock, tabSiblings, raiseTab);
  };
  QgsDockableWidgetHelper::sAppStylesheetFunction = []()->QString
  {
    return KadasApplication::instance()->styleSheet();
  };
  QgsDockableWidgetHelper::sOwnerWindow = mMainWindow;


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
#ifdef WITH_BINDINGS
  mPythonInterface = new KadasPluginInterfaceImpl( this );
  loadPythonSupport();
#endif

  // Layer refresh manager
  mLayerRefreshManager = new KadasLayerRefreshManager( this );

  mMainWindow->show();
  splash.finish( mMainWindow );
  processEvents();

  // Setup layout item widgets
  QgsLayoutGuiUtils::registerGuiForKnownItemTypes( mMainWindow->mapCanvas() );

  // Init KadasItemLayerRegistry
  KadasItemLayerRegistry::init();

  // Extract portal token if necessary before loading startup project
  QString tokenUrl = settings.value( "/portal/token-url" ).toString();
  if ( !tokenUrl.isEmpty() )
  {
    QgsDebugMsgLevel( QString( "Extracting portal TOKEN from %1" ).arg( tokenUrl ), 1 );
    QNetworkRequest req = QNetworkRequest( QUrl( tokenUrl ) );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( reply, &QNetworkReply::finished, this, &KadasApplication::extractPortalToken );
  }
  else
  {
    QgsDebugMsgLevel( QString( "No TOKEN url defined for portal" ), 1 );
    loadStartupProject();
  }

  // Show news popup
  KadasNewsPopup::showIfNewsAvailable();

  // Continue loading application after exec()
  QTimer::singleShot(1, this, &KadasApplication::initAfterExec);
}

void KadasApplication::mergeChildSettingsGroups( QgsSettings &settings, QgsSettings &newSettings )
{
  for ( const QString &group : newSettings.childGroups() )
  {
    newSettings.beginGroup( group );
    settings.beginGroup( group );
    for ( const QString &key : newSettings.childKeys() )
    {
      settings.setValue( key, newSettings.value( key ) );
    }
    mergeChildSettingsGroups( settings, newSettings );
    newSettings.endGroup();
    settings.endGroup();
  }
}

void KadasApplication::extractPortalToken()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( reply->error() == QNetworkReply::NoError )
  {
    QByteArray data = reply->readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson( data, &err );
    if ( !doc.isNull() )
    {
      QJsonObject obj = doc.object();
      if ( obj.contains( QStringLiteral( "token" ) ) )
      {
        QgsDebugMsgLevel( QString( "ESRI Token found" ), 1 );
        QString cookie = QString( "agstoken=\"token\": \"%1\"" ).arg( obj[QStringLiteral( "token" )].toString() );
        QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
        QStringList cookieUrls = QgsSettings().value( "/portal/cookieurls", "" ).toString().split( "," );
        for ( const QString &url : cookieUrls )
        {
          QgsDebugMsgLevel( QString( "Setting cookie for url %1" ).arg( url ) , 1 );
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url.trimmed() );
        }
      }
    }
    else
    {
      QgsDebugMsgLevel( QString( "could not read TOKEN from response: %1" ).arg( err.errorString() ), 1 );
    }
  }
  else
  {
    QgsDebugMsgLevel( QString( "error fetching token %1" ).arg( reply->error() ), 1 );
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
  if ( adjustInsertionPoint )
  {
    QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
    insOffset += computeLayerGroupInsertionOffset( rootGroup );
    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
  }

  QgsRasterLayer *layer = KadasAppLayerHandling::addRasterLayer( uri, layerName, providerKey, !quiet );

  QgsProject::instance()->addMapLayer( layer );
  if ( adjustInsertionPoint )
  {
    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
  }
  return layer;
}

QgsVectorLayer *KadasApplication::addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey, bool quiet, int insOffset, bool adjustInsertionPoint )  const
{
  if ( adjustInsertionPoint )
  {
    QgsLayerTreeGroup *rootGroup = mMainWindow->layerTreeView()->layerTreeModel()->rootGroup();
    insOffset += computeLayerGroupInsertionOffset( rootGroup );
    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( rootGroup, insOffset ) );
  }

  QgsVectorLayer *layer = KadasAppLayerHandling::addVectorLayer( uri, layerName, providerKey, !quiet );

  QgsProject::instance()->addMapLayer( layer );
  if ( adjustInsertionPoint )
  {
    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTreeRegistryBridge::InsertionPoint( mMainWindow->layerTreeView()->layerTreeModel()->rootGroup(), 0 ) );
  }
  return layer;
}

void KadasApplication::addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType, bool quiet ) const
{
  bool ok = false;
  KadasAppLayerHandling::addOgrVectorLayers( layerUris, enc, dataSourceType, ok, !quiet );
}

void KadasApplication::addRasterLayers( const QStringList &layerUris, bool quiet )  const
{
  bool ok = false;
  KadasAppLayerHandling::addGdalRasterLayers( layerUris, ok, !quiet );
}

QgsVectorTileLayer *KadasApplication::addVectorTileLayer( const QString &url, const QString &baseName, bool quiet )
{
  return KadasAppLayerHandling::addVectorTileLayer( url, baseName, !quiet );
}

QgsPointCloudLayer *KadasApplication::addPointCloudLayer( const QString &uri, const QString &baseName, const QString &providerKey, bool quiet )
{
  return KadasAppLayerHandling::addPointCloudLayer( uri, baseName, providerKey, !quiet );
}

QPair<KadasMapItem *, KadasItemLayerRegistry::StandardLayer> KadasApplication::addImageItem( const QString &filename ) const
{
  QString attachedPath = QgsProject::instance()->createAttachedFile( QFileInfo( filename ).fileName() );
  QFile( attachedPath ).remove();
  QFile( filename ).copy( attachedPath );
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  QgsCoordinateReferenceSystem crs( "EPSG:3857" );
  QgsCoordinateTransform crst( mMainWindow->mapCanvas()->mapSettings().destinationCrs(), crs, QgsProject::instance()->transformContext() );
  if ( filename.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    KadasSymbolItem *item = new KadasSymbolItem( crs );
    item->setup( attachedPath, 0.5, 0.5, 0, 64 );
    item->setPosition( KadasItemPos::fromPoint( crst.transform( mMainWindow->mapCanvas()->extent().center() ) ) );
    return qMakePair( item, KadasItemLayerRegistry::StandardLayer::SymbolsLayer );
  }
  else
  {
    KadasPictureItem *item = new KadasPictureItem( crs );
    item->setup( attachedPath, KadasItemPos::fromPoint( crst.transform( mMainWindow->mapCanvas()->extent().center() ) ) );
    return qMakePair( item, KadasItemLayerRegistry::StandardLayer::PicturesLayer );
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
    if ( reply->error() != QNetworkReply::NoError )
    {
      QgsDebugMsgLevel( QString( "Could not read %1" ).arg( templateUrl.toString() ) , 2 );
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
      QgsDebugMsgLevel( QString( "Could not find file %1 in archive %2" ).arg( projectFileName, templateUrl.toString() ) , 2 );
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
      QgsDebugMsgLevel( QString( "Could not extract file %1 from archive %2 to dir %3" ).arg( projectFileName, templateUrl.toString(), mProjectTempDir->path() ) , 2 );
      QMessageBox::critical( mMainWindow, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return false;
    }
    unzipFile.close();
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

  // Adjust current zoom to an optimal zoom resolution
  if ( !mMainWindow->mapCanvas()->zoomResolutions().isEmpty() )
  {
    double mapRes = mMainWindow->mapCanvas()->mapSettings().mapUnitsPerPixel();
    const QList<double> zoomResolutions = mMainWindow->mapCanvas()->zoomResolutions();
    for ( int i = 0, n = zoomResolutions.size() - 1; i < n; ++i )
    {
      if ( qgsDoubleNear( zoomResolutions[i], mapRes, 0.0001 ) )
      {
        // Already at optimal resolution
        break;
      }
      if ( mapRes >= zoomResolutions[i] && mapRes < zoomResolutions[i + 1] )
      {
        // Zoom in or out, whichever is closer
        if ( mapRes - zoomResolutions[i] < zoomResolutions[i + 1] - mapRes )
        {
          mMainWindow->mapCanvas()->zoomIn();
        }
        else
        {
          mMainWindow->mapCanvas()->zoomOut();
        }
        break;
      }
    }
  }

  return success;
}

void KadasApplication::projectClose()
{
  cleanupAutosave();

  emit projectWillBeClosed();

  mMainWindow->mapCanvas()->freeze( true );

  unsetMapTool();


  KadasLayoutDesignerManager::instance()->closeAllDesigners();

  // clear out any stuff from project
  mMainWindow->mapCanvas()->cancelJobs();
  mMainWindow->mapCanvas()->setLayers( QList<QgsMapLayer *>() );
  mMainWindow->mapCanvas()->clearCache();
  mMainWindow->mapCanvas()->clearExtentHistory();
  mMainWindow->mapCanvas()->freeze( false );

  onActiveLayerChanged( currentLayer() );

  mMainWindow->mapWidgetManager()->clearMapWidgets();
  mMainWindow->resetMagnification();

  KadasMapCanvasItemManager::clear();

  QgsProject::instance()->clear();
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

void KadasApplication::saveAttributeTableDocks( QDomDocument &doc )
{
  QDomElement qgisNode = doc.firstChildElement( QStringLiteral( "qgis" ) );

  // save attribute tables
  QDomElement attributeTablesElement = doc.createElement( QStringLiteral( "attributeTables" ) );

  QSet< KadasAttributeTableDialog * > storedDialogs;
  auto saveDialog = [&storedDialogs, &attributeTablesElement, &doc]( KadasAttributeTableDialog * attributeTableDialog )
  {
    if ( storedDialogs.contains( attributeTableDialog ) )
      return;

    QgsDebugMsgLevel( attributeTableDialog->windowTitle(), 2 );
    const QDomElement tableElement = attributeTableDialog->writeXml( doc );
    attributeTablesElement.appendChild( tableElement );
    storedDialogs.insert( attributeTableDialog );
  };

  const QList<QWidget * > topLevelWidgets = QgsApplication::topLevelWidgets();
  for ( QWidget *widget : topLevelWidgets )
  {
    const QList< KadasAttributeTableDialog * > dialogChildren = widget->findChildren< KadasAttributeTableDialog * >();
    for ( KadasAttributeTableDialog *attributeTableDialog : dialogChildren )
    {
      saveDialog( attributeTableDialog );
    }
  }

  qgisNode.appendChild( attributeTablesElement );
  }

void KadasApplication::restoreAttributeTables(const QDomDocument &doc)
{
  const QDomElement attributeTablesElement = doc.documentElement().firstChildElement( QStringLiteral( "attributeTables" ) );
  const QDomNodeList attributeTableNodes = attributeTablesElement.elementsByTagName( QStringLiteral( "attributeTable" ) );
  for ( int i = 0; i < attributeTableNodes.size(); ++i )
  {
    const QDomElement attributeTableElement = attributeTableNodes.at( i ).toElement();
    KadasAttributeTableDialog::createFromXml(attributeTableElement, mMainWindow->mapCanvas(), mMainWindow->messageBar(), mMainWindow );
  }
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
    KadasAttributeTableDialog* table = new KadasAttributeTableDialog( vlayer, mMainWindow->mapCanvas(), mMainWindow->messageBar(), mMainWindow, KadasAttributeTableDialog::settingsAttributeTableLocation->value() );
    table->show();
  }
}

void KadasApplication::showLayerProperties( QgsMapLayer *layer )
{
  if ( !layer )
    return;

  if ( layer->type() == Qgis::LayerType::Raster )
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
  else if ( layer->type() == Qgis::LayerType::Vector )
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
  else if ( layer->type() == Qgis::LayerType::VectorTile )
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
  QString layerUrl;
  if ( layer->providerType() == "arcgismapserver" || layer->providerType() == "arcgisfeatureserver" )
  {
    layerUrl = QgsDataSourceUri( layer->source() ).param( "url" );
  }
  else if ( layer->providerType() == "wms" )
  {
    layerUrl = QUrlQuery( layer->source() ).queryItemValue( "url" );
  }
  QgsDebugMsgLevel( QString( "GDI layer url is %1" ).arg( layerUrl ) , 2 );
  if ( layerUrl.isEmpty() )
  {
    return;
  }
  QString gdiBaseUrl = QgsSettings().value( "kadas/gdiBaseUrl" ).toString();
  if ( !gdiBaseUrl.endsWith( "/" ) )
  {
    gdiBaseUrl += "/";
  }
  QUrl searchUrl( gdiBaseUrl + "sharing/rest/search?f=pjson&q=" +  QUrl::toPercentEncoding( "url:" + layerUrl ) );
  QgsDebugMsgLevel( QString( "The GDI item search url is %1" ).arg( searchUrl.toString() ) , 2 );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( QNetworkRequest( searchUrl ) );
  connect( reply, &QNetworkReply::finished, [this, gdiBaseUrl, reply]
  {
    QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    QVariantList results = rootMap["results"].toList();
    if ( results.length() == 1 )
    {
      QVariantMap result = results.at( 0 ).toMap();
      QString id = result["id"].toString();
      QString metadataUrl = gdiBaseUrl + "home/item.html?id=" + id;
      QgsDebugMsgLevel( QString( "The GDI item metadata URL is %1" ).arg( metadataUrl ) , 2 );
      QDesktopServices::openUrl( metadataUrl );
    }
    else
    {
      QMessageBox::information( mMainWindow, tr( "No layer info" ), tr( "No info available for this layer" ) );
    }
    reply->deleteLater();
  } );
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
      if ( feature.geometry().type() == Qgis::GeometryType::Point )
      {
        KadasPointItem *item = new KadasPointItem( featureStore.crs() );
        item->addPartFromGeometry( *feature.geometry().constGet() );
        items.append( item );
      }
      else if ( feature.geometry().type() == Qgis::GeometryType::Line )
      {
        KadasLineItem *item = new KadasLineItem( featureStore.crs() );
        item->addPartFromGeometry( *feature.geometry().constGet() );
        items.append( item );
      }
      else if ( feature.geometry().type() == Qgis::GeometryType::Polygon )
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
      return new KadasMapToolEditItem( canvas, item, KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::SymbolsLayer ) );
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
      return new KadasMapToolEditItem( canvas, item, KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::PicturesLayer ) );
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

void KadasApplication::initAfterExec()
{
  // Update plugins
  mainWindow()->pluginManager()->loadPlugins();
  mainWindow()->pluginManager()->updateAllPlugins();
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
  //QgsDebugMsgLevel("Python support library's instance() symbol resolved.", 2 );
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
  if ( QgsSettings().value( "/kadas/injectAuthToken", false ).toBool() == false )
  {
    return;
  }
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

  QUrl url = request->url();
  QUrlQuery query( url );
  if ( query.hasQueryItem( "token" ) )
  {
    return;
  }
  QgsDebugMsgLevel( QString( "injectAuthToken: got url %1" ).arg( url.url() ) , 2 );
  // Extract the token from the esri_auth cookie, if such cookie exists in the pool
  QList<QNetworkCookie> cookies = nam->cookieJar()->cookiesForUrl( request->url() );
  for ( const QNetworkCookie &cookie : cookies )
  {
    QgsDebugMsgLevel( QString( "injectAuthToken: got cookie %1 for url %2" ).arg( QString::fromUtf8( cookie.toRawForm() ) ).arg( url.url() ) , 2 );
    QByteArray data = QUrl::fromPercentEncoding( cookie.toRawForm() ).toLocal8Bit();
    if ( data.startsWith( "agstoken=" ) )
    {
      QRegExp tokenRe( "\"token\":\\s*\"([A-Za-z0-9-_\\.]+)\"" );
      if ( tokenRe.indexIn( QString( data ) ) != -1 )
      {
        query.addQueryItem( "token", tokenRe.cap( 1 ) );
        url.setQuery( query );
        request->setUrl( url );
        QgsDebugMsgLevel( QString( "injectAuthToken: url altered to %1" ).arg( url.toString() ) , 2 );
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
      if ( layerNode->layer()->type() == Qgis::LayerType::Raster || layerNode->layer()->type() == Qgis::LayerType::Vector )
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

bool KadasApplication::askUserForDatumTransform( const QgsCoordinateReferenceSystem &sourceCrs, const QgsCoordinateReferenceSystem &destinationCrs, const QgsMapLayer *layer )
{
  Q_ASSERT( qApp->thread() == QThread::currentThread() );

  QString title;
  if ( layer )
  {
    // try to make a user-friendly (short!) identifier for the layer
    QString layerIdentifier;
    if ( !layer->name().isEmpty() )
    {
      layerIdentifier = layer->name();
    }
    else
    {
      const QVariantMap parts = QgsProviderRegistry::instance()->decodeUri( layer->providerType(), layer->source() );
      if ( parts.contains( QStringLiteral( "path" ) ) )
      {
        const QFileInfo fi( parts.value( QStringLiteral( "path" ) ).toString() );
        layerIdentifier = fi.fileName();
      }
      else if ( layer->dataProvider() )
      {
        const QgsDataSourceUri uri( layer->source() );
        layerIdentifier = uri.table();
      }
    }
    if ( !layerIdentifier.isEmpty() )
      title = tr( "Select Transformation for %1" ).arg( layerIdentifier );
  }

  return QgsDatumTransformDialog::run( sourceCrs, destinationCrs, mMainWindow, mMainWindow->mapCanvas(), title );
}

bool KadasApplication::checkTasksDependOnProject()
{
  QSet< QString > activeTaskDescriptions;
  QMap<QString, QgsMapLayer *> layers = QgsProject::instance()->mapLayers();
  QMap<QString, QgsMapLayer *>::const_iterator layerIt = layers.constBegin();

  for ( ; layerIt != layers.constEnd(); ++layerIt )
  {
    QList< QgsTask * > tasks = QgsApplication::taskManager()->tasksDependentOnLayer( layerIt.value() );
    if ( !tasks.isEmpty() )
    {
      const auto constTasks = tasks;
      for ( QgsTask *task : constTasks )
      {
        activeTaskDescriptions.insert( tr( " • %1" ).arg( task->description() ) );
      }
    }
  }

  if ( !activeTaskDescriptions.isEmpty() )
  {
    QMessageBox::warning( mMainWindow, tr( "Active Tasks" ),
                          tr( "The following tasks are currently running which depend on layers in this project:\n\n%1\n\nPlease cancel these tasks and retry." ).arg( qgis::setToList( activeTaskDescriptions ).join( QLatin1Char( '\n' ) ) ) );
    return true;
  }
  return false;
}
