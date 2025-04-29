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
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgscurve.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsjsonutils.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmultisurface.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/search/kadasworldlocationsearchprovider.h"
#include "kadas/gui/search/kadaslocationsearchprovider.h"

const int KadasWorldLocationSearchProvider::sResultCountLimit = 50;


KadasWorldLocationSearchProvider::KadasWorldLocationSearchProvider( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  mCategoryMap.insert( "geonames", qMakePair( tr( "World Places" ), 30 ) );
}

QgsLocatorFilter *KadasWorldLocationSearchProvider::clone() const
{
  return new KadasWorldLocationSearchProvider( mMapCanvas );
}

void KadasWorldLocationSearchProvider::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  if ( string.length() < 3 )
    return;

  QString serviceUrl;
  if ( QgsSettings().value( "/kadas/isOffline" ).toBool() )
  {
    serviceUrl = QgsSettings().value( "search/worldlocationofflinesearchurl", "http://localhost:5000/SearchServerWld" ).toString();
  }
  else
  {
    serviceUrl = QgsSettings().value( "search/worldlocationsearchurl", "" ).toString();
  }

  // serviceUrl = "https://gist.githubusercontent.com/3nids/0fa5b42732df63f963111de7d0d9cd63/raw/4aa5024cf4ee9b664af8ec9135f7880a5c306f5b/search%2520kadas";

  if ( serviceUrl.isEmpty() )
    return;

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
  QgsNetworkReplyContent rep = QgsNetworkAccessManager::blockingGet( req, QString(), false, feedback );

  if ( rep.error() == QNetworkReply::NoError )
  {
    QByteArray replyText = rep.content();
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
      result.groupScore = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 1;
      QString label = itemAttrsMap["label"].toString();
      label.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
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
  else
  {
    QgsDebugMsgLevel( QString( "Could not fetch %1: %2" ).arg( url.toString(), rep.errorString() ), 1 );
  }
}

void KadasWorldLocationSearchProvider::triggerResult( const QgsLocatorResult &result )
{
  QgsCoordinateTransform mapCanvasTransform( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), mMapCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
  QgsCoordinateTransform annotationLayerTransform;
  if ( QgsProject::instance()->mainAnnotationLayer()->crs().isValid() && QgsProject::instance()->mainAnnotationLayer()->crs() != mMapCanvas->mapSettings().destinationCrs() )
  {
    annotationLayerTransform = QgsCoordinateTransform( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), QgsProject::instance()->mainAnnotationLayer()->crs(), QgsProject::instance() );
  }
  else
  {
    annotationLayerTransform = mapCanvasTransform;
  }

  QVariantMap data = result.userData().value<QVariantMap>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString geometry = data.value( QStringLiteral( "geometry" ) ).toString();
  QgsRectangle bbox = data.value( QStringLiteral( "bbox" ) ).value<QgsRectangle>();

  bool geomShown = false;
  if ( !geometry.isEmpty() )
  {
    QString feature = QString( "{\"type\": \"FeatureCollection\", \"features\": [{\"type\": \"Feature\", \"geometry\": %1}]}" ).arg( geometry );
    QgsFeatureList features = QgsJsonUtils::stringToFeatureList( feature );
    if ( !features.isEmpty() && !features[0].geometry().isEmpty() )
    {
      QgsGeometry geometry = features[0].geometry();
      geometry.transform( annotationLayerTransform );
      QgsAnnotationItem *item = nullptr;
      switch ( features[0].geometry().type() )
      {
        case Qgis::GeometryType::Point:
        {
          QgsPoint *pt = qgsgeometry_cast<QgsPoint *>( geometry.constGet() );
          item = new QgsAnnotationMarkerItem( *pt->clone() );

          QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
          symbolLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayer::VerticalAnchorPoint::Bottom );
          dynamic_cast<QgsAnnotationMarkerItem *>( item )->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
          break;
        }
        case Qgis::GeometryType::Line:
        {
          QgsCurve *curve = qgsgeometry_cast<QgsCurve *>( geometry.constGet() );
          item = new QgsAnnotationLineItem( curve->clone() );
          break;
        }
        case Qgis::GeometryType::Polygon:
        {
          QgsCurvePolygon *poly = nullptr;
          if ( geometry.isMultipart() )
          {
            QgsMultiSurface *ms = qgsgeometry_cast<QgsMultiSurface *>( geometry.constGet() );
            poly = qgsgeometry_cast<QgsCurvePolygon *>( ( ms )->geometryN( 0 ) )->clone();
          }
          else
          {
            poly = qgsgeometry_cast<QgsCurvePolygon *>( geometry.constGet() )->clone();
          }
          item = new QgsAnnotationPolygonItem( poly );


          dynamic_cast<QgsAnnotationPolygonItem *>( item )->setSymbol( KadasLocationSearchFilter::createPolygonSymbol() );
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

  QgsPointXY itemPos = annotationLayerTransform.transform( pos );
  QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( itemPos ) );
  QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
  symbolLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayer::VerticalAnchorPoint::Bottom );
  item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
  mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );

  if ( !bbox.isNull() )
  {
    bbox = mapCanvasTransform.transform( bbox );
    mMapCanvas->setExtent( bbox );
  }
  else
  {
    QgsPointXY mapPos = mapCanvasTransform.transform( pos );
    mMapCanvas->setCenter( mapPos );
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
