/***************************************************************************
    kadasworldlocationsearchprovider.cpp
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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsjsonutils.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgscurve.h>
#include <qgis/qgscurvepolygon.h>

#include <kadas/gui/search/kadasworldlocationsearchprovider.h>


const int KadasWorldLocationSearchProvider::sSearchTimeout = 10000;
const int KadasWorldLocationSearchProvider::sResultCountLimit = 50;


KadasWorldLocationSearchProvider::KadasWorldLocationSearchProvider( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  mCategoryMap.insert( "geonames", qMakePair( tr( "World Places" ), 30 ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );
}

QgsLocatorFilter *KadasWorldLocationSearchProvider::clone() const
{
  return new KadasWorldLocationSearchProvider( mMapCanvas );
}

void KadasWorldLocationSearchProvider::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  QString serviceUrl;
  if ( QgsSettings().value( "/kadas/isOffline" ).toBool() )
  {
    serviceUrl = QgsSettings().value( "search/worldlocationofflinesearchurl", "http://localhost:5000/SearchServerWld" ).toString();
  }
  else
  {
    serviceUrl = QgsSettings().value( "search/worldlocationsearchurl", "" ).toString();
  }

  QUrl url( serviceUrl );
  QUrlQuery query( url );
  query.removeAllQueryItems( "type" );
  query.removeAllQueryItems( "searchText" );
  query.removeAllQueryItems( "sr" );
  query.addQueryItem( "type", "locations" );
  query.addQueryItem( "searchText", string );
  query.addQueryItem( "sr", "4326" );
  if ( !query.hasQueryItem( "limit" ) )
  {
    query.addQueryItem( "limit", QString::number( sResultCountLimit ) );
  }
  url.setQuery( query );

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );

  connect( feedback, &QgsFeedback::canceled, reply, &QNetworkReply::abort );
  connect( reply, &QNetworkReply::finished, this, [this, reply]()
  {
    if ( reply->error() == QNetworkReply::NoError )
    {
      QByteArray replyText = reply->readAll();
      QJsonParseError err;
      QJsonDocument doc = QJsonDocument::fromJson( replyText, &err );
      if ( doc.isNull() )
      {
        QgsDebugMsgLevel( QString( "Parsing error:" ).arg( err.errorString() ), 2 );
      }
      QJsonObject resultMap = doc.object();
      const QJsonArray constResults = resultMap["results"].toArray();
      for ( const QJsonValue &item : constResults )
      {
        QJsonObject itemMap = item.toObject();
        QJsonObject itemAttrsMap = itemMap["attrs"].toObject();


        QString origin = itemAttrsMap["origin"].toString();

        QgsLocatorResult result;
        QVariantMap resultData;
        resultData[QStringLiteral( "pos" )] = QgsPointXY( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );

        result.group = mCategoryMap.contains( origin ) ? mCategoryMap[origin].first : origin;
        // TODO QGIS 3.40: uncomment
        // result.groupScore = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 1;
        QString label = itemAttrsMap["label"].toString();
        label.replace( QRegExp( "<[^>]+>" ), "" );   // Remove HTML tags
        result.displayString = label;

        if ( itemAttrsMap.contains( "geometryGeoJSON" ) )
        {
          resultData[QStringLiteral( "geometry" )] = QJsonDocument( itemAttrsMap["geometryGeoJSON"].toObject() ).toJson( QJsonDocument::Compact );
        }
        if ( itemAttrsMap.contains( "boundingBox" ) )
        {
          static QRegularExpression bboxRe( "BOX\\s*\\(\\s*(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)\\s*,\\s*(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)\\s*\\)", QRegularExpression::CaseInsensitiveOption );
          QRegularExpressionMatch match = bboxRe.match( itemAttrsMap["boundingBox"].toString() );
          if ( match.isValid() )
          {
            resultData[QStringLiteral( "bbox" )] = QgsRectangle( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble(), match.captured( 3 ).toDouble(), match.captured( 4 ).toDouble() );
          }
        }
        result.setUserData( resultData );
        emit resultFetched( result );
      }
    }
    reply->deleteLater();
  } );
}

void KadasWorldLocationSearchProvider::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString geometry = data.value( QStringLiteral( "geometry" ) ).toString();
  QgsRectangle bbox = data.value( QStringLiteral( "bbox" ) ).value<QgsRectangle>();

  QgsCoordinateTransform ct( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mMapCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
  QgsPointXY itemPos = ct.transform( pos );

  bool geomShown = false;
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
          QgsPoint *pt = qgsgeometry_cast< QgsPoint * >( geometry.get() );
          item = new QgsAnnotationMarkerItem( *pt );
          break;
        }
        case Qgis::GeometryType::Line:
        {
          QgsCurve *curve = qgsgeometry_cast< QgsCurve * >( geometry.get() );
          item = new QgsAnnotationLineItem( curve );
          break;
        }
        case Qgis::GeometryType::Polygon:
        {
          QgsCurvePolygon *poly = qgsgeometry_cast< QgsCurvePolygon * >( geometry.get() );
          item = new QgsAnnotationPolygonItem( poly );
          break;
        }
        case Qgis::GeometryType::Unknown:
        case Qgis::GeometryType::Null:
          break;
      }
      if ( item )
      {
        geomShown = true;
        mGeometryItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );
      }
    }
  }

  if ( !geomShown )
  {
    QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( itemPos ) );
    QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
    symbolLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayer::VerticalAnchorPoint::Bottom );
    item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
    mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );
  }

  if ( !bbox.isNull() )
  {
    bbox = ct.transform( bbox );
    mMapCanvas->setExtent( bbox );
  }
  else
  {
    mMapCanvas->setCenter( itemPos );
  }

}


void KadasWorldLocationSearchProvider::clearPreviousResults()
{
  if ( !mPinItemId.isEmpty() )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( mPinItemId );
    mPinItemId = QString();
  }
  if ( !mGeometryItemId.isEmpty() )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( mGeometryItemId );
    mGeometryItemId = QString();
  }
}
