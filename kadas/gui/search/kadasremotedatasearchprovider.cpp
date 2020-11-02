/***************************************************************************
    kadasremotedatasearchprovider.cpp
    ---------------------------------
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

#include <kadas/gui/search/kadasremotedatasearchprovider.h>


const int KadasRemoteDataSearchProvider::sSearchTimeout = 2000;
const int KadasRemoteDataSearchProvider::sResultCountLimit = 100;


KadasRemoteDataSearchProvider::KadasRemoteDataSearchProvider( QgsMapCanvas *mapCanvas )
  : KadasSearchProvider( mapCanvas )
{
  mReplyFilter = 0;

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, &QTimer::timeout, this, &KadasRemoteDataSearchProvider::searchTimeout );
}

void KadasRemoteDataSearchProvider::startSearch( const QString &searchtext, const SearchRegion &searchRegion )
{
  // List queryable rasters
  typedef QPair<QString, QString> LayerIdName; // <layerid, layername>
  QList< LayerIdName > queryableLayers;
  for ( const QgsMapLayer *layer : QgsProject::instance()->mapLayers() )
  {
    const QgsRasterLayer *rlayer = qobject_cast<const QgsRasterLayer *> ( layer );
    if ( !rlayer )
    {
      continue;
    }

    // Detect ArcGIS Rest MapServer layers
    QgsDataSourceUri dataSource( rlayer->dataProvider()->dataSourceUri() );
    QStringList urlParts = dataSource.param( "url" ).split( "/", Qt::SkipEmptyParts );
    int nParts = urlParts.size();
    if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
    {
      queryableLayers.append( qMakePair( urlParts[nParts - 3] + ":" + urlParts[nParts - 2], rlayer->name() ) );
    }

    // Detect geo.admin.ch layers
    dataSource.setEncodedUri( rlayer->dataProvider()->dataSourceUri() );
    if ( dataSource.param( "url" ).contains( "geo.admin.ch" ) )
    {
      for ( const QString &id : dataSource.params( "layers" ) )
      {
        queryableLayers.append( qMakePair( id, rlayer->name() ) );
      }
    }
  }
  if ( queryableLayers.isEmpty() )
  {
    return;
  }

  for ( const LayerIdName &ql : queryableLayers )
  {
    QUrl url( QgsSettings().value( "search/remotedatasearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString() );
    QUrlQuery query( url );
    query.addQueryItem( "type", "featuresearch" );
    query.addQueryItem( "searchText", searchtext );
    query.addQueryItem( "features", ql.first );
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
      query.addQueryItem( "bbox", QString( "%1,%2,%3,%4" ).arg( rect.xMinimum(), 0, 'f', 4 ).arg( rect.yMinimum(), 0, 'f', 4 ).arg( rect.xMaximum(), 0, 'f', 4 ).arg( rect.yMaximum(), 0, 'f', 4 ) );
      QgsPolygon *poly = new QgsPolygon();
      poly->setExteriorRing( exterior );
      mReplyFilter = new QgsGeometry( poly );
    }

    url.setQuery( query );
    QNetworkRequest req( url );
    req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    reply->setProperty( "layerName", ql.second );
    connect( reply, &QNetworkReply::finished, this, &KadasRemoteDataSearchProvider::replyFinished );
    mNetReplies.append( reply );
  }
  mTimeoutTimer.start( sSearchTimeout );
}

void KadasRemoteDataSearchProvider::cancelSearch()
{
  mTimeoutTimer.stop();
  while ( !mNetReplies.isEmpty() )
  {
    QNetworkReply *reply = mNetReplies.front();
    disconnect( reply, &QNetworkReply::finished, this, &KadasRemoteDataSearchProvider::replyFinished );
    reply->close();
    mNetReplies.removeAll( reply );
    reply->deleteLater();
  }
  delete mReplyFilter;
  mReplyFilter = 0;
}

void KadasRemoteDataSearchProvider::replyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( !reply )
  {
    return;
  }

  if ( reply->error() == QNetworkReply::NoError )
  {
    QString layerName = reply->property( "layerName" ).toString();
    QStringList bboxStr = QUrlQuery( reply->request().url().query() ).queryItemValue( "bbox" ).split( "," );
    QgsRectangle bbox;
    if ( bboxStr.size() == 4 )
    {
      bbox.setXMinimum( bboxStr[0].toDouble() );
      bbox.setYMinimum( bboxStr[1].toDouble() );
      bbox.setXMaximum( bboxStr[2].toDouble() );
      bbox.setYMaximum( bboxStr[3].toDouble() );
    }

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
      QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

      if ( !mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
      {
        QgsDebugMsg( "Box RegEx did not match " + itemAttrsMap["geom_st_box2d"].toString() );
        continue;
      }

      SearchResult searchResult;
      searchResult.crs = itemAttrsMap["sr"].toString();
      searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                        mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
      // When bbox is empty, fallback to pos + zoomScale is used
      searchResult.pos = QgsPointXY( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
      QgsCoordinateTransform ct( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsCoordinateReferenceSystem( searchResult.crs ), QgsProject::instance() );
      if ( !bbox.isEmpty() && !bbox.contains( searchResult.pos ) )
      {
        continue;
      }
      if ( mReplyFilter && !mReplyFilter->contains( &searchResult.pos ) )
      {
        continue;
      }

      searchResult.pos = ct.transform( searchResult.pos );
      searchResult.zoomScale = 1000;
      searchResult.category = tr( "Layer %1" ).arg( layerName );
      searchResult.categoryPrecedence = 11;
      searchResult.text = itemAttrsMap["label"].toString() + " (" + itemAttrsMap["detail"].toString() + ")";
      searchResult.text.replace( QRegExp( "<[^>]+>" ), "" );   // Remove HTML tags
      searchResult.showPin = true;
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
