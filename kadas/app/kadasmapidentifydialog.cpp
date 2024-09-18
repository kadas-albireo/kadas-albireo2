/***************************************************************************
    kadasmapidentifydialog.cpp
    --------------------------
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

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTreeWidget>
#include <QUrlQuery>
#include <QVBoxLayout>

#include <qgis/qgsarcgisrestquery.h>
#include <qgis/qgsarcgisrestutils.h>
#include <qgis/qgsfeaturestore.h>
#include <qgis/qgsgeometryrubberband.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasteridentifyresult.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include "kadas/gui/kadasmapcanvasitemmanager.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"
#include "kadasmapidentifydialog.h"


const int KadasMapIdentifyDialog::sGeometryRole = Qt::UserRole + 1;
const int KadasMapIdentifyDialog::sGeometryCrsRole = Qt::UserRole + 2;
QPointer<KadasMapIdentifyDialog> KadasMapIdentifyDialog::sInstance;

void KadasMapIdentifyDialog::popup( QgsMapCanvas *canvas, const QgsPointXY &mapPos )
{
  if ( !sInstance.isNull() )
  {
    sInstance->collectInfo( mapPos );
  }
  else
  {
    sInstance = new KadasMapIdentifyDialog( canvas, mapPos );
  }
  sInstance->show();
  sInstance->raise();
}

KadasMapIdentifyDialog::KadasMapIdentifyDialog( QgsMapCanvas *canvas, const QgsPointXY &mapPos )
  : mCanvas( canvas )
{
  setWindowTitle( tr( "Identify results" ) );
  setAttribute( Qt::WA_DeleteOnClose );
  setLayout( new QVBoxLayout() );
  resize( 480, 480 );

  mTreeWidget = new QTreeWidget( this );
  mTreeWidget->setColumnCount( 2 );
  mTreeWidget->header()->setStretchLastSection( true );
  mTreeWidget->header()->resizeSection( 0, 200 );
  mTreeWidget->setHeaderHidden( true );
  mTreeWidget->setDropIndicatorShown( false );
  mTreeWidget->setVerticalScrollMode( QTreeWidget::ScrollPerPixel );
  layout()->addWidget( mTreeWidget );
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  layout()->addWidget( bbox );

  connect( mTreeWidget, &QTreeWidget::itemClicked, this, &KadasMapIdentifyDialog::onItemClicked );

  collectInfo( mapPos );

  connect( QgsProject::instance(), &QgsProject::cleared, this, &KadasMapIdentifyDialog::clear );
  connect( QgsProject::instance(), &QgsProject::cleared, this, &QObject::deleteLater );
}

KadasMapIdentifyDialog::~KadasMapIdentifyDialog()
{
  clear();
}

void KadasMapIdentifyDialog::clear()
{
  delete mRubberband;
  mRubberband = nullptr;
  delete mResultPin.data();
  mResultPin.clear();
  delete mClickPosPin.data();
  mClickPosPin.clear();
  qDeleteAll( mGeometries );
  mGeometries.clear();
  if ( mRasterIdentifyReply )
  {
    mRasterIdentifyReply->abort();
    mRasterIdentifyReply->deleteLater();
    mRasterIdentifyReply = nullptr;
    mTimeoutTimer = nullptr;
  }
  mTreeWidget->clear();
  mLayerTreeItemMap.clear();
}

void KadasMapIdentifyDialog::onItemClicked( QTreeWidgetItem *item, int /*col*/ )
{
  delete mRubberband;
  mRubberband = nullptr;
  delete mResultPin.data();
  mResultPin.clear();

  while ( item && item->data( 0, sGeometryRole ).isNull() )
    item = item->parent();
  if ( !item )
    return;

  int idx = item->data( 0, sGeometryRole ).toInt();
  QgsAbstractGeometry *geom = mGeometries[idx];
  if ( !geom )
  {
    return;
  }
  QgsCoordinateReferenceSystem crs( item->data( 0, sGeometryCrsRole ).toString() );
  QgsCoordinateTransform crst( crs, mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  geom->transform( crst );
  item->setData( 0, sGeometryCrsRole, mCanvas->mapSettings().destinationCrs().authid() );

  if ( dynamic_cast<QgsPoint *>( geom ) )
  {
    QgsPoint *p = static_cast<QgsPoint *>( geom );
    mResultPin = new KadasPinItem( mCanvas->mapSettings().destinationCrs() );
    mResultPin->setPosition( KadasItemPos( p->x(), p->y() ) );
    mResultPin->setFilePath( ":/kadas/icons/pin_blue" );
  }
  else
  {
    mRubberband = new QgsGeometryRubberBand( mCanvas, QgsWkbTypes::geometryType( geom->wkbType() ) );
    mRubberband->setGeometry( geom->clone() );
    mRubberband->setIconType( QgsGeometryRubberBand::ICON_NONE );
    mRubberband->setFillColor( QColor( 255, 0, 0, 127 ) );
    mRubberband->setStrokeColor( QColor( 255, 0, 0 ) );
    mRubberband->setStrokeWidth( 2 );
  }
}

void KadasMapIdentifyDialog::collectInfo( const QgsPointXY &mapPos )
{
  clear();

  mClickPosPin = new KadasPinItem( mCanvas->mapSettings().destinationCrs() );
  mClickPosPin->setPosition( KadasItemPos( mapPos.x(), mapPos.y() ) );
  KadasMapCanvasItemManager::addItem( mClickPosPin );

  // Prepare for raster layers
  const QgsCoordinateReferenceSystem &mapCrs = mCanvas->mapSettings().destinationCrs();
  QgsCoordinateTransform crst( mapCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  QgsPointXY worldPos = crst.transform( mapPos );
  QgsRectangle worldExtent = crst.transformBoundingBox( mCanvas->extent() );
  QStringList rlayerIds;
  QMap<QString, QVariant> rlayerMap;

  // Prepare for vector layers
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( mCanvas->mapSettings() );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );

  for ( QgsMapLayer *layer : mCanvas->layers() )
  {
    if ( dynamic_cast<KadasPluginLayer *>( layer ) )
    {
      KadasPluginLayer *pluginLayer = static_cast<KadasPluginLayer *>( layer );
      QList<KadasPluginLayer::IdentifyResult> results = pluginLayer->identify( mapPos, mCanvas->mapSettings() );
      if ( !results.isEmpty() )
      {
        addPluginLayerResults( pluginLayer, results );
      }
    }
    else if ( dynamic_cast<QgsRasterLayer *>( layer ) )
    {
      QgsRasterLayer *rlayer = static_cast<QgsRasterLayer *>( layer );

#if 0
      // Detect ArcGIS MapServer Layers
      if ( rlayer->providerType() == "arcgismapserver" )
      {
        QgsDataSourceUri dataSource( rlayer->dataProvider()->dataSourceUri() );
        QStringList urlParts = dataSource.param( "url" ).split( "/", Qt::SkipEmptyParts );
        int nParts = urlParts.size();
        // Example: https://<...>/services/<group>/<service>/MapServer
        if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
        {
          rlayerIds.append( urlParts[nParts - 3] + ":" + urlParts[nParts - 2] );
          rlayerMap.insert( urlParts[nParts - 2], rlayer->id() );
        }
      }

      if ( rlayer->providerType() == "wms" )
      {
        QgsDataSourceUri dataSource;
        dataSource.setEncodedUri( rlayer->dataProvider()->dataSourceUri() );
        QStringList urlParts = dataSource.param( "url" ).split( "/", Qt::SkipEmptyParts );
        int nParts = urlParts.size();
        // Detect MapServer WMS Layers
        // Example: https://<...>/services/<group>/<service>/MapServer/WMSServer
        if ( nParts > 5 && urlParts[nParts - 1] == "WMSServer" && urlParts[nParts - 2] == "MapServer" && urlParts[nParts - 5] == "services" )
        {
          rlayerIds.append( urlParts[nParts - 4] + ":" + urlParts[nParts - 3] );
          rlayerMap.insert( urlParts[nParts - 3], rlayer->id() );
        }
        // Detect MapServer WMTS Layers
        // Example: https://<...>/rest/services/<group>/<service>/MapServer/WMTS/1.0.0/WMTSCapabilities.xml
        else if ( nParts > 8 && urlParts[nParts - 1] == "WMTSCapabilities.xml" && urlParts[nParts - 4] == "MapServer" && urlParts[nParts - 7] == "services" )
        {
          rlayerIds.append( urlParts[nParts - 6] + ":" + urlParts[nParts - 5] );
          rlayerMap.insert( urlParts[nParts - 5], rlayer->id() );
        }
      }
#else
      int capabilities = rlayer->dataProvider()->capabilities();
      Qgis::RasterIdentifyFormat format = Qgis::RasterIdentifyFormat::Undefined;
      if ( capabilities & QgsRasterInterface::IdentifyFeature ) format = Qgis::RasterIdentifyFormat::Feature;
      else if ( capabilities & QgsRasterInterface::IdentifyValue ) format = Qgis::RasterIdentifyFormat::Value;
      else if ( capabilities & QgsRasterInterface::IdentifyText ) format = Qgis::RasterIdentifyFormat::Text;
//        else if ( capabilities & QgsRasterInterface::IdentifyHtml ) format = Qgis::RasterIdentifyFormat::Html;

      if ( ( capabilities & QgsRasterDataProvider::Identify ) && format != Qgis::RasterIdentifyFormat::Undefined )
      {
        QgsCoordinateTransform crst( mCanvas->mapSettings().destinationCrs(), rlayer->crs(), QgsProject::instance()->transformContext() );
        QgsRasterIdentifyResult result = rlayer->dataProvider()->identify( crst.transform( mapPos ), format, crst.transformBoundingBox( mCanvas->extent() ), 0.25 * mCanvas->width(), 0.25 * mCanvas->height(), mCanvas->mapSettings().outputDpi() );
        addRasterIdentifyResult( rlayer, result );
      }
#endif
    }
    else if ( dynamic_cast<QgsVectorLayer *>( layer ) )
    {
      QgsVectorLayer *vlayer = static_cast<QgsVectorLayer *>( layer );
      if ( vlayer->hasScaleBasedVisibility() &&
           ( vlayer->maximumScale() > mCanvas->mapSettings().scale() ||
             vlayer->minimumScale() <= mCanvas->mapSettings().scale() ) )
      {
        continue;
      }

      QgsFeatureRenderer *renderer = vlayer->renderer();
      bool filteredRendering = false;
      if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
      {
        // setup scale for scale dependent visibility (rule based)
        renderer->startRender( renderContext, vlayer->fields() );
        filteredRendering = renderer->capabilities() & QgsFeatureRenderer::Filter;
      }

      QgsRectangle layerFilterRect = mCanvas->mapSettings().mapToLayerCoordinates( vlayer, filterRect );
#if _QGIS_VERSION_INT >= 33500
      QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( Qgis::FeatureRequestFlag::ExactIntersect ) );
#else
      QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
#endif
      QgsFeature feature;
      while ( fit.nextFeature( feature ) )
      {
        if ( filteredRendering && !renderer->willRenderFeature( feature, renderContext ) )
        {
          continue;
        }
        addVectorLayerResult( vlayer, feature );
      }
      if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
      {
        renderer->stopRender( renderContext );
      }
    }
  }

  // Raster layer query
  if ( !rlayerIds.isEmpty() )
  {
    QUrl identifyUrl( QgsSettings().value( "kadas/identifyurl", "" ).toString() );
    QUrlQuery query( identifyUrl );
    query.addQueryItem( "geometryType", "esriGeometryPoint" );
    query.addQueryItem( "geometry", QString( "%1,%2" ).arg( worldPos.x(), 0, 'f', 10 ).arg( worldPos.y(), 0, 'f', 10 ) );
    query.addQueryItem( "imageDisplay", QString( "%1,%2,%3" ).arg( mCanvas->width() ).arg( mCanvas->height() ).arg( mCanvas->mapSettings().outputDpi() ) );
    query.addQueryItem( "mapExtent", QString( "%1,%2,%3,%4" ).arg( worldExtent.xMinimum(), 0, 'f', 10 )
                        .arg( worldExtent.yMinimum(), 0, 'f', 10 )
                        .arg( worldExtent.xMaximum(), 0, 'f', 10 )
                        .arg( worldExtent.yMaximum(), 0, 'f', 10 ) );
    query.addQueryItem( "tolerance", "15" );
    query.addQueryItem( "layers", rlayerIds.join( "," ) );
    identifyUrl.setQuery( query );

    QNetworkRequest req( identifyUrl );
    mRasterIdentifyReply = QgsNetworkAccessManager::instance()->get( req );
    mRasterIdentifyReply->setProperty( "layerMap", rlayerMap );
    mTimeoutTimer = new QTimer( mRasterIdentifyReply );
    mTimeoutTimer->setSingleShot( true );
    connect( mRasterIdentifyReply, &QNetworkReply::finished, this, &KadasMapIdentifyDialog::rasterIdentifyFinished );
    connect( mTimeoutTimer, &QTimer::timeout, this, &KadasMapIdentifyDialog::rasterIdentifyFinished );
    mTimeoutTimer->start( 4000 );
  }
}

void KadasMapIdentifyDialog::addPluginLayerResults( KadasPluginLayer *pLayer, const QList<KadasPluginLayer::IdentifyResult> &results )
{
  if ( !mLayerTreeItemMap[pLayer->id()] )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << pLayer->name() );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( pLayer->id(), item );
    item->setExpanded( true );
    item->setFirstColumnSpanned( true );
  }

  for ( const KadasPluginLayer::IdentifyResult &result : results )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << result.featureName() );

    QgsAbstractGeometry *geomv2 = result.geometry().constGet()->clone();
    mGeometries.append( geomv2 );
    item->setData( 0, sGeometryRole, mGeometries.size() - 1 );
    item->setData( 0, sGeometryCrsRole, pLayer->crs().authid() );
    mLayerTreeItemMap[pLayer->id()]->addChild( item );

    for ( auto it = result.attributes().begin(), itEnd = result.attributes().end(); it != itEnd; ++it )
    {
      item->addChild( new QTreeWidgetItem( QStringList() << it.key() << it.value().toString() ) );
    }
    item->setExpanded( true );
  }
}

void KadasMapIdentifyDialog::addVectorLayerResult( QgsVectorLayer *vLayer, const QgsFeature &feature )
{
  if ( !mLayerTreeItemMap[vLayer->id()] )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << vLayer->name() );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( vLayer->id(), item );
    item->setExpanded( true );
    item->setFirstColumnSpanned( true );
  }

  QString label = vLayer->displayField().isEmpty() ? QString::number( feature.id() ) : QString( "%1 [%2]" ).arg( feature.attribute( vLayer->displayField() ).toString() ).arg( feature.id() );
  QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << label );
  QgsAbstractGeometry *geomv2 = feature.geometry().constGet()->clone();
  mGeometries.append( geomv2 );
  item->setData( 0, sGeometryRole, mGeometries.size() - 1 );
  item->setData( 0, sGeometryCrsRole, vLayer->crs().authid() );
  mLayerTreeItemMap[vLayer->id()]->addChild( item );

  QgsAttributes attributes = feature.attributes();
  for ( int i = 0, n = attributes.size(); i < n; ++i )
  {
    QString attribName = vLayer->attributeDisplayName( i );
    QStringList pair = QStringList() << attribName << attributes.at( i ).toString();
    item->addChild( new QTreeWidgetItem( pair ) );
  }
  item->setExpanded( true );
}

void KadasMapIdentifyDialog::addRasterIdentifyResult( QgsRasterLayer *rLayer, const QgsRasterIdentifyResult &result )
{
  QMap<QString, QString> sublayerNames;
  if ( rLayer->dataProvider()->name() == "arcgismapserver" )
  {
    QgsDataSourceUri dataSource( rLayer->dataProvider()->dataSourceUri() );
    QString serviceUrl = dataSource.param( QStringLiteral( "url" ) );
    QString authcfg = dataSource.authConfigId();
    QString trash;
    QVariantMap serviceInfo = QgsArcGisRestQueryUtils::getServiceInfo( serviceUrl, authcfg, trash, trash, dataSource.httpHeaders() );
    for ( const QVariant &entry : serviceInfo["layers"].toList() )
    {
      QVariantMap entryMap = entry.toMap();
      sublayerNames.insert( entryMap["id"].toString(), entryMap["name"].toString() );
    }
  }
  const QMap<int, QVariant> &results = result.results();
  if ( results.isEmpty() )
  {
    return;
  }
  if ( result.format() == Qgis::RasterIdentifyFormat::Feature )
  {
    bool nonempty = false;
    for ( const QVariant &variant : result.results() )
    {
      QgsFeatureStoreList stores = variant.value<QgsFeatureStoreList>();
      if ( !stores.isEmpty() )
      {
        nonempty = true;
        break;
      }
    }
    if ( !nonempty )
    {
      return;
    }
  }
  if ( !mLayerTreeItemMap[rLayer->id()] )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << rLayer->name() );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( rLayer->id(), item );
    item->setExpanded( true );
    item->setFirstColumnSpanned( true );
  }

  switch ( result.format() )
  {
    case Qgis::RasterIdentifyFormat::Undefined: break;
    case Qgis::RasterIdentifyFormat::Html: break;
    case Qgis::RasterIdentifyFormat::Value:
    {
      for ( auto resultIt = results.begin(), resultEnd = results.end(); resultIt != resultEnd; ++resultIt )
      {
        QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << tr( "Band %1" ).arg( resultIt.key() ) << resultIt.value().toString() );
        mLayerTreeItemMap[rLayer->id()]->addChild( item );
      }
      break;
    }
    case Qgis::RasterIdentifyFormat::Text:
    {
      for ( auto resultIt = results.begin(), resultEnd = results.end(); resultIt != resultEnd; ++resultIt )
      {
        QString sublayerName = rLayer->dataProvider()->subLayers()[ resultIt.key() ];
        QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << sublayerName << resultIt.value().toString() );
        mLayerTreeItemMap[rLayer->id()]->addChild( item );
      }
      break;
    }
    case Qgis::RasterIdentifyFormat::Feature:
    {
      for ( auto resultIt = results.begin(), resultEnd = results.end(); resultIt != resultEnd; ++resultIt )
      {
        QgsFeatureStoreList stores = resultIt.value().value<QgsFeatureStoreList>();

        for ( const QgsFeatureStore &store : stores )
        {
          for ( int i = 0, n = store.count(); i < n; ++i )
          {
            QgsFeature feature = store.features().at( i );
            QTreeWidgetItem *layerItem = nullptr;
            if ( resultIt.key() < rLayer->dataProvider()->subLayers().length() )
            {
              QString sublayerName = rLayer->dataProvider()->subLayers()[ resultIt.key() ];
              sublayerName = sublayerNames.value( sublayerName, sublayerName );
              layerItem = new QTreeWidgetItem( QStringList() << sublayerName );
              mLayerTreeItemMap[rLayer->id()]->addChild( layerItem );
            }
            else
            {
              layerItem = mLayerTreeItemMap[rLayer->id()];
            }

            // If OBJECTID available use as identifier
            QVariant featureId = feature.attribute( "OBJECTID" );
            if ( ! featureId.isValid() || featureId.isNull() )
              featureId = feature.id();

            QString label = QString( "%1 [%2]" ).arg( rLayer->name() ).arg( featureId.toString() );
            QTreeWidgetItem *featureItem = new QTreeWidgetItem( QStringList() << label );
            layerItem->addChild( featureItem );
            featureItem->setExpanded( true );
            featureItem->setFirstColumnSpanned( true );

            for ( int j = 0, m = feature.attributeCount(); j < m; ++j )
            {
              const QgsField &field = feature.fields().at( j );
              QStringList pair = QStringList() << field.displayName() << field.displayString( feature.attribute( j ) );
              featureItem->addChild( new QTreeWidgetItem( pair ) );
            }
            layerItem->setExpanded( true );
            layerItem->setFirstColumnSpanned( true );
          }
        }
      }
      break;
    }
  }
}

void KadasMapIdentifyDialog::rasterIdentifyFinished()
{
  if ( !mRasterIdentifyReply )
  {
    return;
  }

  QVariantList results;
  if ( mRasterIdentifyReply->error() == QNetworkReply::NoError && mTimeoutTimer->isActive() )
  {
    results = QJsonDocument::fromJson( mRasterIdentifyReply->readAll() ).object().toVariantMap()["results"].toList();
  }

  QMap<QString, QVariant> layerMap = mRasterIdentifyReply->property( "layerMap" ).toMap();

  mRasterIdentifyReply->deleteLater();
  mRasterIdentifyReply = nullptr;
  mTimeoutTimer = nullptr;

  // Add results to tree
  for ( int i = 0, n = results.size(); i < n; ++i )
  {
    QVariantMap result = results[i].toMap();
    QString layerId = result["layerId"].toString();
    if ( !layerMap.contains( layerId ) )
    {
      continue;
    }
    QString qgisLayerId = layerMap[layerId].toString();
    if ( !mLayerTreeItemMap.contains( layerId ) )
    {
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( qgisLayerId );
      if ( !layer )
      {
        mLayerTreeItemMap.insert( layerId, 0 );
      }
      else
      {
        QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << layer->name() );
        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        mLayerTreeItemMap.insert( layerId, item );
        mTreeWidget->invisibleRootItem()->addChild( item );
        item->setExpanded( true );
        item->setFirstColumnSpanned( true );
      }
    }
    QTreeWidgetItem *parent = mLayerTreeItemMap[layerId];
    if ( !parent )
    {
      continue;
    }
    QTreeWidgetItem *resultItem = new QTreeWidgetItem( QStringList() << "" );
    QgsCoordinateReferenceSystem crs;
    QgsAbstractGeometry *geometryV2 = QgsArcGisRestUtils::convertGeometry( result["geometry"].toMap(), result["geometryType"].toString(), false, false, &crs );
    mGeometries.append( geometryV2 );

    resultItem->setData( 0, sGeometryRole, mGeometries.size() - 1 );
    resultItem->setData( 0, sGeometryCrsRole, crs.authid() );

    QString displayFieldName = result["displayFieldName"].toString();
    QVariantMap attributes = result["attributes"].toMap();
    for ( const QString &key : attributes.keys() )
    {
      QString value = attributes[key].toString();
      QTreeWidgetItem *attrItem = new QTreeWidgetItem( QStringList() << key << value );
      if ( key == displayFieldName )
        resultItem->setText( 0, value );
      resultItem->addChild( attrItem );
    }
    parent->addChild( resultItem );
    resultItem->setExpanded( true );
    resultItem->setFirstColumnSpanned( true );
  }
}

