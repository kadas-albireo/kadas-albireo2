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
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QJsonDocument>

#include <qgis/qgsarcgisrestutils.h>
#include <qgis/qgsgeometryrubberband.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/app/kadasmapidentifydialog.h>


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
  delete mResultPin;
  mResultPin = nullptr;
  delete mClickPosPin;
  mClickPosPin = nullptr;
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
  delete mResultPin;
  mResultPin = nullptr;

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
      const QgsRasterLayer *rlayer = static_cast<const QgsRasterLayer *>( layer );

      // Detect ArcGIS Rest MapServer layers
      QgsDataSourceUri dataSource( rlayer->dataProvider()->dataSourceUri() );
      QStringList urlParts = dataSource.param( "url" ).split( "/", QString::SkipEmptyParts );
      int nParts = urlParts.size();
      if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
      {
        rlayerIds.append( urlParts[nParts - 3] + ":" + urlParts[nParts - 2] );
        rlayerMap.insert( urlParts[nParts - 2], rlayer->id() );
      }

      // Detect geo.admin.ch layers
      dataSource.setEncodedUri( rlayer->dataProvider()->dataSourceUri() );
      if ( dataSource.param( "url" ).contains( "geo.admin.ch" ) )
      {
        QStringList sublayerIds = dataSource.params( "layers" );
        rlayerIds.append( sublayerIds );
        for ( const QString &id : sublayerIds )
        {
          rlayerMap.insert( id, rlayer->id() );
        }
      }
    }
    else if ( dynamic_cast<QgsVectorLayer *>( layer ) )
    {
      QgsVectorLayer *vlayer = static_cast<QgsVectorLayer *>( layer );
      if ( vlayer->hasScaleBasedVisibility() &&
           ( vlayer->minimumScale() > mCanvas->mapSettings().scale() ||
             vlayer->maximumScale() <= mCanvas->mapSettings().scale() ) )
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
      QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
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
    item->setFirstColumnSpanned( true );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( pLayer->id(), item );
    item->setExpanded( true );
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
    item->setFirstColumnSpanned( true );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( vLayer->id(), item );
    item->setExpanded( true );
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

// TODO: Redlining item attributes?

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
        item->setFirstColumnSpanned( true );
        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        mLayerTreeItemMap.insert( layerId, item );
        mTreeWidget->invisibleRootItem()->addChild( item );
        item->setExpanded( true );
      }
    }
    QTreeWidgetItem *parent = mLayerTreeItemMap[layerId];
    if ( !parent )
    {
      continue;
    }
    QTreeWidgetItem *resultItem = new QTreeWidgetItem( QStringList() << "" );
    QgsCoordinateReferenceSystem crs;
    QgsAbstractGeometry *geometryV2 = QgsArcGisRestUtils::parseEsriGeoJSON( result["geometry"].toMap(), result["geometryType"].toString(), false, false, &crs ).release();
    mGeometries.append( geometryV2 );

    resultItem->setData( 0, sGeometryRole, mGeometries.size() - 1 );
    resultItem->setData( 0, sGeometryCrsRole, crs.authid() );

    resultItem->setFirstColumnSpanned( true );
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
  }
}

