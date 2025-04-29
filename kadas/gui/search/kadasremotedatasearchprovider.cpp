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

#include <qgis/qgsannotationlayer.h>
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
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmarkersymbol.h>

#include "kadas/gui/search/kadasremotedatasearchprovider.h"


const int KadasRemoteDataSearchProvider::sSearchTimeout = 10000;
const int KadasRemoteDataSearchProvider::sResultCountLimit = 100;


KadasRemoteDataSearchProvider::KadasRemoteDataSearchProvider( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );
}

QgsLocatorFilter *KadasRemoteDataSearchProvider::clone() const
{
  return new KadasRemoteDataSearchProvider( mMapCanvas );
}

void KadasRemoteDataSearchProvider::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  if ( string.length() < 3 )
    return;

  QString remoteDataSearchUrl = QgsSettings().value( "search/remotedatasearchurl", "" ).toString();
  if ( remoteDataSearchUrl.isEmpty() )
    return;

  // List queryable rasters
  typedef QPair<QString, QString> LayerIdName; // <layerid, layername>
  QList<LayerIdName> queryableLayers;
  const auto layers = QgsProject::instance()->layers<QgsRasterLayer *>();
  for ( const QgsRasterLayer *rlayer : layers )
  {
    // Detect ArcGIS Rest MapServer layers
    if ( rlayer->providerType() == "arcgismapserver" )
    {
      QgsDataSourceUri dataSource( rlayer->dataProvider()->dataSourceUri() );
      QStringList urlParts = dataSource.param( "url" ).split( "/", Qt::SkipEmptyParts );
      int nParts = urlParts.size();
      // Example: https://<...>/services/<group>/<service>/MapServer
      if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
      {
        queryableLayers.append( qMakePair( urlParts[nParts - 3] + ":" + urlParts[nParts - 2], rlayer->name() ) );
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
        queryableLayers.append( qMakePair( urlParts[nParts - 4] + ":" + urlParts[nParts - 3], rlayer->name() ) );
      }
      // Detect MapServer WMTS Layers
      // Example: https://<...>/rest/services/<group>/<service>/MapServer/WMTS/1.0.0/WMTSCapabilities.xml
      else if ( nParts > 8 && urlParts[nParts - 1] == "WMTSCapabilities.xml" && urlParts[nParts - 4] == "MapServer" && urlParts[nParts - 7] == "services" )
      {
        queryableLayers.append( qMakePair( urlParts[nParts - 6] + ":" + urlParts[nParts - 5], rlayer->name() ) );
      }
      // Detect geo.admin.ch layers
      else if ( nParts > 1 && urlParts[1].endsWith( "geo.admin.ch" ) )
      {
        const QStringList params = dataSource.params( "layers" );
        for ( const QString &id : params )
        {
          queryableLayers.append( qMakePair( id, rlayer->name() ) );
        }
      }
    }
  }
  if ( queryableLayers.isEmpty() )
  {
    return;
  }

  QgsCoordinateTransform ct( QgsCoordinateReferenceSystem( context.targetExtentCrs ), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  QgsRectangle bbox;
  if ( !context.targetExtent.isNull() )
    bbox = ct.transformBoundingBox( context.targetExtent );

  for ( const LayerIdName &ql : queryableLayers )
  {
    QUrl url( remoteDataSearchUrl );
    QUrlQuery query( url );
    query.addQueryItem( "type", "featuresearch" );
    query.addQueryItem( "searchText", string );
    query.addQueryItem( "features", ql.first );
    QgsRectangle bbox;
    if ( !bbox.isNull() )
    {
      query.addQueryItem( "bbox", QString( "%1,%2,%3,%4" ).arg( bbox.xMinimum(), 0, 'f', 4 ).arg( bbox.yMinimum(), 0, 'f', 4 ).arg( bbox.xMaximum(), 0, 'f', 4 ).arg( bbox.yMaximum(), 0, 'f', 4 ) );
    }
    url.setQuery( query );
    QNetworkRequest req( url );
    req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    reply->setProperty( "layerName", ql.second );

    connect( feedback, &QgsFeedback::canceled, reply, &QNetworkReply::abort );
    connect( reply, &QNetworkReply::finished, this, [this, reply, &bbox]() {
      if ( reply->error() == QNetworkReply::NoError )
      {
        QString layerName = reply->property( "layerName" ).toString();
        layerName.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags

        QByteArray replyText = reply->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson( replyText, &err );
        if ( doc.isNull() )
        {
          QgsDebugMsgLevel( QString( "Parsing error:" ).arg( err.errorString() ), 2 );
        }
        QVariantMap resultMap = doc.object().toVariantMap();
        for ( const QVariant &item : resultMap["results"].toList() )
        {
          QVariantMap itemMap = item.toMap();
          QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

          if ( !mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
          {
            QgsDebugMsgLevel( "Box RegEx did not match " + itemAttrsMap["geom_st_box2d"].toString(), 2 );
            continue;
          }

          QgsLocatorResult result;
          QVariantMap resultData;

          const QString crs = itemAttrsMap["sr"].toString();
          QgsPointXY pos( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
          resultData[QStringLiteral( "crs" )] = crs;
          resultData[QStringLiteral( "bbox" )] = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(), mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
          if ( !bbox.isNull() && !bbox.contains( pos ) )
            continue;

          resultData[QStringLiteral( "pos" )] = pos;

          result.group = tr( "Layer %1" ).arg( layerName );
          result.displayString = itemAttrsMap["label"].toString() + " (" + itemAttrsMap["detail"].toString() + ")";

          result.setUserData( resultData );
          emit resultFetched( result );
        }
      }
      reply->deleteLater();
    } );
  }
}

void KadasRemoteDataSearchProvider::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsRectangle bbox = data.value( QStringLiteral( "bbox" ) ).value<QgsRectangle>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString crs = data.value( QStringLiteral( "crs" ) ).toString();

  QgsPointXY itemPos = QgsCoordinateTransform(
                         QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ),
                         mMapCanvas->mapSettings().destinationCrs(),
                         QgsProject::instance()
  )
                         .transform( pos );

  QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( itemPos ) );
  QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
  symbolLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayer::VerticalAnchorPoint::Bottom );
  item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
  mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );

  if ( !bbox.isNull() )
  {
    QgsRectangle bboxCanvas = QgsCoordinateTransform( QgsCoordinateReferenceSystem( crs ), mMapCanvas->mapSettings().destinationCrs(), QgsProject::instance() ).transform( bbox );
    mMapCanvas->setExtent( bboxCanvas, true );
  }
  else
  {
    mMapCanvas->setCenter( itemPos );
  }
}

void KadasRemoteDataSearchProvider::clearPreviousResults()
{
  if ( !mPinItemId.isEmpty() )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( mPinItemId );
    mPinItemId = QString();
  }
}
