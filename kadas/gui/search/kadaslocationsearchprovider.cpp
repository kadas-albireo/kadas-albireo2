/***************************************************************************
    KadasLocationSearchFilter.cpp
    -------------------------------
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
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsblockingnetworkrequest.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsjsonutils.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgscurve.h>
#include <qgis/qgscurvepolygon.h>

#include "kadas/gui/search/kadaslocationsearchprovider.h"


const int KadasLocationSearchFilter::sSearchTimeout = 10000;
const int KadasLocationSearchFilter::sResultCountLimit = 50;


KadasLocationSearchFilter::KadasLocationSearchFilter( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  mCategoryMap.insert( "gg25", qMakePair( tr( "Municipalities" ), 20 ) );
  mCategoryMap.insert( "kantone", qMakePair( tr( "Cantons" ), 21 ) );
  mCategoryMap.insert( "district", qMakePair( tr( "Districts" ), 22 ) );
  mCategoryMap.insert( "sn25", qMakePair( tr( "Places" ), 23 ) );
  mCategoryMap.insert( "zipcode", qMakePair( tr( "Zip Codes" ), 24 ) );
  mCategoryMap.insert( "address", qMakePair( tr( "Address" ), 25 ) );
  mCategoryMap.insert( "gazetteer", qMakePair( tr( "General place name directory" ), 26 ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );
}

KadasLocationSearchFilter::~KadasLocationSearchFilter()
{
}

QgsLocatorFilter *KadasLocationSearchFilter::clone() const
{
  return new KadasLocationSearchFilter( mMapCanvas );
}

void KadasLocationSearchFilter::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  if ( string.length() < 3 )
    return;

  QString serviceUrl;
  if ( QgsSettings().value( "/kadas/isOffline" ).toBool() )
  {
    serviceUrl = QgsSettings().value( "search/locationofflinesearchurl", "http://localhost:5000/SearchServerCh" ).toString();
  }
  else
  {
    serviceUrl = QgsSettings().value( "search/locationsearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString();
  }
  QgsDebugMsgLevel( serviceUrl, 2 );

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


  QgsBlockingNetworkRequest bnr = QgsBlockingNetworkRequest();

  connect( feedback, &QgsFeedback::canceled, &bnr, &QgsBlockingNetworkRequest::abort );
  QgsBlockingNetworkRequest::ErrorCode errCode = bnr.get( req, false, feedback );
  if ( errCode != QgsBlockingNetworkRequest::NoError )
    return;

  QgsNetworkReplyContent reply = bnr.reply();
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson( reply.content(), &err );
  if ( doc.isNull() )
    QgsDebugMsgLevel( QString( "Parsing error:" ).arg( err.errorString() ), 2 );

  QJsonObject resultMap = doc.object();
  //bool fuzzy = resultMap["fuzzy"] == "true";
  const QJsonArray constResults = resultMap["results"].toArray();
  for ( const QJsonValue &item : constResults )
  {
    QJsonObject itemMap = item.toObject();
    QJsonObject itemAttrsMap = itemMap["attrs"].toObject();

    QString origin = itemAttrsMap["origin"].toString();

    QgsLocatorResult result;
    result.group = mCategoryMap.contains( origin ) ? mCategoryMap[origin].first : origin;
    //TODO when QGIS 3.40
    //result.groupScore = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 0;
    result.displayString = itemAttrsMap["label"].toString().replace( QRegExp( "<[^>]+>" ), "" );   // Remove HTML tags

    QVariantMap resultData;
    if ( mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      resultData[QStringLiteral( "bbox" )] = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                             mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    }
    // When bbox is empty, fallback to pos + zoomScale is used
    resultData[QStringLiteral( "pos" )] = QgsPointXY( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
    resultData[QStringLiteral( "zoomScale" )] = origin == "address" ? 5000 : 25000;
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

void KadasLocationSearchFilter::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString geometry = data.value( QStringLiteral( "geometry" ) ).toString();

  QgsPointXY itemPos = QgsCoordinateTransform(
                         QgsCoordinateReferenceSystem::fromOgcWmsCrs( QStringLiteral( "EPSG:4326" ) ),
                         mMapCanvas->mapSettings().destinationCrs(),
                         QgsProject::instance()
                       ).transform( pos );

  QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( itemPos ) );
  QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
  symbolLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayer::VerticalAnchorPoint::Bottom );
  item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
  mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );

  mMapCanvas->setCenter( itemPos );

  // not sure if we will get this from somewhere, it is not documented in the swisstopo API
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
        mGeometryItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );
      }
    }
  }
}


void KadasLocationSearchFilter::clearPreviousResults()
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
