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
#include <QFile>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QSplashScreen>

#include <qgis/qgsauthguiutils.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsdataitem.h>
#include <qgis/qgsguiutils.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemapcanvasbridge.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgsmapcanvasannotationitem.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsproviderregistry.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssublayersdialog.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsziputils.h>

#include <kadas/core/kadas.h>
#include <kadas/core/kadasitemlayer.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/maptools/kadasmaptoolpan.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadascrashrpt.h>
#include <kadas/app/kadasmainwindow.h>


static QStringList splitSubLayerDef ( const QString& subLayerDef )
{
  QStringList elements = subLayerDef.split ( QgsDataProvider::SUBLAYER_SEPARATOR );
  // merge back parts of the name that may have been split
  while ( elements.size() > 5 ) {
    elements[1] += ":" + elements[2];
    elements.removeAt ( 2 );
  }
  return elements;
}

static void setupVectorLayer ( const QString& vectorLayerPath,
                               const QStringList& sublayers,
                               QgsVectorLayer*& layer,
                               const QString& providerKey,
                               QgsVectorLayer::LayerOptions options )
{
  //set friendly name for datasources with only one layer
  QgsSettings settings;
  QStringList elements = splitSubLayerDef ( sublayers.at ( 0 ) );
  QString rawLayerName = elements.size() >= 2 ? elements.at ( 1 ) : QString();
  QString subLayerNameFormatted = rawLayerName;
  if ( settings.value ( QStringLiteral ( "qgis/formatLayerName" ), false ).toBool() ) {
    subLayerNameFormatted =  QgsMapLayer::formatLayerName ( subLayerNameFormatted );
  }

  if ( elements.size() >= 4 && layer->name().compare ( rawLayerName, Qt::CaseInsensitive ) != 0
       && layer->name().compare ( subLayerNameFormatted, Qt::CaseInsensitive ) != 0 ) {
    layer->setName ( QStringLiteral ( "%1 %2" ).arg ( layer->name(), rawLayerName ) );
  }

  // Systematically add a layername= option to OGR datasets in case
  // the current single layer dataset becomes layer a multi-layer one.
  // Except for a few select extensions, known to be always single layer dataset.
  QFileInfo fi ( vectorLayerPath );
  QString ext = fi.suffix().toLower();
  if ( providerKey == QLatin1String ( "ogr" ) &&
       ext != QLatin1String ( "shp" ) &&
       ext != QLatin1String ( "mif" ) &&
       ext != QLatin1String ( "tab" ) &&
       ext != QLatin1String ( "csv" ) &&
       ext != QLatin1String ( "geojson" ) &&
       ! vectorLayerPath.contains ( QStringLiteral ( "layerid=" ) ) &&
       ! vectorLayerPath.contains ( QStringLiteral ( "layername=" ) ) ) {
    auto uriParts = QgsProviderRegistry::instance()->decodeUri (
                      layer->providerType(), layer->dataProvider()->dataSourceUri() );
    QString composedURI ( uriParts.value ( QStringLiteral ( "path" ) ).toString() );
    composedURI += "|layername=" + rawLayerName;

    auto newLayer = qgis::make_unique<QgsVectorLayer> ( composedURI, layer->name(), QStringLiteral ( "ogr" ), options );
    if ( newLayer && newLayer->isValid() ) {
      delete layer;
      layer = newLayer.release();
    }
  }
}


KadasApplication* KadasApplication::instance()
{
  return qobject_cast<KadasApplication*> ( QCoreApplication::instance() );
}

KadasApplication::KadasApplication ( int& argc, char** argv )
  : QgsApplication ( argc, argv, true )
{

  // Install crash reporter
  KadasCrashRpt crashReporter;
  crashReporter.install();

  QgsApplication::init();
  QgsApplication::initQgis();

  // Setup application style
  setWindowIcon ( QIcon ( ":/kadas/logo" ) );
  QFile styleSheet ( ":/stylesheet" );
  if ( styleSheet.open ( QIODevice::ReadOnly ) ) {
    setStyleSheet ( QString::fromLocal8Bit ( styleSheet.readAll() ) );
  }

  // Create / migrate settings
  QSettings settings;

  QFile srcSettings;
  bool settingsEmpty = false;
  if ( QFile ( settings.fileName() ).exists() ) {
    QgsDebugMsg ( "Patching settings" );
    srcSettings.setFileName ( QDir ( Kadas::pkgDataPath() ).absoluteFilePath ( "settings_patch.ini" ) );
  } else {
    QgsDebugMsg ( "Copying full settings" );
    settingsEmpty = true;
    srcSettings.setFileName ( QDir ( Kadas::pkgDataPath() ).absoluteFilePath ( "settings_full.ini" ) );
  }
  if ( srcSettings.exists() ) {
    QSettings newSettings ( srcSettings.fileName(), QSettings::IniFormat );
    QString timestamp = settings.value ( "timestamp", "0" ).toString();
    QString newtimestamp = newSettings.value ( "timestamp" ).toString();
    if ( settingsEmpty || newtimestamp > timestamp ) {
      settings.setValue ( "timestamp", newtimestamp );
      // Merge new settings to old settings
      for ( const QString& group : newSettings.childGroups() ) {
        newSettings.beginGroup ( group );
        settings.beginGroup ( group );
        for ( const QString& key : newSettings.childKeys() ) {
          settings.setValue ( key, newSettings.value ( key ) );
        }
        newSettings.endGroup();
        settings.endGroup();
      }
    }
  }
  settings.sync();

  // Setup localization
  QString userLocale = settings.value ( "locale/userLocale", QLocale::system().name() ).toString();

  QString qtTranslationsPath = QLibraryInfo::location ( QLibraryInfo::TranslationsPath );
  QTranslator qtTranslator;
  qtTranslator.load ( "qt_" + userLocale, qtTranslationsPath );
  qtTranslator.load ( "qtbase_" + userLocale, qtTranslationsPath );
  installTranslator ( &qtTranslator );

  QString i18nPath = QDir ( Kadas::pkgDataPath() ).absoluteFilePath ( "i18n" );
  QTranslator appTranslator;
  appTranslator.load ( QString ( "%1_%2" ).arg ( Kadas::KADAS_RELEASE_NAME ), userLocale, i18nPath );
  installTranslator ( &appTranslator );

  // Create main window
  QSplashScreen splash ( QPixmap ( ":/kadas/splash" ) );
  splash.show();
  mClipboard = new KadasClipboard ( this );
  mMainWindow = new KadasMainWindow ( &splash );
  mMainWindow->mapCanvas()->setCanvasColor ( Qt::transparent );
  mMainWindow->mapCanvas()->setMapUpdateInterval ( 1000 );

  mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge ( QgsProject::instance()->layerTreeRoot(), mMainWindow->mapCanvas(), this );

  connect ( mMainWindow->layerTreeView(), &QgsLayerTreeView::currentLayerChanged, this, &KadasApplication::onActiveLayerChanged );
  connect ( mMainWindow->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &KadasApplication::onMapToolChanged );
  connect ( QgsProject::instance(), &QgsProject::isDirtyChanged, this, &KadasApplication::updateWindowTitle );
  connect ( QgsProject::instance(), &QgsProject::readProject, this, &KadasApplication::updateWindowTitle );
  connect ( QgsProject::instance(), &QgsProject::projectSaved, this, &KadasApplication::updateWindowTitle );
  connect ( this, &KadasApplication::focusChanged, this, &KadasApplication::onFocusChanged );

  QgsLayerTreeModel* layerTreeModel = mMainWindow->layerTreeView()->layerTreeModel();
  connect ( layerTreeModel->rootGroup(), &QgsLayerTreeNode::addedChildren, QgsProject::instance(), &QgsProject::setDirty );
  connect ( layerTreeModel->rootGroup(), &QgsLayerTreeNode::removedChildren, QgsProject::instance(), &QgsProject::setDirty );
  connect ( layerTreeModel->rootGroup(), &QgsLayerTreeNode::visibilityChanged, QgsProject::instance(), &QgsProject::setDirty );

  mMapToolPan = new KadasMapToolPan ( mMainWindow->mapCanvas() );
  mMainWindow->mapCanvas()->setMapTool ( mMapToolPan );

  // Perform online/offline check to select default template
  QString onlineTestUrl = settings.value ( "/kadas/onlineTestUrl" ).toString();
  QString projectTemplate;
  if ( !onlineTestUrl.isEmpty() ) {
    QEventLoop eventLoop;
    QNetworkReply* reply = QgsNetworkAccessManager::instance()->head ( QNetworkRequest ( onlineTestUrl ) );
    QObject::connect ( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
    eventLoop.exec();

    if ( reply->error() == QNetworkReply::NoError ) {
      projectTemplate = settings.value ( "/kadas/onlineDefaultProject" ).toString();
      settings.setValue ( "/kadas/isOffline", false );
    } else {
      projectTemplate = settings.value ( "/kadas/offlineDefaultProject" ).toString();
      settings.setValue ( "/kadas/isOffline", true );
    }
    delete reply;
  }

  if ( !projectTemplate.isEmpty() ) {
    projectTemplate = QDir ( Kadas::projectTemplatesPath() ).absoluteFilePath ( projectTemplate );
    projectOpen ( projectTemplate );
  }

  // TODO: QgsApplication::setMaxThreads( QSettings().value( "/Qgis/max_threads", -1 ).toInt() );

  QgsProject::instance()->setDirty ( false );
  updateWindowTitle();
  mMainWindow->show();
  splash.finish ( mMainWindow );

  QObject::connect ( this, &QApplication::lastWindowClosed, this, &QApplication::quit );
}

KadasApplication::~KadasApplication()
{
  projectClose();
}

QgsRasterLayer* KadasApplication::addRasterLayer ( const QString& uri, const QString& layerName, const QString& providerKey ) const
{
  QgsSettings settings;
  QString baseName = settings.value ( QStringLiteral ( "qgis/formatLayerName" ), false ).toBool() ? QgsMapLayer::formatLayerName ( layerName ) : layerName;
  QgsDebugMsg ( "Creating new raster layer using " + uri + " with baseName " + baseName + " and providerKey " + providerKey );

  QgsRasterLayer* layer = nullptr;
  if ( !providerKey.isEmpty() && uri.endsWith ( QLatin1String ( ".adf" ), Qt::CaseInsensitive ) ) {
    QString dirName = QFileInfo ( uri ).path();
    layer = new QgsRasterLayer ( dirName, QFileInfo ( dirName ).completeBaseName(), QStringLiteral ( "gdal" ) );
  } else if ( providerKey.isEmpty() ) {
    layer = new QgsRasterLayer ( uri, baseName );
  } else {
    layer = new QgsRasterLayer ( uri, baseName, providerKey );
  }

  QgsDebugMsg ( QStringLiteral ( "Constructed new layer" ) );

  if ( layer->isValid() ) {
    QgsProject::instance()->addMapLayer ( layer );
  } else {
    if ( layer->providerType() == QLatin1String ( "gdal" ) && !layer->subLayers().empty() ) {
      QList<QgsMapLayer*> subLayers = showGDALSublayerSelectionDialog ( layer );
      QgsProject::instance()->addMapLayers ( subLayers );

      // The first layer loaded is not useful in that case. The user can select it in the list if he wants to load it.
      delete layer;
      layer = !subLayers.isEmpty() ? qobject_cast< QgsRasterLayer* > ( subLayers.at ( 0 ) ) : nullptr;
    } else {
      QString title = tr ( "Invalid Layer" );
      QgsError error = layer->error();
      mMainWindow->messageBar()->pushMessage ( title, error.message ( QgsErrorMessage::Text ),
          Qgis::Critical, mMainWindow->messageTimeout() );
      delete layer;
      layer = nullptr;
    }
  }

  return layer;
}

QgsVectorLayer* KadasApplication::addVectorLayer ( const QString& uri, const QString& layerName, const QString& providerKey )  const
{
  QgsSettings settings;
  QString baseName = settings.value ( QStringLiteral ( "qgis/formatLayerName" ), false ).toBool() ? QgsMapLayer::formatLayerName ( layerName ) : layerName;
  QgsDebugMsg ( "Creating new vector layer using " + uri + " with baseName " + baseName + " and providerKey " + providerKey );

  // if the layer needs authentication, ensure the master password is set
  bool authok = true;
  QRegExp rx ( "authcfg=([a-z]|[A-Z]|[0-9]){7}" );
  if ( rx.indexIn ( uri ) != -1 ) {
    authok = false;
    if ( !QgsAuthGuiUtils::isDisabled ( mMainWindow->messageBar(), mMainWindow->messageTimeout() ) ) {
      authok = kApp->authManager()->setMasterPassword ( true );
    }
  }

  // create the layer
  bool isVsiCurl = uri.startsWith ( QLatin1String ( "/vsicurl" ), Qt::CaseInsensitive );
  QString scheme = QUrl ( uri ).scheme();
  bool isRemoteUrl = scheme.startsWith ( QStringLiteral ( "http" ) ) || scheme == QStringLiteral ( "ftp" );

  QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
  // Default style is loaded later in this method
  options.loadDefaultStyle = false;
  if ( isVsiCurl || isRemoteUrl ) {
    mMainWindow->messageBar()->pushInfo ( tr ( "Remote layer" ), tr ( "Loading %1, please wait..." ).arg ( uri ) );
    QApplication::setOverrideCursor ( Qt::WaitCursor );
    processEvents();
  }
  QgsVectorLayer* layer = new QgsVectorLayer ( uri, baseName, providerKey, options );
  if ( isVsiCurl || isRemoteUrl ) {
    QApplication::restoreOverrideCursor( );
  }

  QStringList sublayers = layer->dataProvider()->subLayers();
  if ( authok && layer && layer->isValid() ) {
    QgsDebugMsg ( QStringLiteral ( "got valid layer with %1 sublayers" ).arg ( sublayers.count() ) );

    // If the newly created layer has more than 1 layer of data available, we show the
    // sublayers selection dialog so the user can select the sublayers to actually load.
    if ( sublayers.count() > 1 && !uri.contains ( QStringLiteral ( "layerid=" ) ) && !uri.contains ( QStringLiteral ( "layername=" ) ) ) {
      QList<QgsMapLayer*> subLayers = showOGRSublayerSelectionDialog ( layer );
      QgsProject::instance()->addMapLayers ( subLayers );

      // The first layer loaded is not useful in that case. The user can select it in the list if he wants to load it.
      delete layer;
      layer = !subLayers.isEmpty() ? qobject_cast< QgsVectorLayer* > ( subLayers.at ( 0 ) ) : nullptr;
    } else {
      //set friendly name for datasources with only one layer
      if ( !sublayers.isEmpty() ) {
        setupVectorLayer ( uri, sublayers, layer, providerKey, options );
      }

      QgsProject::instance()->addMapLayer ( layer );
    }
  } else {
    QString title = tr ( "Invalid Layer" );
    QgsError error = layer->error();
    mMainWindow->messageBar()->pushMessage ( title, error.message ( QgsErrorMessage::Text ),
        Qgis::Critical, mMainWindow->messageTimeout() );
    delete layer;
    layer = nullptr;
  }

  return layer;
}

void KadasApplication::addVectorLayers ( const QStringList& layerUris, const QString& enc, const QString& dataSourceType ) const
{
  for ( QString uri : layerUris ) {
    uri = uri.trimmed();
    QString baseName;
    if ( dataSourceType == QLatin1String ( "file" ) ) {
      QString srcWithoutLayername ( uri );
      int posPipe = srcWithoutLayername.indexOf ( '|' );
      if ( posPipe >= 0 ) {
        srcWithoutLayername.resize ( posPipe );
      }
      baseName = QFileInfo ( srcWithoutLayername ).completeBaseName();

      // if needed prompt for zipitem layers
      QString vsiPrefix = QgsZipItem::vsiPrefix ( uri );
      if ( ! uri.startsWith ( QLatin1String ( "/vsi" ), Qt::CaseInsensitive ) &&
           ( vsiPrefix == QLatin1String ( "/vsizip/" ) || vsiPrefix == QLatin1String ( "/vsitar/" ) ) ) {
        if ( showZipSublayerSelectionDialog ( uri ) ) {
          continue;
        }
      }
    } else if ( dataSourceType == QLatin1String ( "database" ) ) {
      // Try to extract the database name and use it as base name
      // sublayers names (if any) will be appended to the layer name
      QVariantMap parts = QgsProviderRegistry::instance()->decodeUri ( QStringLiteral ( "ogr" ), uri );
      if ( parts.value ( QStringLiteral ( "layerName" ) ).isValid() ) {
        baseName = parts.value ( QStringLiteral ( "layerName" ) ).toString();
      } else {
        baseName = uri;
      }
    } else { //directory //protocol
      baseName = QFileInfo ( uri ).completeBaseName();
    }
    addVectorLayer ( uri, baseName, QStringLiteral ( "ogr" ) );
  }
}

KadasItemLayer* KadasApplication::getItemLayer ( const QString& layerName ) const
{
  if ( mItemLayerMap.contains ( layerName ) ) {
    return qobject_cast<KadasItemLayer*> ( QgsProject::instance()->mapLayer ( mItemLayerMap[layerName] ) );
  }
  return nullptr;
}

KadasItemLayer* KadasApplication::getOrCreateItemLayer ( const QString& layerName )
{
  KadasItemLayer* layer = getItemLayer ( layerName );
  if ( !layer ) {
    layer = new KadasItemLayer ( layerName );
    mItemLayerMap[layerName] = layer->id();
    QgsProject::instance()->addMapLayer ( layer );
  }
  return layer;
}

void KadasApplication::exportToGpx()
{
  // TODO
}

void KadasApplication::exportToKml()
{
  // TODO
}

void KadasApplication::importFromGpx()
{
  // TODO
}

void KadasApplication::importFromKml()
{
  // TODO
}

void KadasApplication::paste()
{
  // TODO
}

bool KadasApplication::projectCreateFromTemplate ( const QString& templateFile )
{
  if ( projectOpen ( templateFile ) ) {
    QgsProject::instance()->setFileName ( QString() );
    return true;
  }
  return false;
}

bool KadasApplication::projectOpen ( const QString& projectFile )
{
  if ( !projectSaveDirty() ) {
    return false;
  }
  QString fileName = projectFile;
  if ( fileName.isEmpty() ) {
    QgsSettings settings;
    QString lastUsedDir = settings.value ( "UI/lastProjectDir", QDir::homePath() ).toString();
    fileName = QFileDialog::getOpenFileName (
                 mMainWindow, tr ( "Choose a KADAS Project" ), lastUsedDir, tr ( "QGIS files" ) + " (*.qgs *.qgz)"
               );
    if ( fileName.isEmpty() ) {
      return false;
    }
    settings.setValue ( "UI/lastProjectDir", QFileInfo ( fileName ).absolutePath() );
  }

  projectClose();

  QApplication::setOverrideCursor ( Qt::WaitCursor );
  mMainWindow->mapCanvas()->freeze ( true );
  bool autoSetupOnFirstLayer = mLayerTreeCanvasBridge->autoSetupOnFirstLayer();
  mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer ( false );

  QgsProjectDirtyBlocker dirtyBlocker ( QgsProject::instance() );
  bool success = QgsProject::instance()->read ( fileName );

  if ( success ) {
    emit projectRead();
  }

  mLayerTreeCanvasBridge->setAutoSetupOnFirstLayer ( autoSetupOnFirstLayer );
  mMainWindow->mapCanvas()->freeze ( false );
  mMainWindow->mapCanvas()->refresh();
  QApplication::restoreOverrideCursor();

  if ( !success ) {
    QMessageBox::critical ( mMainWindow, tr ( "Unable to open project" ), QgsProject::instance()->error() );
  }

  return success;
}

void KadasApplication::projectClose()
{
  // TODO
//  mMainWindow->closeChildMapCanvases();

  // TODO
//  deleteLayoutDesigners();

  // ensure layout widgets are fully deleted
//  QgsApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );

  mMainWindow->mapCanvas()->clearExtentHistory();

  // Remove annotation items
  QGraphicsScene* scene = mMainWindow->mapCanvas()->scene();
  for ( QGraphicsItem* item : mMainWindow->mapCanvas()->items() ) {
    if ( dynamic_cast<QgsMapCanvasAnnotationItem*> ( item ) ) {
      scene->removeItem ( item );
      delete item;
    }
  }

  // clear out any stuff from project
  mMainWindow->mapCanvas()->freeze ( true );
  mMainWindow->mapCanvas()->setLayers ( QList<QgsMapLayer*>() );
  mMainWindow->mapCanvas()->clearCache();
  mMainWindow->mapCanvas()->freeze ( false );

  // Avoid unnecessary layer changed handling for each layer removed - instead,
  // defer the handling until we've removed all layers
  mBlockActiveLayerChanged = true;
  QgsProject::instance()->clear();
  mBlockActiveLayerChanged = false;

  onActiveLayerChanged ( currentLayer() );
}

bool KadasApplication::projectSaveDirty()
{
  if ( QgsProject::instance()->isDirty() ) {
    QMessageBox::StandardButton response = QMessageBox::question (
        mMainWindow, tr ( "Save Project" ), tr ( "Do you want to save the current project?" ),
        QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard );
    if ( response == QMessageBox::Save ) {
      return projectSave();
    } else if ( response == QMessageBox::Cancel ) {
      return false;
    }
  }
  return true;
}

bool KadasApplication::projectSave ( const QString& fileName, bool promptFileName )
{
  if ( ( QgsProject::instance()->fileName().isNull() && fileName.isEmpty() ) || promptFileName ) {
    QgsSettings settings;
    QString lastUsedDir = settings.value ( QStringLiteral ( "UI/lastProjectDir" ), QDir::homePath() ).toString();

    QString path = QFileDialog::getSaveFileName (
                     mMainWindow, tr ( "Choose a KADAS Project" ), lastUsedDir, tr ( "QGIS files" ) + " (*.qgs *.qgz)"
                   );
    if ( path.isEmpty() ) {
      return false;
    }

    QFileInfo fullPath ( path );

    if (
      fullPath.suffix().compare ( QLatin1String ( "qgz" ), Qt::CaseInsensitive ) != 0 ||
      fullPath.suffix().compare ( QLatin1String ( "qgs" ), Qt::CaseInsensitive ) != 0
    ) {
      path += ".qgz";
    }

    QgsProject::instance()->setFileName ( path );
  } else if ( !fileName.isEmpty() ) {
    QgsProject::instance()->setFileName ( fileName );
  }

  if ( QgsProject::instance()->write() ) {
    mMainWindow->messageBar()->pushMessage ( tr ( "Project saved" ), "", Qgis::Info, mMainWindow->messageTimeout() );
    mMainWindow->statusBar()->showMessage ( tr ( "Project saved to %1" ).arg ( QDir::toNativeSeparators ( QgsProject::instance()->fileName() ) ), 5000 );
  } else {
    QMessageBox::critical ( mMainWindow,
                            tr ( "Unable to save project %1" ).arg ( QDir::toNativeSeparators ( QgsProject::instance()->fileName() ) ),
                            QgsProject::instance()->error() );
    return false;
  }

  return true;
}

void KadasApplication::saveMapAsImage()
{
  QPair< QString, QString> fileAndFilter = QgsGuiUtils::getSaveAsImageName ( mMainWindow, tr ( "Choose an Image File" ) );
  if ( fileAndFilter.first.isEmpty() ) {
    return;
  }
  mMainWindow->mapCanvas()->saveAsImage ( fileAndFilter.first, nullptr, fileAndFilter.second );
  mMainWindow->messageBar()->pushMessage ( tr ( "Map image saved to %1" ).arg ( QFileInfo ( fileAndFilter.first ).fileName() ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
}

void KadasApplication::saveMapToClipboard()
{
  QImage image ( mMainWindow->mapCanvas()->size(), QImage::Format_ARGB32 );
  image.fill ( QColor ( 255, 255, 255, 1 ) );
  QPainter painter ( &image );
  mMainWindow->mapCanvas()->render ( &painter );
  QApplication::clipboard()->setImage ( image );
  mMainWindow->messageBar()->pushMessage ( tr ( "Map image saved to clipboard" ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
}

void KadasApplication::showLayerAttributeTable ( const QgsMapLayer* layer )
{
  // TODO
}

void KadasApplication::showLayerProperties ( const QgsMapLayer* layer )
{
  // TODO
}

void KadasApplication::showLayerInfo ( const QgsMapLayer* layer )
{
  // TODO
}

QgsMapLayer* KadasApplication::currentLayer() const
{
  return mMainWindow->layerTreeView()->currentLayer();
}

void KadasApplication::displayMessage ( const QString& message, Qgis::MessageLevel level )
{
  mMainWindow->messageBar()->pushMessage ( message, level, mMainWindow->messageTimeout() );
}

void KadasApplication::onActiveLayerChanged ( QgsMapLayer* layer )
{
  if ( mBlockActiveLayerChanged ) {
    return;
  }
  mMainWindow->mapCanvas()->setCurrentLayer ( layer );
  emit activeLayerChanged ( layer );
}

void KadasApplication::onFocusChanged ( QWidget* /*old*/, QWidget* now )
{
  // If nothing has focus, ensure map canvas receives it
  if ( !now ) {
    mMainWindow->mapCanvas()->setFocus();
  }
}

void KadasApplication::onMapToolChanged ( QgsMapTool* newTool, QgsMapTool* oldTool )
{
  if ( oldTool ) {
    disconnect ( oldTool, &QgsMapTool::messageEmitted, this, &KadasApplication::displayMessage );
//    disconnect( oldTool, SIGNAL( messageDiscarded() ), this, SLOT( removeMapToolMessage() ) );
    if ( dynamic_cast<KadasMapToolPan*> ( oldTool ) ) {
      disconnect ( static_cast<KadasMapToolPan*> ( oldTool ), &KadasMapToolPan::itemPicked, this, &KadasApplication::handleItemPicked );
      disconnect ( static_cast<KadasMapToolPan*> ( oldTool ), &KadasMapToolPan::contextMenuRequested, this, &KadasApplication::showCanvasContextMenu );
    }
  }
  // Automatically return to pan tool if no tool is active
  if ( !newTool ) {
    mMainWindow->mapCanvas()->setMapTool ( mMapToolPan );
    return;
  }

  if ( newTool ) {
    connect ( newTool, &QgsMapTool::messageEmitted, this, &KadasApplication::displayMessage );
    if ( dynamic_cast<KadasMapToolPan*> ( newTool ) ) {
      connect ( static_cast<KadasMapToolPan*> ( newTool ), &KadasMapToolPan::itemPicked, this, &KadasApplication::handleItemPicked );
      connect ( static_cast<KadasMapToolPan*> ( newTool ), &KadasMapToolPan::contextMenuRequested, this, &KadasApplication::showCanvasContextMenu );
    }
  }
}

void KadasApplication::handleItemPicked ( const KadasFeaturePicker::PickResult& result )
{
  if ( qobject_cast<KadasItemLayer*> ( result.layer ) ) {
    KadasItemLayer* layer = static_cast<KadasItemLayer*> ( result.layer );
    QgsMapTool* tool = new KadasMapToolEditItem ( mMainWindow->mapCanvas(), result.itemId, layer );
    connect ( tool, &QgsMapTool::deactivated, tool, &QObject::deleteLater );
    mMainWindow->mapCanvas()->setMapTool ( tool );
  }
  // TODO
}

void KadasApplication::showCanvasContextMenu ( const QPoint& screenPos, const QgsPointXY& mapPos )
{
  // TODO
}

void KadasApplication::updateWindowTitle()
{
  QString fileName = QFileInfo ( QgsProject::instance()->fileName() ).baseName();
  if ( fileName.isEmpty() ) {
    fileName = tr ( "<New Project>" );
  }
  QString modified;
  if ( QgsProject::instance()->isDirty() ) {
    modified = " *";
  }
  QString title = QString ( "%1%2 - %3" ).arg ( fileName, modified, Kadas::KADAS_FULL_RELEASE_NAME );
  mMainWindow->setWindowTitle ( title );
}

QList<QgsMapLayer*> KadasApplication::showGDALSublayerSelectionDialog ( QgsRasterLayer* layer ) const
{
  QList<QgsMapLayer*> result;
  QgsSettings settings;

  QStringList sublayers = layer->subLayers();

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog ( QgsSublayersDialog::Gdal, QStringLiteral ( "gdal" ), mMainWindow );
  chooseSublayersDialog.setShowAddToGroupCheckbox ( true );

  QgsSublayersDialog::LayerDefinitionList layers;
  QStringList names;
  names.reserve ( sublayers.size() );
  layers.reserve ( sublayers.size() );
  for ( int i = 0; i < sublayers.size(); i++ ) {
    // simplify raster sublayer name - should add a function in gdal provider for this?
    // code is copied from QgsGdalLayerItem::createChildren
    QString name = sublayers[i];
    QString path = layer->source();
    // if netcdf/hdf use all text after filename
    // for hdf4 it would be best to get description, because the subdataset_index is not very practical
    if ( name.startsWith ( QLatin1String ( "netcdf" ), Qt::CaseInsensitive ) ||
         name.startsWith ( QLatin1String ( "hdf" ), Qt::CaseInsensitive ) ) {
      name = name.mid ( name.indexOf ( path ) + path.length() + 1 );
    } else {
      // remove driver name and file name
      name.remove ( name.split ( QgsDataProvider::SUBLAYER_SEPARATOR ) [0] );
      name.remove ( path );
    }
    // remove any : or " left over
    if ( name.startsWith ( ':' ) ) {
      name.remove ( 0, 1 );
    }

    if ( name.startsWith ( '\"' ) ) {
      name.remove ( 0, 1 );
    }

    if ( name.endsWith ( ':' ) ) {
      name.chop ( 1 );
    }

    if ( name.endsWith ( '\"' ) ) {
      name.chop ( 1 );
    }

    names << name;

    QgsSublayersDialog::LayerDefinition def;
    def.layerId = i;
    def.layerName = name;
    layers << def;
  }

  chooseSublayersDialog.populateLayerTable ( layers );

  if ( chooseSublayersDialog.exec() ) {
    // create more informative layer names, containing filename as well as sublayer name
    QRegExp rx ( "\"(.*)\"" );
    QString uri, name;

    const auto constSelection = chooseSublayersDialog.selection();
    for ( const QgsSublayersDialog::LayerDefinition& def : constSelection ) {
      int i = def.layerId;
      if ( rx.indexIn ( sublayers[i] ) != -1 ) {
        uri = rx.cap ( 1 );
        name = sublayers[i];
        name.replace ( uri, QFileInfo ( uri ).completeBaseName() );
      } else {
        name = names[i];
      }

      QgsRasterLayer* rlayer = new QgsRasterLayer ( sublayers[i], name );
      if ( rlayer && rlayer->isValid() ) {
        result << rlayer;
      }
    }
  }
  return result;
}

QList<QgsMapLayer*> KadasApplication::showOGRSublayerSelectionDialog ( QgsVectorLayer* layer ) const
{
  QList<QgsMapLayer*> result;
  QStringList sublayers = layer->dataProvider()->subLayers();

  QgsSublayersDialog::LayerDefinitionList list;
  QMap< QString, int > mapLayerNameToCount;
  bool uniqueNames = true;
  int lastLayerId = -1;
  const auto constSublayers = sublayers;
  for ( const QString& sublayer : constSublayers ) {
    // OGR provider returns items in this format:
    // <layer_index>:<name>:<feature_count>:<geom_type>

    QStringList elements = splitSubLayerDef ( sublayer );
    if ( elements.count() >= 4 ) {
      QgsSublayersDialog::LayerDefinition def;
      def.layerId = elements[0].toInt();
      def.layerName = elements[1];
      def.count = elements[2].toInt();
      def.type = elements[3];
      if ( lastLayerId != def.layerId ) {
        int count = ++mapLayerNameToCount[def.layerName];
        if ( count > 1 || def.layerName.isEmpty() ) {
          uniqueNames = false;
        }
        lastLayerId = def.layerId;
      }
      list << def;
    } else {
      QgsDebugMsg ( "Unexpected output from OGR provider's subLayers()! " + sublayer );
    }
  }

  // Check if the current layer uri contains the

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog ( QgsSublayersDialog::Ogr, QStringLiteral ( "ogr" ), mMainWindow );
  chooseSublayersDialog.setShowAddToGroupCheckbox ( true );
  chooseSublayersDialog.populateLayerTable ( list );

  if ( !chooseSublayersDialog.exec() ) {
    return result;
  }

  QString name = layer->name();

  auto uriParts = QgsProviderRegistry::instance()->decodeUri (
                    layer->providerType(), layer->dataProvider()->dataSourceUri() );
  QString uri ( uriParts.value ( QStringLiteral ( "path" ) ).toString() );

  // The uri must contain the actual uri of the vectorLayer from which we are
  // going to load the sublayers.
  QString fileName = QFileInfo ( uri ).baseName();
  const auto constSelection = chooseSublayersDialog.selection();
  for ( const QgsSublayersDialog::LayerDefinition& def : constSelection ) {
    QString layerGeometryType = def.type;
    QString composedURI = uri;
    if ( uniqueNames ) {
      composedURI += "|layername=" + def.layerName;
    } else {
      // Only use layerId if there are ambiguities with names
      composedURI += "|layerid=" + QString::number ( def.layerId );
    }

    if ( !layerGeometryType.isEmpty() ) {
      composedURI += "|geometrytype=" + layerGeometryType;
    }

    QgsDebugMsg ( "Creating new vector layer using " + composedURI );
    QString name = fileName + " " + def.layerName;
    QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
    options.loadDefaultStyle = false;
    QgsVectorLayer* layer = new QgsVectorLayer ( composedURI, name, QStringLiteral ( "ogr" ), options );
    if ( layer && layer->isValid() ) {
      result << layer;
    } else {
      QString msg = tr ( "%1 is not a valid or recognized data source" ).arg ( composedURI );
      mMainWindow->messageBar()->pushMessage ( tr ( "Invalid Data Source" ), msg, Qgis::Critical, mMainWindow->messageTimeout() );
      delete layer;
    }
  }
  return result;
}

bool KadasApplication::showZipSublayerSelectionDialog ( const QString& path ) const
{
  QVector<QgsDataItem*> childItems;
  QgsSettings settings;

  QgsDebugMsg ( "askUserForZipItemLayers( " + path + ')' );

  // if scanZipBrowser == no: skip to the next file
  if ( settings.value ( QStringLiteral ( "qgis/scanZipInBrowser2" ), "basic" ).toString() == QLatin1String ( "no" ) ) {
    return false;
  }

  QgsZipItem zipItem ( nullptr, path, path );
  zipItem.populate ( true );
  QgsDebugMsg ( QStringLiteral ( "Path= %1 got zipitem with %2 children" ).arg ( path ).arg ( zipItem.rowCount() ) );

  // if 1 or 0 child found, exit so a normal item is created by gdal or ogr provider
  if ( zipItem.rowCount() <= 1 ) {
    return false;
  }

  // We initialize a selection dialog and display it.
  QgsSublayersDialog chooseSublayersDialog ( QgsSublayersDialog::Vsifile, QStringLiteral ( "vsi" ), mMainWindow );
  QgsSublayersDialog::LayerDefinitionList layers;

  for ( int i = 0; i < zipItem.children().size(); i++ ) {
    QgsDataItem* item = zipItem.children().at ( i );
    QgsLayerItem* layerItem = qobject_cast<QgsLayerItem*> ( item );
    if ( !layerItem ) {
      continue;
    }

    QgsDebugMsgLevel ( QStringLiteral ( "item path=%1 provider=%2" ).arg ( item->path(), layerItem->providerKey() ), 2 );

    QgsSublayersDialog::LayerDefinition def;
    def.layerId = i;
    def.layerName = item->name();
    if ( layerItem->providerKey() == QLatin1String ( "gdal" ) ) {
      def.type = tr ( "Raster" );
    } else if ( layerItem->providerKey() == QLatin1String ( "ogr" ) ) {
      def.type = tr ( "Vector" );
    }
    layers << def;
  }

  chooseSublayersDialog.populateLayerTable ( layers );

  if ( chooseSublayersDialog.exec() ) {
    const auto constSelection = chooseSublayersDialog.selection();
    for ( const QgsSublayersDialog::LayerDefinition& def : constSelection ) {
      childItems << zipItem.children().at ( def.layerId );
    }
  }

  // add childItems
  const auto constChildItems = childItems;
  for ( QgsDataItem* item : constChildItems ) {
    QgsLayerItem* layerItem = qobject_cast<QgsLayerItem*> ( item );
    if ( !layerItem ) {
      continue;
    }

    QgsDebugMsg ( QStringLiteral ( "item path=%1 provider=%2" ).arg ( item->path(), layerItem->providerKey() ) );
    if ( layerItem->providerKey() == QLatin1String ( "gdal" ) ) {
      addRasterLayer ( item->path(), QFileInfo ( item->name() ).completeBaseName(), QString() );
    } else if ( layerItem->providerKey() == QLatin1String ( "ogr" ) ) {
      addVectorLayers ( QStringList ( item->path() ), QStringLiteral ( "System" ), QStringLiteral ( "file" ) );
    }
  }

  return true;
}
