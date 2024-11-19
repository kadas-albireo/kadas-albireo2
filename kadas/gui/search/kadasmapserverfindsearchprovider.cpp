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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsarcgisrestutils.h>
#include <qgis/qgsdatasourceuri.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsjsonutils.h>


#include "kadas/gui/search/kadasmapserverfindsearchprovider.h"


const int KadasMapServerFindSearchProvider::sSearchTimeout = 10000;
const int KadasMapServerFindSearchProvider::sResultCountLimit = 100;


KadasMapServerFindSearchProvider::KadasMapServerFindSearchProvider( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );
}

QgsLocatorFilter *KadasMapServerFindSearchProvider::clone() const
{
  return new KadasMapServerFindSearchProvider( mMapCanvas );
}

void KadasMapServerFindSearchProvider::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  // List queryable rasters
  typedef QPair<QString, QString> LayerUrlName; // <layerurl, layername>
  QList<LayerUrlName> queryableLayers;
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
  if ( !context.targetExtent.isNull() )
  {
    QgsCoordinateTransform ct( QgsCoordinateReferenceSystem( context.targetExtentCrs ), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
    QgsRectangle box = ct.transformBoundingBox( context.targetExtent );
    spatialFilter = QString( "{\"spatialRel\": \"esriSpatialRelIntersects\", \"geometryType\": \"esriGeometryEnvelope\", \"geometry\": { \"xmin\": %1, \"ymin\": %2, \"xmax\": %3, \"ymax\": %4, \"spatialReference\": {\"wkid\": 4326}}}" )
                      .arg( box.xMinimum(), 0, 'f', 4 )
                      .arg( box.yMinimum(), 0, 'f', 4 )
                      .arg( box.xMaximum(), 0, 'f', 4 )
                      .arg( box.yMaximum(), 0, 'f', 4 );
  }

  for ( const LayerUrlName &ql : queryableLayers )
  {
    QUrl url( ql.first + "/find" );
    QUrlQuery query( url );
    query.addQueryItem( "f", "json" );
    query.addQueryItem( "searchText", string );
    query.addQueryItem( "layers", ql.second );
    query.addQueryItem( "spatialFilter", spatialFilter );

    url.setQuery( query );
    QNetworkRequest req( url );
    req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( feedback, &QgsFeedback::canceled, reply, &QNetworkReply::abort );
    connect( reply, &QNetworkReply::finished, this, [this, reply]() {
      if ( reply->error() == QNetworkReply::NoError )
      {
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
          QVariantMap itemAttrsMap = itemMap["attributes"].toMap();
          QString authid = QString( "EPSG:%1" ).arg( itemAttrsMap["spatialReference"].toMap()["wkid"].toString() );
          QgsCoordinateReferenceSystem crs( authid );
          QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
          QgsAbstractGeometry *geom = QgsArcGisRestUtils::convertGeometry( itemMap["geometry"].toMap(), itemMap["geometryType"].toString(), false, false, &crs );
          geom->transform( QgsCoordinateTransform( crs, crsWgs84, QgsProject::instance() ) );

          QgsLocatorResult result;
          QVariantMap resultData;

          resultData[QStringLiteral( "geometry" )] = geom->asJson( 5 );
          resultData[QStringLiteral( "bbox" )] = geom->boundingBox();
          resultData[QStringLiteral( "pos" )] = QgsPointXY( geom->centroid() );
          // resultData[QStringLiteral( "zoomScale" )] = 1000;
          result.group = tr( "Layer %1" ).arg( itemMap["layerName"].toString() );
          result.displayString = QString( "%1: %2" ).arg( itemMap["foundFieldName"].toString(), itemMap["value"].toString() );
          delete geom;

          result.setUserData( resultData );
          emit resultFetched( result );
        }
      }
      reply->deleteLater();
    } );
  }
}

void KadasMapServerFindSearchProvider::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString geometry = data.value( QStringLiteral( "geometry" ) ).toString();

  QgsPointXY itemPos = QgsCoordinateTransform(
                         QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ),
                         mMapCanvas->mapSettings().destinationCrs(),
                         QgsProject::instance()
  )
                         .transform( pos );

  mMapCanvas->setCenter( itemPos );

  // not sure if we will get this from somewhere, it is not documented in the swisstopo API
  // also, there is no coordinate transform, so this is probably broken
  if ( !geometry.isEmpty() )
  {
    QString feature = QString( "{\"type\": \"FeatureCollection\", \"features\": [{\"type\": \"feature\", \"geometry\": %1}]}" ).arg( geometry );
    QgsFeatureList features = QgsJsonUtils::stringToFeatureList( feature );
    if ( !features.isEmpty() && !features[0].geometry().isEmpty() )
    {
      QgsGeometry geometry = features[0].geometry();
      QgsAnnotationItem *item = nullptr;
      switch ( features[0].geometry().type() )
      {
        case Qgis::GeometryType::Point:
        {
          QgsPoint *pt = qgsgeometry_cast<QgsPoint *>( geometry.get() );
          item = new QgsAnnotationMarkerItem( *pt );
          break;
        }
        case Qgis::GeometryType::Line:
        {
          QgsCurve *curve = qgsgeometry_cast<QgsCurve *>( geometry.get() );
          item = new QgsAnnotationLineItem( curve );
          break;
        }
        case Qgis::GeometryType::Polygon:
        {
          QgsCurvePolygon *poly = qgsgeometry_cast<QgsCurvePolygon *>( geometry.get() );
          item = new QgsAnnotationPolygonItem( poly );
          break;
        }
        case Qgis::GeometryType::Unknown:
        case Qgis::GeometryType::Null:
          break;
      }
      if ( item )
      {
        mGeometryItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );
      }
    }
  }
}

void KadasMapServerFindSearchProvider::clearPreviousResults()
{
  if ( !mGeometryItemId.isEmpty() )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( mGeometryItemId );
    mGeometryItemId = QString();
  }
}
