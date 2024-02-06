/***************************************************************************
    kadasapplayerhandling.cpp (derived from qgisapplayerhandling.cpp)
    -------------------------
    begin                : July 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QUrlQuery>
#include <gdal.h>

#include <qgis/qgsauthmanager.h>
#include <qgis/qgsmeshlayer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsprojecttimesettings.h>
#include <qgis/qgspointcloudlayer.h>
#include <qgis/qgsmeshlayertemporalproperties.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproviderutils.h>
#include <qgis/qgszipitem.h>
#include <qgis/qgsproviderregistry.h>
#include <qgis/qgsprovidersublayerdetails.h>
#include <qgis/qgslayertreenode.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgsgui.h>
#include <qgis/qgsmbtiles.h>
#include <qgis/qgsmessagelog.h>
#include <qgis/qgsvectortilelayer.h>
#include <qgis/qgsprojectstorageregistry.h>
#include <qgis/qgsprojectstorage.h>
#include <qgis/qgsmaplayerfactory.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsauthguiutils.h>
#include <qgis/qgslayerdefinition.h>
#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>
#include <qgis/qgsprovidersublayersdialog.h>
#include <qgis/qgsmessagebaritem.h>
#include <qgis/qgsdockwidget.h>
#include <qgis/qgseditorwidgetregistry.h>
#include <qgis/qgsweakrelation.h>
#include <qgis/qgsfieldformatterregistry.h>
#include <qgis/qgsmaplayerutils.h>
#include <qgis/qgsfieldformatter.h>
#include <qgis/qgsabstractdatabaseproviderconnection.h>

#include <qgis/qgsprovidersublayersdialog.h>

#include <kadas/app/kadasapplayerhandling.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>

int gCanvasFreezeCount = 0;

class KadasCanvasRefreshBlocker
{
  public:
    KadasCanvasRefreshBlocker()
    {
      ++gCanvasFreezeCount;
      kApp->mainWindow()->mapCanvas()->freeze();
    }
    ~KadasCanvasRefreshBlocker()
    {
      release();
    }
    KadasCanvasRefreshBlocker( const KadasCanvasRefreshBlocker &other ) = delete;
    KadasCanvasRefreshBlocker &operator=( const KadasCanvasRefreshBlocker &other ) = delete;
    void release()
    {
      if ( !mReleased )
      {
        mReleased = true;
        if ( --gCanvasFreezeCount == 0 )
        {
          kApp->mainWindow()->mapCanvas()->freeze( false );
          kApp->mainWindow()->mapCanvas()->refresh();
        }
      }
    }
  private:
    bool mReleased = false;
};

void KadasAppLayerHandling::postProcessAddedLayer( QgsMapLayer *layer )
{
  switch ( layer->type() )
  {
    case Qgis::LayerType::Vector:
    case Qgis::LayerType::Raster:
    {
      bool ok = false;
      layer->loadDefaultStyle( ok );
      layer->loadDefaultMetadata( ok );
      break;
    }

    case Qgis::LayerType::Plugin:
      break;

    case Qgis::LayerType::Mesh:
    {
      QgsMeshLayer *meshLayer = qobject_cast< QgsMeshLayer *>( layer );
      QDateTime referenceTime = QgsProject::instance()->timeSettings()->temporalRange().begin();
      if ( !referenceTime.isValid() ) // If project reference time is invalid, use current date
        referenceTime = QDateTime( QDate::currentDate(), QTime( 0, 0, 0 ), Qt::UTC );

      if ( meshLayer->dataProvider() && !qobject_cast< QgsMeshLayerTemporalProperties * >( meshLayer->temporalProperties() )->referenceTime().isValid() )
        qobject_cast< QgsMeshLayerTemporalProperties * >( meshLayer->temporalProperties() )->setReferenceTime( referenceTime, meshLayer->dataProvider()->temporalCapabilities() );

      bool ok = false;
      meshLayer->loadDefaultStyle( ok );
      meshLayer->loadDefaultMetadata( ok );
      break;
    }

    case Qgis::LayerType::VectorTile:
    {
      bool ok = false;
      QString error = layer->loadDefaultStyle( ok );
      if ( !ok )
        kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Error loading style" ), error, Qgis::MessageLevel::Warning );
      error = layer->loadDefaultMetadata( ok );
      if ( !ok )
        kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Error loading layer metadata" ), error, Qgis::MessageLevel::Warning );

      break;
    }

    case Qgis::LayerType::Annotation:
    case Qgis::LayerType::Group:
      break;

    case Qgis::LayerType::PointCloud:
    {
      bool ok = false;
      layer->loadDefaultStyle( ok );
      layer->loadDefaultMetadata( ok );

      break;
    }
  }
}

void KadasAppLayerHandling::postProcessAddedLayers( const QList<QgsMapLayer *> &layers )
{
  for ( QgsMapLayer *layer : layers )
  {
    switch ( layer->type() )
    {
      case Qgis::LayerType::Vector:
      {
        QgsVectorLayer *vl = qobject_cast< QgsVectorLayer * >( layer );

        // try to automatically load related tables for OGR layers
        if ( vl->providerType() == QLatin1String( "ogr" ) )
        {
          // first need to create weak relations!!
          std::unique_ptr< QgsAbstractDatabaseProviderConnection > conn { QgsMapLayerUtils::databaseConnection( vl ) };
          if ( conn && ( conn->capabilities() & QgsAbstractDatabaseProviderConnection::Capability::RetrieveRelationships ) )
          {
            const QVariantMap uriParts = QgsProviderRegistry::instance()->decodeUri( layer->providerType(), layer->source() );
            const QString layerName = uriParts.value( QStringLiteral( "layerName" ) ).toString();
            if ( layerName.isEmpty() )
              continue;

            const QList< QgsWeakRelation > relations = conn->relationships( QString(), layerName );
            if ( !relations.isEmpty() )
            {
              vl->setWeakRelations( relations );
              resolveVectorLayerDependencies( vl, QgsMapLayer::StyleCategory::Relations, QgsVectorLayerRef::MatchType::Source, DependencyFlag::LoadAllRelationships | DependencyFlag::SilentLoad );
              resolveVectorLayerWeakRelations( vl, QgsVectorLayerRef::MatchType::Source, true );
            }
          }
        }
        break;
      }
      case Qgis::LayerType::Raster:
      case Qgis::LayerType::Plugin:
      case Qgis::LayerType::Mesh:
      case Qgis::LayerType::VectorTile:
      case Qgis::LayerType::Annotation:
      case Qgis::LayerType::PointCloud:
      case Qgis::LayerType::Group:
        break;
    }
  }
}

QList< QgsMapLayer * > KadasAppLayerHandling::addOgrVectorLayers( const QStringList &layers, const QString &encoding, const QString &dataSourceType, bool &ok, bool showWarningOnInvalid )
{
  //note: this method ONLY supports vector layers from the OGR provider!
  ok = false;

  KadasCanvasRefreshBlocker refreshBlocker;

  QList<QgsMapLayer *> layersToAdd;
  QList<QgsMapLayer *> addedLayers;
  QgsSettings settings;
  bool userAskedToAddLayers = false;

  for ( const QString &layerUri : layers )
  {
    const QString uri = layerUri.trimmed();
    QString baseName;
    if ( dataSourceType == QLatin1String( "file" ) )
    {
      QString srcWithoutLayername( uri );
      int posPipe = srcWithoutLayername.indexOf( '|' );
      if ( posPipe >= 0 )
        srcWithoutLayername.resize( posPipe );
      baseName = QgsProviderUtils::suggestLayerNameFromFilePath( srcWithoutLayername );

      // if needed prompt for zipitem layers
      QString vsiPrefix = QgsZipItem::vsiPrefix( uri );
      if ( ! uri.startsWith( QLatin1String( "/vsi" ), Qt::CaseInsensitive ) &&
           ( vsiPrefix == QLatin1String( "/vsizip/" ) || vsiPrefix == QLatin1String( "/vsitar/" ) ) )
      {
        if ( askUserForZipItemLayers( uri, { Qgis::LayerType::Vector } ) )
          continue;
      }
    }
    else if ( dataSourceType == QLatin1String( "database" ) )
    {
      // Try to extract the database name and use it as base name
      // sublayers names (if any) will be appended to the layer name
      const QVariantMap parts( QgsProviderRegistry::instance()->decodeUri( QStringLiteral( "ogr" ), uri ) );
      if ( parts.value( QStringLiteral( "databaseName" ) ).isValid() )
        baseName = parts.value( QStringLiteral( "databaseName" ) ).toString();
      else
        baseName = uri;
    }
    else //directory //protocol
    {
      baseName = QgsProviderUtils::suggestLayerNameFromFilePath( uri );
    }

    if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
    {
      baseName = QgsMapLayer::formatLayerName( baseName );
    }

    QgsDebugMsgLevel( "completeBaseName: " + baseName, 2 );
    const bool isVsiCurl { uri.startsWith( QLatin1String( "/vsicurl" ), Qt::CaseInsensitive ) };
    const auto scheme { QUrl( uri ).scheme() };
    const bool isRemoteUrl { scheme.startsWith( QLatin1String( "http" ) ) || scheme == QLatin1String( "ftp" ) };

    std::unique_ptr< QgsTemporaryCursorOverride > cursorOverride;
    if ( isVsiCurl || isRemoteUrl )
    {
      cursorOverride = std::make_unique< QgsTemporaryCursorOverride >( Qt::WaitCursor );
      kApp->mainWindow()->messageBar()->pushInfo( QObject::tr( "Remote layer" ), QObject::tr( "loading %1, please wait …" ).arg( uri ) );
      qApp->processEvents();
    }

    QList< QgsProviderSublayerDetails > sublayers = QgsProviderRegistry::instance()->providerMetadata( QStringLiteral( "ogr" ) )->querySublayers( uri, Qgis::SublayerQueryFlag::IncludeSystemTables );
    // filter out non-vector sublayers
    sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), []( const QgsProviderSublayerDetails & sublayer )
    {
      return sublayer.type() != Qgis::LayerType::Vector;
    } ), sublayers.end() );

    cursorOverride.reset();

    const QVariantMap uriParts = QgsProviderRegistry::instance()->decodeUri( QStringLiteral( "ogr" ), uri );
    const QString path = uriParts.value( QStringLiteral( "path" ) ).toString();

    if ( !sublayers.empty() )
    {
      userAskedToAddLayers = true;

      const bool detailsAreIncomplete = QgsProviderUtils::sublayerDetailsAreIncomplete( sublayers, QgsProviderUtils::SublayerCompletenessFlag::IgnoreUnknownFeatureCount );
      const bool singleSublayerOnly = sublayers.size() == 1;
      QString groupName;

      if ( !singleSublayerOnly || detailsAreIncomplete )
      {
        // ask user for sublayers (unless user settings dictate otherwise!)
        switch ( shouldAskUserForSublayers( sublayers ) )
        {
          case SublayerHandling::AskUser:
          {
            // prompt user for sublayers
            QgsProviderSublayersDialog dlg( uri, QStringLiteral( "ogr" ), path, sublayers, {Qgis::LayerType::Vector}, kApp->mainWindow() );

            if ( dlg.exec() )
              sublayers = dlg.selectedLayers();
            else
              sublayers.clear(); // dialog was canceled, so don't add any sublayers
            groupName = dlg.groupName();
            break;
          }

          case SublayerHandling::LoadAll:
          {
            if ( detailsAreIncomplete )
            {
              // requery sublayers, resolving geometry types
              sublayers = QgsProviderRegistry::instance()->querySublayers( uri, Qgis::SublayerQueryFlag::ResolveGeometryType );
              // filter out non-vector sublayers
              sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), []( const QgsProviderSublayerDetails & sublayer )
              {
                return sublayer.type() != Qgis::LayerType::Vector;
              } ), sublayers.end() );
            }
            break;
          }

          case SublayerHandling::AbortLoading:
            sublayers.clear(); // don't add any sublayers
            break;
        };
      }
      else if ( detailsAreIncomplete )
      {
        // requery sublayers, resolving geometry types
        sublayers = QgsProviderRegistry::instance()->querySublayers( uri, Qgis::SublayerQueryFlag::ResolveGeometryType );
        // filter out non-vector sublayers
        sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), []( const QgsProviderSublayerDetails & sublayer )
        {
          return sublayer.type() != Qgis::LayerType::Vector;
        } ), sublayers.end() );
      }

      // now add sublayers
      if ( !sublayers.empty() )
      {
        addedLayers << addSublayers( sublayers, baseName, groupName );
      }

    }
    else
    {
      QString msg = QObject::tr( "%1 is not a valid or recognized data source." ).arg( uri );
      // If the failed layer was a vsicurl type, give the user a chance to try the normal download.
      if ( isVsiCurl &&
           QMessageBox::question( kApp->mainWindow(), QObject::tr( "Invalid Data Source" ),
                                  QObject::tr( "Download with \"Protocol\" source type has failed, do you want to try the \"File\" source type?" ) ) == QMessageBox::Yes )
      {
        QString fileUri = uri;
        fileUri.replace( QLatin1String( "/vsicurl/" ), " " );
        return addOgrVectorLayers( QStringList() << fileUri, encoding, dataSourceType, showWarningOnInvalid );
      }
      else if ( showWarningOnInvalid )
      {
        kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
      }
    }
  }

  // make sure at least one layer was successfully added
  if ( layersToAdd.isEmpty() )
  {
    // we also return true if we asked the user for sublayers, but they choose none. In this case nothing
    // went wrong, so we shouldn't return false and cause GUI warnings to appear
    ok = userAskedToAddLayers || !addedLayers.isEmpty();
  }

  // Register this layer with the layers registry
  QgsProject::instance()->addMapLayers( layersToAdd );
  for ( QgsMapLayer *l : std::as_const( layersToAdd ) )
  {
    if ( !encoding.isEmpty() )
    {
      if ( QgsVectorLayer *vl = qobject_cast< QgsVectorLayer * >( l ) )
        vl->setProviderEncoding( encoding );
    }

    kApp->askUserForDatumTransform( l->crs(), QgsProject::instance()->crs(), l );
    KadasAppLayerHandling::postProcessAddedLayer( l );
  }
//  QgisApp::instance()->activateDeactivateLayerRelatedActions( QgisApp::instance()->activeLayer() );

  ok = true;
  addedLayers.append( layersToAdd );
  return addedLayers;
}

QgsPointCloudLayer *KadasAppLayerHandling::addPointCloudLayer( const QString &uri, const QString &baseName, const QString &provider, bool showWarningOnInvalid )
{
  KadasCanvasRefreshBlocker refreshBlocker;
  QgsSettings settings;

  QString base( baseName );

  if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
  {
    base = QgsMapLayer::formatLayerName( base );
  }

  QgsDebugMsgLevel( "completeBaseName: " + base, 2 );

  // create the layer
  std::unique_ptr<QgsPointCloudLayer> layer( new QgsPointCloudLayer( uri, base, provider ) );

  if ( !layer || !layer->isValid() )
  {
    if ( showWarningOnInvalid )
    {
      QString msg = QObject::tr( "%1 is not a valid or recognized data source, error: \"%2\"" ).arg( uri, layer->error().message( QgsErrorMessage::Format::Text ) );
      kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
    }

    // since the layer is bad, stomp on it
    return nullptr;
  }

  KadasAppLayerHandling::postProcessAddedLayer( layer.get() );


  QgsProject::instance()->addMapLayer( layer.get() );
//  QgisApp::instance()->activateDeactivateLayerRelatedActions( QgisApp::instance()->activeLayer() );

  return layer.release();
}

QgsPluginLayer *KadasAppLayerHandling::addPluginLayer( const QString &uri, const QString &baseName, const QString &provider )
{
  QgsPluginLayer *layer = QgsApplication::pluginLayerRegistry()->createLayer( provider, uri );
  if ( !layer )
    return nullptr;

  layer->setName( baseName );

  QgsProject::instance()->addMapLayer( layer );

  return layer;
}

QgsVectorTileLayer *KadasAppLayerHandling::addVectorTileLayer( const QString &uri, const QString &baseName, bool showWarningOnInvalid )
{
  KadasCanvasRefreshBlocker refreshBlocker;
  QgsSettings settings;

  QString base( baseName );

  if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
  {
    base = QgsMapLayer::formatLayerName( base );
  }

  QgsDebugMsgLevel( "completeBaseName: " + base, 2 );

  // create the layer
  const QgsVectorTileLayer::LayerOptions options( QgsProject::instance()->transformContext() );
  std::unique_ptr<QgsVectorTileLayer> layer( new QgsVectorTileLayer( uri, base, options ) );

  if ( !layer || !layer->isValid() )
  {
    if ( showWarningOnInvalid )
    {
      QString msg = QObject::tr( "%1 is not a valid or recognized data source." ).arg( uri );
      kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
    }

    // since the layer is bad, stomp on it
    return nullptr;
  }

  KadasAppLayerHandling::postProcessAddedLayer( layer.get() );

  QgsProject::instance()->addMapLayer( layer.get() );
//  QgisApp::instance()->activateDeactivateLayerRelatedActions( QgisApp::instance()->activeLayer() );

  return layer.release();
}

bool KadasAppLayerHandling::askUserForZipItemLayers( const QString &path, const QList<Qgis::LayerType> &acceptableTypes )
{
  // query sublayers
  QList< QgsProviderSublayerDetails > sublayers = QgsProviderRegistry::instance()->querySublayers( path, Qgis::SublayerQueryFlag::IncludeSystemTables );

  // filter out non-matching sublayers
  sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), [acceptableTypes]( const QgsProviderSublayerDetails & sublayer )
  {
    return !acceptableTypes.empty() && !acceptableTypes.contains( sublayer.type() );
  } ), sublayers.end() );

  if ( sublayers.empty() )
    return false;

  const bool detailsAreIncomplete = QgsProviderUtils::sublayerDetailsAreIncomplete( sublayers, QgsProviderUtils::SublayerCompletenessFlag::IgnoreUnknownFeatureCount );
  const bool singleSublayerOnly = sublayers.size() == 1;
  QString groupName;

  if ( !singleSublayerOnly || detailsAreIncomplete )
  {
    // ask user for sublayers (unless user settings dictate otherwise!)
    switch ( shouldAskUserForSublayers( sublayers ) )
    {
      case SublayerHandling::AskUser:
      {
        // prompt user for sublayers
        QgsProviderSublayersDialog dlg( path, QString(), path, sublayers, acceptableTypes, kApp->mainWindow() );

        if ( dlg.exec() )
          sublayers = dlg.selectedLayers();
        else
          sublayers.clear(); // dialog was canceled, so don't add any sublayers
        groupName = dlg.groupName();
        break;
      }

      case SublayerHandling::LoadAll:
      {
        if ( detailsAreIncomplete )
        {
          // requery sublayers, resolving geometry types
          sublayers = QgsProviderRegistry::instance()->querySublayers( path, Qgis::SublayerQueryFlag::ResolveGeometryType );
          sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), [acceptableTypes]( const QgsProviderSublayerDetails & sublayer )
          {
            return !acceptableTypes.empty() && !acceptableTypes.contains( sublayer.type() );
          } ), sublayers.end() );
        }
        break;
      }

      case SublayerHandling::AbortLoading:
        sublayers.clear(); // don't add any sublayers
        break;
    };
  }
  else if ( detailsAreIncomplete )
  {
    // requery sublayers, resolving geometry types
    sublayers = QgsProviderRegistry::instance()->querySublayers( path, Qgis::SublayerQueryFlag::ResolveGeometryType );
    sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), [acceptableTypes]( const QgsProviderSublayerDetails & sublayer )
    {
      return !acceptableTypes.empty() && !acceptableTypes.contains( sublayer.type() );
    } ), sublayers.end() );
  }

  // now add sublayers
  if ( !sublayers.empty() )
  {
    KadasCanvasRefreshBlocker refreshBlocker;
    QgsSettings settings;

    QString base = QgsProviderUtils::suggestLayerNameFromFilePath( path );
    if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
    {
      base = QgsMapLayer::formatLayerName( base );
    }

    addSublayers( sublayers, base, groupName );
//    QgisApp::instance()->activateDeactivateLayerRelatedActions( QgisApp::instance()->activeLayer() );
  }

  return true;
}

KadasAppLayerHandling::SublayerHandling KadasAppLayerHandling::shouldAskUserForSublayers( const QList<QgsProviderSublayerDetails> &layers, bool hasNonLayerItems )
{
  if ( hasNonLayerItems )
    return SublayerHandling::AskUser;

  QgsSettings settings;
  const Qgis::SublayerPromptMode promptLayers = settings.enumValue( QStringLiteral( "qgis/promptForSublayers" ), Qgis::SublayerPromptMode::AlwaysAsk );

  switch ( promptLayers )
  {
    case Qgis::SublayerPromptMode::AlwaysAsk:
      return SublayerHandling::AskUser;

    case Qgis::SublayerPromptMode::AskExcludingRasterBands:
    {
      // if any non-raster layers are found, we ask the user. Otherwise we load all
      for ( const QgsProviderSublayerDetails &sublayer : layers )
      {
        if ( sublayer.type() != Qgis::LayerType::Raster )
          return SublayerHandling::AskUser;
      }
      return SublayerHandling::LoadAll;
    }

    case Qgis::SublayerPromptMode::NeverAskSkip:
      return SublayerHandling::AbortLoading;

    case Qgis::SublayerPromptMode::NeverAskLoadAll:
      return SublayerHandling::LoadAll;
  }

  return SublayerHandling::AskUser;
}

QList<QgsMapLayer *> KadasAppLayerHandling::addSublayers( const QList<QgsProviderSublayerDetails> &layers, const QString &baseName, const QString &groupName )
{
  QgsLayerTreeGroup *group = nullptr;
  if ( !groupName.isEmpty() )
  {
    int index { 0 };
    QgsLayerTreeNode *currentNode { kApp->mainWindow()->layerTreeView()->currentNode() };
    if ( currentNode && currentNode->parent() )
    {
      if ( QgsLayerTree::isGroup( currentNode ) )
      {
        group = qobject_cast<QgsLayerTreeGroup *>( currentNode )->insertGroup( 0, groupName );
      }
      else if ( QgsLayerTree::isLayer( currentNode ) )
      {
        const QList<QgsLayerTreeNode *> currentNodeSiblings { currentNode->parent()->children() };
        int nodeIdx { 0 };
        for ( const QgsLayerTreeNode *child : std::as_const( currentNodeSiblings ) )
        {
          nodeIdx++;
          if ( child == currentNode )
          {
            index = nodeIdx;
            break;
          }
        }
        group = qobject_cast<QgsLayerTreeGroup *>( currentNode->parent() )->insertGroup( index, groupName );
      }
      else
      {
        group = QgsProject::instance()->layerTreeRoot()->insertGroup( 0, groupName );
      }
    }
    else
    {
      group = QgsProject::instance()->layerTreeRoot()->insertGroup( 0, groupName );
    }
  }

  QgsSettings settings;
  const bool formatLayerNames = settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool();

  // if we aren't adding to a group, we need to add the layers in reverse order so that they maintain the correct
  // order in the layer tree!
  QList<QgsProviderSublayerDetails> sortedLayers = layers;
  if ( groupName.isEmpty() )
  {
    std::reverse( sortedLayers.begin(), sortedLayers.end() );
  }

  QList< QgsMapLayer * > result;
  result.reserve( sortedLayers.size() );

  for ( const QgsProviderSublayerDetails &sublayer : std::as_const( sortedLayers ) )
  {
    QgsProviderSublayerDetails::LayerOptions options( QgsProject::instance()->transformContext() );
    options.loadDefaultStyle = false;

    std::unique_ptr<QgsMapLayer> layer( sublayer.toLayer( options ) );
    if ( !layer )
      continue;

    QgsMapLayer *ml = layer.get();
    // if we aren't adding to a group, then we're iterating the layers in the reverse order
    // so account for that in the returned list of layers
    if ( groupName.isEmpty() )
      result.insert( 0, ml );
    else
      result << ml;

    QString layerName = layer->name();
    if ( formatLayerNames )
    {
      layerName = QgsMapLayer::formatLayerName( layerName );
    }

    const bool projectWasEmpty = QgsProject::instance()->mapLayers().empty();

    // if user has opted to add sublayers to a group, then we don't need to include the
    // filename in the layer's name, because the group is already titled with the filename.
    // But otherwise, we DO include the file name so that users can differentiate the source
    // when multiple layers are loaded from a GPX file or similar (refs https://github.com/qgis/QGIS/issues/37551)
    if ( group )
    {
      if ( !layerName.isEmpty() )
        layer->setName( layerName );
      else if ( !baseName.isEmpty() )
        layer->setName( baseName );
      QgsProject::instance()->addMapLayer( layer.release(), false );
      group->addLayer( ml );
    }
    else
    {
      if ( layerName != baseName && !layerName.isEmpty() && !baseName.isEmpty() )
        layer->setName( QStringLiteral( "%1 — %2" ).arg( baseName, layerName ) );
      else if ( !layerName.isEmpty() )
        layer->setName( layerName );
      else if ( !baseName.isEmpty() )
        layer->setName( baseName );
      QgsProject::instance()->addMapLayer( layer.release() );
    }

    // Some of the logic relating to matching a new project's CRS to the first layer added CRS is deferred to happen when the event loop
    // next runs -- so in those cases we can't assume that the project's CRS has been matched to the actual desired CRS yet.
    // In these cases we don't need to show the coordinate operation selection choice, so just hardcode an exception in here to avoid that...
    QgsCoordinateReferenceSystem projectCrsAfterLayerAdd = QgsProject::instance()->crs();
    const QgsGui::ProjectCrsBehavior projectCrsBehavior = QgsSettings().enumValue( QStringLiteral( "/projections/newProjectCrsBehavior" ),  QgsGui::UseCrsOfFirstLayerAdded, QgsSettings::App );
    switch ( projectCrsBehavior )
    {
      case QgsGui::UseCrsOfFirstLayerAdded:
      {
        if ( projectWasEmpty )
          projectCrsAfterLayerAdd = ml->crs();
        break;
      }

      case QgsGui::UsePresetCrs:
        break;
    }

    kApp->askUserForDatumTransform( ml->crs(), projectCrsAfterLayerAdd, ml );
  }

  if ( group )
  {
    // Respect if user don't want the new group of layers visible.
    QgsSettings settings;
    const bool newLayersVisible = settings.value( QStringLiteral( "/qgis/new_layers_visible" ), true ).toBool();
    if ( !newLayersVisible )
      group->setItemVisibilityCheckedRecursive( newLayersVisible );
  }

  // Post process all added layers
  for ( QgsMapLayer *ml : std::as_const( result ) )
  {
    KadasAppLayerHandling::postProcessAddedLayer( ml );
  }

  return result;
}

QList< QgsMapLayer * > KadasAppLayerHandling::openLayer( const QString &fileName, bool &ok, bool allowInteractive, bool suppressBulkLayerPostProcessing )
{
  QList< QgsMapLayer * > openedLayers;
  auto postProcessAddedLayers = [suppressBulkLayerPostProcessing, &openedLayers]
  {
    if ( !suppressBulkLayerPostProcessing )
      KadasAppLayerHandling::postProcessAddedLayers( openedLayers );
  };

  ok = false;
  const QFileInfo fileInfo( fileName );

  // highest priority = delegate to provider registry to handle
  const QList< QgsProviderRegistry::ProviderCandidateDetails > candidateProviders = QgsProviderRegistry::instance()->preferredProvidersForUri( fileName );
  if ( candidateProviders.size() == 1 && candidateProviders.at( 0 ).layerTypes().size() == 1 )
  {
    // one good candidate provider and possible layer type -- that makes things nice and easy!
    switch ( candidateProviders.at( 0 ).layerTypes().at( 0 ) )
    {
      case Qgis::LayerType::Vector:
      case Qgis::LayerType::Raster:
      case Qgis::LayerType::Mesh:
      case Qgis::LayerType::Annotation:
      case Qgis::LayerType::Plugin:
      case Qgis::LayerType::VectorTile:
      case Qgis::LayerType::Group:
        // not supported here yet!
        break;

      case Qgis::LayerType::PointCloud:
      {
        if ( QgsPointCloudLayer *layer = addPointCloudLayer( fileName, fileInfo.completeBaseName(), candidateProviders.at( 0 ).metadata()->key(), true ) )
        {
          ok = true;
          openedLayers << layer;
        }
        else
        {
          // The layer could not be loaded and the reason has been reported by the provider, we can exit now
          return {};
        }
        break;
      }
    }
  }

  if ( ok )
  {
    postProcessAddedLayers();
    return openedLayers;
  }

  CPLPushErrorHandler( CPLQuietErrorHandler );

  // if needed prompt for zipitem layers
  QString vsiPrefix = QgsZipItem::vsiPrefix( fileName );
  if ( vsiPrefix == QLatin1String( "/vsizip/" ) || vsiPrefix == QLatin1String( "/vsitar/" ) )
  {
    if ( askUserForZipItemLayers( fileName, {} ) )
    {
      CPLPopErrorHandler();
      ok = true;
      return openedLayers;
    }
  }

  if ( fileName.endsWith( QStringLiteral( ".mbtiles" ), Qt::CaseInsensitive ) )
  {
    QgsMbTiles reader( fileName );
    if ( reader.open() )
    {
      if ( reader.metadataValue( "format" ) == QLatin1String( "pbf" ) )
      {
        // these are vector tiles
        QUrlQuery uq;
        uq.addQueryItem( QStringLiteral( "type" ), QStringLiteral( "mbtiles" ) );
        uq.addQueryItem( QStringLiteral( "url" ), fileName );
        const QgsVectorTileLayer::LayerOptions options( QgsProject::instance()->transformContext() );
        std::unique_ptr<QgsVectorTileLayer> vtLayer( new QgsVectorTileLayer( uq.toString(), fileInfo.completeBaseName(), options ) );
        if ( vtLayer->isValid() )
        {
          openedLayers << vtLayer.get();
          QgsProject::instance()->addMapLayer( vtLayer.release() );
          postProcessAddedLayers();
          ok = true;
          return openedLayers;
        }
      }
      else // raster tiles
      {
        // prefer to use WMS provider's implementation to open MBTiles rasters
        QUrlQuery uq;
        uq.addQueryItem( QStringLiteral( "type" ), QStringLiteral( "mbtiles" ) );
        uq.addQueryItem( QStringLiteral( "url" ), QUrl::fromLocalFile( fileName ).toString() );
        if ( QgsRasterLayer *rasterLayer = addRasterLayer( uq.toString(), fileInfo.completeBaseName(), QStringLiteral( "wms" ) ) )
        {
          openedLayers << rasterLayer;
          postProcessAddedLayers();
          ok = true;
          return openedLayers;
        }
      }
    }
  }
  else if ( fileName.endsWith( QStringLiteral( ".vtpk" ), Qt::CaseInsensitive ) )
  {
    // these are vector tiles
    QUrlQuery uq;
    uq.addQueryItem( QStringLiteral( "type" ), QStringLiteral( "vtpk" ) );
    uq.addQueryItem( QStringLiteral( "url" ), fileName );
    const QgsVectorTileLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    std::unique_ptr<QgsVectorTileLayer> vtLayer( new QgsVectorTileLayer( uq.toString(), fileInfo.completeBaseName(), options ) );
    if ( vtLayer->isValid() )
    {
      openedLayers << vtLayer.get();
      KadasAppLayerHandling::postProcessAddedLayer( vtLayer.get() );
      QgsProject::instance()->addMapLayer( vtLayer.release() );
      postProcessAddedLayers();
      ok = true;
      return openedLayers;
    }
  }

  QList< QgsProviderSublayerModel::NonLayerItem > nonLayerItems;
  if ( QgsProjectStorage *ps = QgsApplication::projectStorageRegistry()->projectStorageFromUri( fileName ) )
  {
    const QStringList projects = ps->listProjects( fileName );
    for ( const QString &project : projects )
    {
      QgsProviderSublayerModel::NonLayerItem projectItem;
      projectItem.setType( QStringLiteral( "project" ) );
      projectItem.setName( project );
      projectItem.setUri( QStringLiteral( "%1://%2?projectName=%3" ).arg( ps->type(), fileName, project ) );
      projectItem.setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mIconQgsProjectFile.svg" ) ) );
      nonLayerItems << projectItem;
    }
  }

  // query sublayers
  QList< QgsProviderSublayerDetails > sublayers = QgsProviderRegistry::instance()->querySublayers( fileName, Qgis::SublayerQueryFlag::IncludeSystemTables );

  if ( !sublayers.empty() || !nonLayerItems.empty() )
  {
    const bool detailsAreIncomplete = QgsProviderUtils::sublayerDetailsAreIncomplete( sublayers, QgsProviderUtils::SublayerCompletenessFlag::IgnoreUnknownFeatureCount );
    const bool singleSublayerOnly = sublayers.size() == 1;
    QString groupName;

    if ( allowInteractive && ( !singleSublayerOnly || detailsAreIncomplete || !nonLayerItems.empty() ) )
    {
      // ask user for sublayers (unless user settings dictate otherwise!)
      switch ( shouldAskUserForSublayers( sublayers, !nonLayerItems.empty() ) )
      {
        case SublayerHandling::AskUser:
        {
          // prompt user for sublayers
          QgsProviderSublayersDialog dlg( fileName, QString(), fileName, sublayers, {}, kApp->mainWindow() );
          dlg.setNonLayerItems( nonLayerItems );

          if ( dlg.exec() )
          {
            sublayers = dlg.selectedLayers();
            nonLayerItems = dlg.selectedNonLayerItems();
          }
          else
          {
            sublayers.clear(); // dialog was canceled, so don't add any sublayers
            nonLayerItems.clear();
          }
          groupName = dlg.groupName();
          break;
        }

        case SublayerHandling::LoadAll:
        {
          if ( detailsAreIncomplete )
          {
            // requery sublayers, resolving geometry types
            sublayers = QgsProviderRegistry::instance()->querySublayers( fileName, Qgis::SublayerQueryFlag::ResolveGeometryType );
          }
          break;
        }

        case SublayerHandling::AbortLoading:
          sublayers.clear(); // don't add any sublayers
          break;
      };
    }
    else if ( detailsAreIncomplete )
    {
      // requery sublayers, resolving geometry types
      sublayers = QgsProviderRegistry::instance()->querySublayers( fileName, Qgis::SublayerQueryFlag::ResolveGeometryType );
    }

    ok = true;

    // now add sublayers
    if ( !sublayers.empty() )
    {
      KadasCanvasRefreshBlocker refreshBlocker;
      QgsSettings settings;

      QString base = QgsProviderUtils::suggestLayerNameFromFilePath( fileName );
      if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
      {
        base = QgsMapLayer::formatLayerName( base );
      }

      openedLayers.append( addSublayers( sublayers, base, groupName ) );
//      KadasAppLayerHandling()->activateDeactivateLayerRelatedActions( KadasAppLayerHandling()->activeLayer() );
    }
    else if ( !nonLayerItems.empty() )
    {
      ok = true;
      KadasCanvasRefreshBlocker refreshBlocker;
      if ( kApp->checkTasksDependOnProject() )
        return {};

      // possibly save any pending work before opening a different project
      if ( kApp->projectSaveDirty() )
      {
        // error handling and reporting is in addProject() function
        kApp->projectOpen( nonLayerItems.at( 0 ).uri() );
      }
      return {};
    }
  }

  CPLPopErrorHandler();

  if ( !ok )
  {
    // maybe a known file type, which couldn't be opened due to a missing dependency... (eg. las for a non-pdal-enabled build)
    QgsProviderRegistry::UnusableUriDetails details;
    if ( QgsProviderRegistry::instance()->handleUnusableUri( fileName, details ) )
    {
      ok = true;

      if ( details.detailedWarning.isEmpty() )
        kApp->mainWindow()->messageBar()->pushMessage( QString(), details.warning, Qgis::MessageLevel::Critical );
      else
        kApp->mainWindow()->messageBar()->pushMessage( QString(), details.warning, details.detailedWarning, Qgis::MessageLevel::Critical );
    }
  }

  if ( !ok )
  {
    // we have no idea what this file is...
    QgsMessageLog::logMessage( QObject::tr( "Unable to load %1" ).arg( fileName ) );

    const QString msg = QObject::tr( "%1 is not a valid or recognized data source." ).arg( fileName );
    kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
  }
  else
  {
    postProcessAddedLayers();
  }

  return openedLayers;
}

QgsVectorLayer *KadasAppLayerHandling::addVectorLayer( const QString &uri, const QString &baseName, const QString &provider, bool showWarningOnInvalid )
{
  return addLayerPrivate< QgsVectorLayer >( Qgis::LayerType::Vector, uri, baseName, !provider.isEmpty() ? provider : QLatin1String( "ogr" ), showWarningOnInvalid );
}

QgsRasterLayer *KadasAppLayerHandling::addRasterLayer( const QString &uri, const QString &baseName, const QString &provider, bool showWarningOnInvalid )
{
  return addLayerPrivate< QgsRasterLayer >( Qgis::LayerType::Raster, uri, baseName, !provider.isEmpty() ? provider : QLatin1String( "gdal" ), showWarningOnInvalid );
}

QgsMeshLayer *KadasAppLayerHandling::addMeshLayer( const QString &uri, const QString &baseName, const QString &provider )
{
  return addLayerPrivate< QgsMeshLayer >( Qgis::LayerType::Mesh, uri, baseName, provider, true );
}

QList<QgsMapLayer *> KadasAppLayerHandling::addGdalRasterLayers( const QStringList &uris, bool &ok, bool showWarningOnInvalid )
{
  ok = false;
  if ( uris.empty() )
  {
    return {};
  }

  KadasCanvasRefreshBlocker refreshBlocker;

  // this is messy since some files in the list may be rasters and others may
  // be ogr layers. We'll set returnValue to false if one or more layers fail
  // to load.

  QList< QgsMapLayer * > res;

  for ( const QString &uri : uris )
  {
    QString errMsg;

    // if needed prompt for zipitem layers
    QString vsiPrefix = QgsZipItem::vsiPrefix( uri );
    if ( ( !uri.startsWith( QLatin1String( "/vsi" ), Qt::CaseInsensitive ) || uri.endsWith( QLatin1String( ".zip" ) ) || uri.endsWith( QLatin1String( ".tar" ) ) ) &&
         ( vsiPrefix == QLatin1String( "/vsizip/" ) || vsiPrefix == QLatin1String( "/vsitar/" ) ) )
    {
      if ( askUserForZipItemLayers( uri, { Qgis::LayerType::Raster } ) )
        continue;
    }

    const bool isVsiCurl { uri.startsWith( QLatin1String( "/vsicurl" ), Qt::CaseInsensitive ) };
    const bool isRemoteUrl { uri.startsWith( QLatin1String( "http" ) ) || uri == QLatin1String( "ftp" ) };

    std::unique_ptr< QgsTemporaryCursorOverride > cursorOverride;
    if ( isVsiCurl || isRemoteUrl )
    {
      cursorOverride = std::make_unique< QgsTemporaryCursorOverride >( Qt::WaitCursor );
      kApp->mainWindow()->messageBar()->pushInfo( QObject::tr( "Remote layer" ), QObject::tr( "loading %1, please wait …" ).arg( uri ) );
      qApp->processEvents();
    }

    if ( QgsRasterLayer::isValidRasterFileName( uri, errMsg ) )
    {
      QFileInfo myFileInfo( uri );

      // set the layer name to the file base name unless provided explicitly
      QString layerName;
      const QVariantMap uriDetails = QgsProviderRegistry::instance()->decodeUri( QStringLiteral( "gdal" ), uri );
      if ( !uriDetails[ QStringLiteral( "layerName" ) ].toString().isEmpty() )
      {
        layerName = uriDetails[ QStringLiteral( "layerName" ) ].toString();
      }
      else
      {
        layerName = QgsProviderUtils::suggestLayerNameFromFilePath( uri );
      }

      // try to create the layer
      cursorOverride.reset();
      QgsRasterLayer *layer = addLayerPrivate< QgsRasterLayer >( Qgis::LayerType::Raster, uri, layerName, QStringLiteral( "gdal" ), showWarningOnInvalid );
      res << layer;

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
      ok = false;

      // Issue message box warning unless we are loading from cmd line since
      // non-rasters are passed to this function first and then successfully
      // loaded afterwards (see main.cpp)
      if ( showWarningOnInvalid )
      {
        QString msg = QObject::tr( "%1 is not a supported raster data source" ).arg( uri );
        if ( !errMsg.isEmpty() )
          msg += '\n' + errMsg;

        kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Unsupported Data Source" ), msg, Qgis::MessageLevel::Critical );
      }
    }
  }
  return res;
}

void KadasAppLayerHandling::addMapLayer( QgsMapLayer *mapLayer )
{
  KadasCanvasRefreshBlocker refreshBlocker;

  if ( mapLayer->isValid() )
  {
    // Register this layer with the layers registry
    QList<QgsMapLayer *> myList;
    myList << mapLayer;
    QgsProject::instance()->addMapLayers( myList );

    kApp->askUserForDatumTransform( mapLayer->crs(), QgsProject::instance()->crs(), mapLayer );
  }
  else
  {
    QString msg = QObject::tr( "The layer is not a valid layer and can not be added to the map" );
    kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Layer is not valid" ), msg, Qgis::MessageLevel::Critical );
  }
}

void KadasAppLayerHandling::openLayerDefinition( const QString &filename )
{
  QString errorMessage;
  QgsReadWriteContext context;
  bool loaded = false;

  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    errorMessage = QStringLiteral( "Can not open file" );
  }
  else
  {
    QDomDocument doc;
    QString message;
    if ( !doc.setContent( &file, &message ) )
    {
      errorMessage = message;
    }
    else
    {
      QFileInfo fileinfo( file );
      QDir::setCurrent( fileinfo.absoluteDir().path() );

      context.setPathResolver( QgsPathResolver( filename ) );
      context.setProjectTranslator( QgsProject::instance() );

      loaded = QgsLayerDefinition::loadLayerDefinition( doc, QgsProject::instance(), QgsProject::instance()->layerTreeRoot(), errorMessage, context );
    }
  }

  if ( loaded )
  {
    const QList< QgsReadWriteContext::ReadWriteMessage > messages = context.takeMessages();
    QVector< QgsReadWriteContext::ReadWriteMessage > shownMessages;
    for ( const QgsReadWriteContext::ReadWriteMessage &message : messages )
    {
      if ( shownMessages.contains( message ) )
        continue;

      kApp->mainWindow()->messageBar()->pushMessage( QString(), message.message(), message.categories().join( '\n' ), message.level() );

      shownMessages.append( message );
    }
  }
  else if ( !loaded || !errorMessage.isEmpty() )
  {
    kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Error loading layer definition" ), errorMessage, Qgis::MessageLevel::Warning );
  }
}

void KadasAppLayerHandling::addLayerDefinition()
{
  QgsSettings settings;
  QString lastUsedDir = settings.value( QStringLiteral( "UI/lastQLRDir" ), QDir::homePath() ).toString();

  QString path = QFileDialog::getOpenFileName( kApp->mainWindow(), QStringLiteral( "Add Layer Definition File" ), lastUsedDir, QStringLiteral( "*.qlr" ) );
  if ( path.isEmpty() )
    return;

  QFileInfo fi( path );
  settings.setValue( QStringLiteral( "UI/lastQLRDir" ), fi.path() );

  openLayerDefinition( path );
}

QList< QgsMapLayer * > KadasAppLayerHandling::addDatabaseLayers( const QStringList &layerPathList, const QString &providerKey, bool &ok )
{
  ok = false;
  QList<QgsMapLayer *> myList;

  if ( layerPathList.empty() )
  {
    // no layers to add so bail out, but
    // allow mMapCanvas to handle events
    // first
    return {};
  }

  KadasCanvasRefreshBlocker refreshBlocker;

  QApplication::setOverrideCursor( Qt::WaitCursor );

  const auto constLayerPathList = layerPathList;
  for ( const QString &layerPath : constLayerPathList )
  {
    // create the layer
    QgsDataSourceUri uri( layerPath );

    QgsVectorLayer::LayerOptions options { QgsProject::instance()->transformContext() };
    options.loadDefaultStyle = false;
    QgsVectorLayer *layer = new QgsVectorLayer( uri.uri( false ), uri.table(), providerKey, options );
    Q_CHECK_PTR( layer );

    if ( ! layer )
    {
      QApplication::restoreOverrideCursor();

      // XXX insert meaningful whine to the user here
      return {};
    }

    if ( layer->isValid() )
    {
      // add to list of layers to register
      //with the central layers registry
      myList << layer;
    }
    else
    {
      QgsMessageLog::logMessage( QObject::tr( "%1 is an invalid layer - not loaded" ).arg( layerPath ) );
      QLabel *msgLabel = new QLabel( QObject::tr( "%1 is an invalid layer and cannot be loaded." ).arg( layerPath ), kApp->mainWindow()->messageBar() );
      msgLabel->setWordWrap( true );
      QgsMessageBarItem *item = new QgsMessageBarItem( msgLabel, Qgis::MessageLevel::Warning );
      kApp->mainWindow()->messageBar()->pushItem( item );
      delete layer;
    }
    //qWarning("incrementing iterator");
  }

  QgsProject::instance()->addMapLayers( myList );

  // load default style after adding to process readCustomSymbology signals
  const auto constMyList = myList;
  for ( QgsMapLayer *l : constMyList )
  {
    bool ok;
    l->loadDefaultStyle( ok );
    l->loadDefaultMetadata( ok );
  }

  QApplication::restoreOverrideCursor();

  ok = true;
  return myList;
}

template<typename T>
T *KadasAppLayerHandling::addLayerPrivate( Qgis::LayerType type, const QString &uri, const QString &name, const QString &providerKey, bool guiWarnings )
{
  QgsSettings settings;

  KadasCanvasRefreshBlocker refreshBlocker;

  QString baseName = settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() ? QgsMapLayer::formatLayerName( name ) : name;

  // if the layer needs authentication, ensure the master password is set
  const thread_local QRegularExpression rx( "authcfg=([a-z]|[A-Z]|[0-9]){7}" );
  if ( rx.match( uri ).hasMatch() )
  {
    if ( !QgsAuthGuiUtils::isDisabled( kApp->mainWindow()->messageBar() ) )
    {
      QgsApplication::authManager()->setMasterPassword( true );
    }
  }

  QVariantMap uriElements = QgsProviderRegistry::instance()->decodeUri( providerKey, uri );
  QString path = uri;
  if ( uriElements.contains( QStringLiteral( "path" ) ) )
  {
    // run layer path through QgsPathResolver so that all inbuilt paths and other localised paths are correctly expanded
    path = QgsPathResolver().readPath( uriElements.value( QStringLiteral( "path" ) ).toString() );
    uriElements[ QStringLiteral( "path" ) ] = path;
  }
  // Not all providers implement decodeUri(), so use original uri if uriElements is empty
  const QString updatedUri = uriElements.isEmpty() ? uri : QgsProviderRegistry::instance()->encodeUri( providerKey, uriElements );

  const bool canQuerySublayers = QgsProviderRegistry::instance()->providerMetadata( providerKey ) &&
                                 ( QgsProviderRegistry::instance()->providerMetadata( providerKey )->capabilities() & QgsProviderMetadata::QuerySublayers );

  T *result = nullptr;
  if ( canQuerySublayers )
  {
    // query sublayers
    QList< QgsProviderSublayerDetails > sublayers = QgsProviderRegistry::instance()->providerMetadata( providerKey ) ?
        QgsProviderRegistry::instance()->providerMetadata( providerKey )->querySublayers( updatedUri, Qgis::SublayerQueryFlag::IncludeSystemTables )
        : QgsProviderRegistry::instance()->querySublayers( updatedUri );

    // filter out non-matching sublayers
    sublayers.erase( std::remove_if( sublayers.begin(), sublayers.end(), [type]( const QgsProviderSublayerDetails & sublayer )
    {
      return sublayer.type() != type;
    } ), sublayers.end() );

    if ( sublayers.empty() )
    {
      if ( guiWarnings )
      {
        QString msg = QObject::tr( "%1 is not a valid or recognized data source." ).arg( uri );
        kApp->mainWindow()->messageBar()->pushMessage( QObject::tr( "Invalid Data Source" ), msg, Qgis::MessageLevel::Critical );
      }

      // since the layer is bad, stomp on it
      return nullptr;
    }
    else if ( sublayers.size() > 1 || QgsProviderUtils::sublayerDetailsAreIncomplete( sublayers, QgsProviderUtils::SublayerCompletenessFlag::IgnoreUnknownFeatureCount ) )
    {
      // ask user for sublayers (unless user settings dictate otherwise!)
      switch ( shouldAskUserForSublayers( sublayers ) )
      {
        case SublayerHandling::AskUser:
        {
          QgsProviderSublayersDialog dlg( updatedUri, QString(), path, sublayers, {type}, kApp->mainWindow() );
          if ( dlg.exec() )
          {
            const QList< QgsProviderSublayerDetails > selectedLayers = dlg.selectedLayers();
            if ( !selectedLayers.isEmpty() )
            {
              result = qobject_cast< T * >( addSublayers( selectedLayers, baseName, dlg.groupName() ).value( 0 ) );
            }
          }
          break;
        }
        case SublayerHandling::LoadAll:
        {
          result = qobject_cast< T * >( addSublayers( sublayers, baseName, QString() ).value( 0 ) );
          break;
        }
        case SublayerHandling::AbortLoading:
          break;
      };
    }
    else
    {
      result = qobject_cast< T * >( addSublayers( sublayers, name, QString() ).value( 0 ) );

      if ( result )
      {
        QString base( baseName );
        if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
        {
          base = QgsMapLayer::formatLayerName( base );
        }
        result->setName( base );
      }
    }
  }
  else
  {
    QgsMapLayerFactory::LayerOptions options( QgsProject::instance()->transformContext() );
    options.loadDefaultStyle = false;
    result = qobject_cast< T * >( QgsMapLayerFactory::createLayer( uri, name, type, options, providerKey ) );
    if ( result )
    {
      QString base( baseName );
      if ( settings.value( QStringLiteral( "qgis/formatLayerName" ), false ).toBool() )
      {
        base = QgsMapLayer::formatLayerName( base );
      }
      result->setName( base );
      QgsProject::instance()->addMapLayer( result );

      kApp->askUserForDatumTransform( result->crs(), QgsProject::instance()->crs(), result );
      KadasAppLayerHandling::postProcessAddedLayer( result );
    }
  }

//  QgisApp::instance()->activateDeactivateLayerRelatedActions( QgisApp::instance()->activeLayer() );
  return result;
}

const QList<QgsVectorLayerRef> KadasAppLayerHandling::findBrokenLayerDependencies( QgsVectorLayer *vl, QgsMapLayer::StyleCategories categories, QgsVectorLayerRef::MatchType matchType, DependencyFlags dependencyFlags )
{
  QList<QgsVectorLayerRef> brokenDependencies;

  if ( categories.testFlag( QgsMapLayer::StyleCategory::Forms ) )
  {
    for ( int i = 0; i < vl->fields().count(); i++ )
    {
      const QgsEditorWidgetSetup setup = QgsGui::editorWidgetRegistry()->findBest( vl, vl->fields().field( i ).name() );
      QgsFieldFormatter *fieldFormatter = QgsApplication::fieldFormatterRegistry()->fieldFormatter( setup.type() );
      if ( fieldFormatter )
      {
        const QList<QgsVectorLayerRef> constDependencies { fieldFormatter->layerDependencies( setup.config() ) };
        for ( const QgsVectorLayerRef &dependency : constDependencies )
        {
          // I guess we need and isNull()/isValid() method for the ref
          if ( dependency.layer ||
               ! dependency.name.isEmpty() ||
               ! dependency.source.isEmpty() ||
               ! dependency.layerId.isEmpty() )
          {
            const QgsVectorLayer *depVl { QgsVectorLayerRef( dependency ).resolveWeakly( QgsProject::instance(), matchType ) };
            if ( ! depVl || ! depVl->isValid() )
            {
              brokenDependencies.append( dependency );
            }
          }
        }
      }
    }
  }

  if ( categories.testFlag( QgsMapLayer::StyleCategory::Relations ) )
  {
    // Check for layer weak relations
    const QList<QgsWeakRelation> weakRelations { vl->weakRelations() };
    for ( const QgsWeakRelation &weakRelation : weakRelations )
    {
      QList< QgsVectorLayerRef > dependencies;

      if ( !( dependencyFlags & DependencyFlag::LoadAllRelationships ) )
      {
        // This is the big question: do we really
        // want to automatically load the referencing layer(s) too?
        // This could potentially lead to a cascaded load of a
        // long list of layers.

        // for now, unless we are forcing load of all relationships we only consider relationships
        // where the referencing layer is a match.
        if ( weakRelation.referencingLayer().resolveWeakly( QgsProject::instance(), matchType ) != vl )
        {
          continue;
        }
      }

      switch ( weakRelation.cardinality() )
      {
        case Qgis::RelationshipCardinality::ManyToMany:
        {
          if ( !weakRelation.mappingTable().resolveWeakly( QgsProject::instance(), matchType ) )
            dependencies << weakRelation.mappingTable();
          [[fallthrough]];
        }

        case Qgis::RelationshipCardinality::OneToOne:
        case Qgis::RelationshipCardinality::OneToMany:
        case Qgis::RelationshipCardinality::ManyToOne:
        {
          if ( !weakRelation.referencedLayer().resolveWeakly( QgsProject::instance(), matchType ) )
            dependencies << weakRelation.referencedLayer();

          if ( !weakRelation.referencingLayer().resolveWeakly( QgsProject::instance(), matchType ) )
            dependencies << weakRelation.referencingLayer();

          break;
        }
      }

      for ( const QgsVectorLayerRef &dependency : std::as_const( dependencies ) )
      {
        // Make sure we don't add it twice if it was already added by the form widgets check
        bool refFound = false;
        for ( const QgsVectorLayerRef &otherRef : std::as_const( brokenDependencies ) )
        {
          if ( ( !dependency.layerId.isEmpty() && dependency.layerId == otherRef.layerId )
               || ( dependency.source == otherRef.source && dependency.provider == otherRef.provider ) )
          {
            refFound = true;
            break;
          }
        }
        if ( ! refFound )
        {
          brokenDependencies.append( dependency );
        }
      }
    }
  }
  return brokenDependencies;
}

void KadasAppLayerHandling::resolveVectorLayerDependencies( QgsVectorLayer *vl, QgsMapLayer::StyleCategories categories, QgsVectorLayerRef::MatchType matchType, DependencyFlags dependencyFlags )
{
  if ( vl && vl->isValid() )
  {
    const QList<QgsVectorLayerRef> dependencies { findBrokenLayerDependencies( vl, categories, matchType, dependencyFlags ) };
    for ( const QgsVectorLayerRef &dependency : dependencies )
    {
      // Check for projects without layer dependencies (see 7e8c7b3d0e094737336ff4834ea2af625d2921bf)
      if ( QgsProject::instance()->mapLayer( dependency.layerId ) || ( dependency.name.isEmpty() && dependency.source.isEmpty() ) )
      {
        continue;
      }
      // try to aggressively resolve the broken dependencies
      QgsVectorLayer *loadedLayer = nullptr;
      const QString providerName { vl->dataProvider()->name() };
      QgsProviderMetadata *providerMetadata { QgsProviderRegistry::instance()->providerMetadata( providerName ) };
      if ( providerMetadata )
      {
        // Retrieve the DB connection (if any)

        std::unique_ptr< QgsAbstractDatabaseProviderConnection > conn { QgsMapLayerUtils::databaseConnection( vl ) };
        if ( conn )
        {
          QString tableSchema;
          QString tableName;
          const QVariantMap sourceParts = providerMetadata->decodeUri( dependency.source );

          // This part should really be abstracted out to the connection classes or to the providers directly.
          // Different providers decode the uri differently, for example we don't get the table name out of OGR
          // but the layerName/layerId instead, so let's try different approaches

          // This works for GPKG
          tableName = sourceParts.value( QStringLiteral( "layerName" ) ).toString();

          // This works for PG and spatialite
          if ( tableName.isEmpty() )
          {
            tableName = sourceParts.value( QStringLiteral( "table" ) ).toString();
            tableSchema = sourceParts.value( QStringLiteral( "schema" ) ).toString();
          }

          // Helper to find layers in connections
          auto layerFinder = [ &conn, &dependency, &providerName ]( const QString & tableSchema, const QString & tableName ) -> QgsVectorLayer *
          {
            // First try the current schema (or no schema if it's not supported from the provider)
            try
            {
              const QString layerUri { conn->tableUri( tableSchema, tableName )};
              // Aggressive doesn't mean stupid: check if a layer with the same URI
              // was already loaded, this catches a corner case for renamed/moved GPKGS
              // where the dependency was actually loaded but it was found as broken
              // because the source does not match anymore (for instance when loaded
              // from a style definition).
              QStringList layerUris;
              for ( auto it = QgsProject::instance()->mapLayers().cbegin(); it != QgsProject::instance()->mapLayers().cend(); ++it )
              {
                if ( it.value()->publicSource() == layerUri )
                {
                  return nullptr;
                }
              }
              // Load it!
              std::unique_ptr< QgsVectorLayer > newVl = std::make_unique< QgsVectorLayer >( layerUri, !dependency.name.isEmpty() ? dependency.name : tableName, providerName );
              if ( newVl->isValid() )
              {
                QgsVectorLayer *res = newVl.get();
                QgsProject::instance()->addMapLayer( newVl.release() );
                return res;
              }
            }
            catch ( QgsProviderConnectionException & )
            {
              // Do nothing!
            }
            return nullptr;
          };

          loadedLayer = layerFinder( tableSchema, tableName );

          // Try different schemas
          if ( ! loadedLayer && conn->capabilities().testFlag( QgsAbstractDatabaseProviderConnection::Capability::Schemas ) && ! tableSchema.isEmpty() )
          {
            const QStringList schemas { conn->schemas() };
            for ( const QString &schemaName : schemas )
            {
              if ( schemaName != tableSchema )
              {
                loadedLayer = layerFinder( schemaName, tableName );
              }
              if ( loadedLayer )
              {
                break;
              }
            }
          }
        }
      }
      if ( ! loadedLayer )
      {
        const QString msg { QObject::tr( "layer '%1' requires layer '%2' to be loaded but '%2' could not be found, please load it manually if possible." ).arg( vl->name(), dependency.name ) };
        kApp->mainWindow()->messageBar()->pushWarning( QObject::tr( "Missing layer form dependency" ), msg );
      }
      else if ( !( dependencyFlags & DependencyFlag::SilentLoad ) )
      {
        kApp->mainWindow()->messageBar()->pushSuccess( QObject::tr( "Missing layer form dependency" ), QObject::tr( "Layer dependency '%2' required by '%1' was automatically loaded." )
            .arg( vl->name(),
                  loadedLayer->name() ) );
      }
    }
  }
}

void KadasAppLayerHandling::resolveVectorLayerWeakRelations( QgsVectorLayer *vectorLayer, QgsVectorLayerRef::MatchType matchType, bool guiWarnings )
{
  if ( vectorLayer && vectorLayer->isValid() )
  {
    const QList<QgsWeakRelation> constWeakRelations { vectorLayer->weakRelations( ) };
    for ( const QgsWeakRelation &rel : constWeakRelations )
    {
      const QList< QgsRelation > relations { rel.resolvedRelations( QgsProject::instance(), matchType ) };
      for ( const QgsRelation &relation : relations )
      {
        if ( relation.isValid() )
        {
          // Avoid duplicates
          const QList<QgsRelation> constRelations { QgsProject::instance()->relationManager()->relations().values() };
          for ( const QgsRelation &other : constRelations )
          {
            if ( relation.hasEqualDefinition( other ) )
            {
              continue;
            }
          }
          QgsProject::instance()->relationManager()->addRelation( relation );
        }
        else if ( guiWarnings )
        {
          kApp->mainWindow()->messageBar()->pushWarning( QObject::tr( "Invalid relationship %1" ).arg( relation.name() ), relation.validationError() );
        }
      }
    }
  }
}

void KadasAppLayerHandling::onVectorLayerStyleLoaded( QgsVectorLayer *vl, QgsMapLayer::StyleCategories categories )
{
  if ( vl && vl->isValid( ) )
  {

    // Check broken dependencies in forms
    if ( categories.testFlag( QgsMapLayer::StyleCategory::Forms ) )
    {
      resolveVectorLayerDependencies( vl );
    }

    // Check broken relations and try to restore them
    if ( categories.testFlag( QgsMapLayer::StyleCategory::Relations ) )
    {
      resolveVectorLayerWeakRelations( vl );
    }
  }
}

