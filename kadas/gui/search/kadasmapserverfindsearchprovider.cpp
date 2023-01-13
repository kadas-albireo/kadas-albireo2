/***************************************************************************
    kadasmapserverfindsearchprovider.cpp
    ------------------------------------
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <qgis/qgsarcgisrestutils.h>
#include <qgis/qgsdatasourceuri.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/search/kadasmapserverfindsearchprovider.h>


const int KadasMapServerFindSearchProvider::sSearchTimeout = 10000;
const int KadasMapServerFindSearchProvider::sResultCountLimit = 100;


KadasMapServerFindSearchProvider::KadasMapServerFindSearchProvider( QgsMapCanvas *mapCanvas )
  : KadasSearchProvider( mapCanvas )
{
  mReplyFilter = 0;

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, &QTimer::timeout, this, &KadasMapServerFindSearchProvider::searchTimeout );
}

void KadasMapServerFindSearchProvider::startSearch( const QString &searchtext, const SearchRegion &searchRegion )
{
  // List queryable rasters
  typedef QPair<QString, QString> LayerUrlName; // <layerurl, layername>
  QList< LayerUrlName > queryableLayers;
  for ( const QgsMapLayer *layer : QgsProject::instance()->mapLayers() )
  {
    const QgsRasterLayer *rlayer = qobject_cast<const QgsRasterLayer *> ( layer );
    if ( !rlayer )
    {
      continue;
    }

    // Detect ArcGIS Rest MapServer layers
    if ( rlayer->providerType() == "arcgismapserver" )
    {
      QgsDataSourceUri dataSource( rlayer->dataProvider()->dataSourceUri() );
      QStringList urlParts = dataSource.param( "url" ).split( "/", Qt::SkipEmptyParts );
      int nParts = urlParts.size();
      // Example: https://<...>/services/<group>/<service>/MapServer
      if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
      {
        QStringList layerList;
        layerList.append( dataSource.param( "layer" ) );
        layerList.append( rlayer->dataProvider()->subLayers() );
        queryableLayers.append( qMakePair( dataSource.param( "url" ), layerList.join( "," ) ) );
      }
    }

    // Detect WMS served by ArcGIS MapServer
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
        queryableLayers.append( qMakePair( urlParts.mid( 0, urlParts.length() - 1 ).join( "/" ), rlayer->name() ) );
      }
      // Detect MapServer WMTS Layers
      // Example: https://<...>/rest/services/<group>/<service>/MapServer/WMTS/1.0.0/WMTSCapabilities.xml
      else if ( nParts > 8 && urlParts[nParts - 1] == "WMTSCapabilities.xml" && urlParts[nParts - 4] == "MapServer" && urlParts[nParts - 7] == "services" )
      {
        queryableLayers.append( qMakePair( urlParts.mid( 0, urlParts.length() - 3 ).join( "/" ), rlayer->name() ) );
      }
    }
  }
  if ( queryableLayers.isEmpty() )
  {
    return;
  }

  QString spatialFilter;
  if ( !searchRegion.polygon.isEmpty() )
  {
    QgsRectangle rect;
    rect.setMinimal();
    QgsLineString *exterior = new QgsLineString();
    QgsCoordinateTransform ct = QgsCoordinateTransform( QgsCoordinateReferenceSystem( searchRegion.crs ), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
    for ( const QgsPointXY &p : searchRegion.polygon )
    {
      QgsPointXY pt = ct.transform( p );
      rect.include( pt );
      exterior->addVertex( QgsPoint( pt ) );
    }
    QgsPolygon *poly = new QgsPolygon();
    poly->setExteriorRing( exterior );
    mReplyFilter = new QgsGeometry( poly );
    spatialFilter = QString( "{\"spatialRel\": \"esriSpatialRelIntersects\", \"geometryType\": \"esriGeometryEnvelope\", \"geometry\": { \"xmin\": %1, \"ymin\": %2, \"xmax\": %3, \"ymax\": %4, \"spatialReference\": {\"wkid\": 4326}}}" )
                    .arg( rect.xMinimum(), 0, 'f', 4 ).arg( rect.yMinimum(), 0, 'f', 4 ).arg( rect.xMaximum(), 0, 'f', 4 ).arg( rect.yMaximum(), 0, 'f', 4 );
  }

  for ( const LayerUrlName &ql : queryableLayers )
  {
    QUrl url( ql.first + "/find" );
    QUrlQuery query( url );
    query.addQueryItem( "f", "json" );
    query.addQueryItem( "searchText", searchtext );
    query.addQueryItem( "layers", ql.second );
    query.addQueryItem( "spatialFilter", spatialFilter );

    url.setQuery( query );
    QNetworkRequest req( url );
    req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( reply, &QNetworkReply::finished, this, &KadasMapServerFindSearchProvider::replyFinished );
    mNetReplies.append( reply );
  }
  mTimeoutTimer.start( sSearchTimeout );
}

void KadasMapServerFindSearchProvider::cancelSearch()
{
  mTimeoutTimer.stop();
  while ( !mNetReplies.isEmpty() )
  {
    QNetworkReply *reply = mNetReplies.front();
    disconnect( reply, &QNetworkReply::finished, this, &KadasMapServerFindSearchProvider::replyFinished );
    reply->close();
    mNetReplies.removeAll( reply );
    reply->deleteLater();
  }
  delete mReplyFilter;
  mReplyFilter = 0;
}

void KadasMapServerFindSearchProvider::replyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( !reply )
  {
    return;
  }

  if ( reply->error() == QNetworkReply::NoError )
  {
    QByteArray replyText = reply->readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson( replyText, &err );
    if ( doc.isNull() )
    {
      QgsDebugMsg( QString( "Parsing error:" ).arg( err.errorString() ) );
    }
    QVariantMap resultMap = doc.object().toVariantMap();
    for ( const QVariant &item : resultMap["results"].toList() )
    {
      QVariantMap itemMap = item.toMap();
      QVariantMap itemAttrsMap = itemMap["attributes"].toMap();
      QString authid = QString( "EPSG:%1" ).arg( itemAttrsMap["spatialReference"].toMap()["wkid"].toString() );
      QgsCoordinateReferenceSystem crs( authid );
      QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
      QgsAbstractGeometry *geom = QgsArcGisRestUtils::convertGeometry( itemMap["geometry"].toMap(), itemMap["geometryType"].toString(), false, false, &crs );
      geom->transform( QgsCoordinateTransform( crs, crsWgs84, QgsProject::instance() ) );

      SearchResult searchResult;
      searchResult.crs = crsWgs84.authid();
      searchResult.geometry = geom->asJson( 5 );
      searchResult.bbox = geom->boundingBox();
      searchResult.pos = geom->centroid();
      searchResult.zoomScale = 1000;
      searchResult.category = tr( "Layer %1" ).arg( itemMap["layerName"].toString() );
      searchResult.categoryPrecedence = 11;
      searchResult.text = QString( "%1: %2" ).arg( itemMap["foundFieldName"].toString() ).arg( itemMap["value"].toString() );
      searchResult.showPin = false;
      delete geom;
      emit searchResultFound( searchResult );
    }
  }
  reply->deleteLater();
  mNetReplies.removeAll( reply );
  if ( mNetReplies.isEmpty() )
  {
    delete mReplyFilter;
    mReplyFilter = 0;
    emit searchFinished();
  }
}
