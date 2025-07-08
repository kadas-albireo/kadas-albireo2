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
#include <QElapsedTimer>
#include <QEventLoop>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsblockingnetworkrequest.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgscurve.h>
#include <qgis/qgscurvepolygon.h>
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
#include <qgis/qgssettings.h>
#include <qgis/qgsnetworkaccessmanager.h>

#include "kadas/gui/search/kadaslocationsearchprovider.h"


const int KadasLocationSearchFilter::sResultCountLimit = 50;


QgsFillSymbol *KadasLocationSearchFilter::createPolygonSymbol()
{
  QgsFillSymbolLayer *outline = new QgsSimpleFillSymbolLayer(
    QColor( 0, 0, 200, 200 ),
    Qt::BrushStyle::NoBrush,
    QColor( 0, 0, 200, 200 ),
    DEFAULT_SIMPLEFILL_BORDERSTYLE,
    .5 // border width
  );

  QgsLinePatternFillSymbolLayer *lineFill = new QgsLinePatternFillSymbolLayer();
  QgsSimpleLineSymbolLayer *simpleLine = new QgsSimpleLineSymbolLayer( QColor( 0, 0, 200, 200 ), 0.5 );
  lineFill->setSubSymbol( new QgsLineSymbol( { simpleLine } ) );
  lineFill->setDistance( 1.5 );
  lineFill->setLineAngle( 45 );

  return new QgsFillSymbol( { outline, lineFill } );
}

KadasLocationSearchFilter::KadasLocationSearchFilter( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
  setFetchResultsDelay( 300 );

  mCategoryMap.insert( "gg25", qMakePair( tr( "Municipalities" ), 26 ) );
  mCategoryMap.insert( "district", qMakePair( tr( "Districts" ), 25 ) );
  mCategoryMap.insert( "kantone", qMakePair( tr( "Cantons" ), 24 ) );
  mCategoryMap.insert( "sn25", qMakePair( tr( "Places" ), 23 ) );
  mCategoryMap.insert( "zipcode", qMakePair( tr( "Zip Codes" ), 23 ) );
  mCategoryMap.insert( "address", qMakePair( tr( "Address" ), 22 ) );
  mCategoryMap.insert( "gazetteer", qMakePair( tr( "General place name directory" ), 21 ) );
}

KadasLocationSearchFilter::~KadasLocationSearchFilter()
{
  if ( mCurrentReply )
  {
    mCurrentReply->abort();
    mCurrentReply->deleteLater();
  }
  if ( mEventLoop )
  {
    mEventLoop->quit();
    delete mEventLoop;
  }
}

QgsLocatorFilter *KadasLocationSearchFilter::clone() const
{
  return new KadasLocationSearchFilter( mMapCanvas );
}

void KadasLocationSearchFilter::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  if ( string.length() < 3 )
    return;

  if ( mCurrentReply )
  {
    mCurrentReply->abort();
    mCurrentReply->deleteLater();
    mCurrentReply = nullptr;
  }

  mFeedback = feedback;

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

  //QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  // we have performance issues with QgsNetworkAccessManager::instance()
  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  mCurrentReply = nam->get( req );

  mEventLoop = new QEventLoop;
  connect( mCurrentReply, &QNetworkReply::finished, this, &KadasLocationSearchFilter::handleNetworkReply );
  connect( feedback, &QgsFeedback::canceled, mEventLoop, [&]() {
    mCurrentReply->abort();
    mCurrentReply->deleteLater();
    mCurrentReply = nullptr;
    mEventLoop->quit();
  } );

  mEventLoop->exec();
  delete mEventLoop;
  mEventLoop = nullptr;
}

void KadasLocationSearchFilter::handleNetworkReply()
{
  if ( !mCurrentReply )
    return;

  QByteArray replyContent = mCurrentReply->readAll();
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson( replyContent, &err );
  if ( doc.isNull() )
    QgsDebugMsgLevel( QString( "Parsing error: %1" ).arg( err.errorString() ), 2 );

  QJsonObject resultMap = doc.object();
  const QJsonArray constResults = resultMap["results"].toArray();
  for ( const QJsonValue &item : constResults )
  {
    if ( mFeedback && mFeedback->isCanceled() )
    {
      mCurrentReply->deleteLater();
      mCurrentReply = nullptr;
      if ( mEventLoop )
        mEventLoop->quit();
      return;
    }

    QJsonObject itemMap = item.toObject();
    QJsonObject itemAttrsMap = itemMap["attrs"].toObject();

    QString origin = itemAttrsMap["origin"].toString();

    QgsLocatorResult result;
    result.group = mCategoryMap.contains( origin ) ? mCategoryMap[origin].first : origin;
    result.groupScore = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 0;
    result.displayString = itemAttrsMap["label"].toString().replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags

    QVariantMap resultData;
    const thread_local QRegularExpression bboxRe( R"(BOX\s*\(\s*(-?\d+\.?\d*)\s+(-?\d+\.?\d*)\s*,\s*(-?\d+\.?\d*)\s+(-?\d+\.?\d*)\s*)", QRegularExpression::CaseInsensitiveOption );
    const thread_local QRegularExpression patBoxRe( R"(^BOX\s*\(\s*(\d+\.?\d*)\s*(\d+\.?\d*)\s*,\s*(\d+\.?\d*)\s*(\d+\.?\d*)\s*\)$)" );
    if ( itemAttrsMap.contains( "geom_st_box2d" ) )
    {
      const QRegularExpressionMatch match = patBoxRe.match( itemAttrsMap["geom_st_box2d"].toString() );
      if ( match.hasMatch() )
      {
        resultData[QStringLiteral( "bbox" )] = QgsRectangle( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble(), match.captured( 3 ).toDouble(), match.captured( 4 ).toDouble() );
      }
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
      const QRegularExpressionMatch match = bboxRe.match( itemAttrsMap["boundingBox"].toString() );
      if ( match.isValid() )
      {
        resultData[QStringLiteral( "bbox" )] = QgsRectangle( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble(), match.captured( 3 ).toDouble(), match.captured( 4 ).toDouble() );
      }
    }
    result.setUserData( resultData );
    emit resultFetched( result );
  }
  mCurrentReply->deleteLater();
  mCurrentReply = nullptr;
  if ( mEventLoop )
    mEventLoop->quit();
}

void KadasLocationSearchFilter::triggerResult( const QgsLocatorResult &result )
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
  bool scaleOk = false;
  int scale = data.value( QStringLiteral( "scale" ) ).toInt( &scaleOk );


  QgsPointXY mapPos = mapCanvasTransform.transform( pos );
  QgsPointXY itemPos = annotationLayerTransform.transform( pos );

  QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( itemPos ) );
  QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
  symbolLayer->setVerticalAnchorPoint( Qgis::VerticalAnchorPoint::Bottom );
  item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
  mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );

  if ( !bbox.isEmpty() )
  {
    QgsRectangle zoomExtent = mapCanvasTransform.transform( bbox );
    zoomExtent.scale( 3 );
    mMapCanvas->setExtent( zoomExtent, true );
  }
  else
  {
    if ( scaleOk )
    {
      QgsRectangle zoomExtent = mMapCanvas->mapSettings().computeExtentForScale( mapPos, scale );
      mMapCanvas->setExtent( zoomExtent, true );
    }
    else
    {
      mMapCanvas->setCenter( mapPos );
    }
  }

  // not sure if we will get this from somewhere, it is not documented in the swisstopo API
  if ( !geometry.isEmpty() )
  {
    QString feature = QString( "{\"type\": \"FeatureCollection\", \"features\": [{\"type\": \"Feature\", \"geometry\": %1}]}" ).arg( geometry );
    QgsFeatureList features = QgsJsonUtils::stringToFeatureList( feature );
    if ( !features.isEmpty() && !features[0].geometry().isEmpty() )
    {
      QgsGeometry geometry = features[0].geometry();
      switch ( features[0].geometry().type() )
      {
        case Qgis::GeometryType::Point:
        {
          // points are already rendered
          break;
        }
        case Qgis::GeometryType::Line:
        {
          QgsCurve *curve = qgsgeometry_cast<QgsCurve *>( geometry.constGet()->clone() );
          QgsAnnotationLineItem *item = new QgsAnnotationLineItem( curve );
          mGeometryItemIds << QgsProject::instance()->mainAnnotationLayer()->addItem( item );
          break;
        }
        case Qgis::GeometryType::Polygon:
        {
          QgsCurvePolygon *poly = nullptr;
          if ( geometry.isMultipart() )
          {
            const QgsMultiSurface *ms = qgsgeometry_cast<const QgsMultiSurface *>( geometry.constGet() );
            for ( int p = 0; p < ms->numGeometries(); p++ )
            {
              poly = qgsgeometry_cast<QgsCurvePolygon *>( ms->geometryN( p )->clone() );
              poly->transform( annotationLayerTransform );
              QgsAnnotationPolygonItem *item = new QgsAnnotationPolygonItem( poly );
              item->setSymbol( createPolygonSymbol() );
              mGeometryItemIds << QgsProject::instance()->mainAnnotationLayer()->addItem( item );
            }
          }
          else
          {
            poly = qgsgeometry_cast<QgsCurvePolygon *>( geometry.constGet()->clone() );
            poly->transform( annotationLayerTransform );
            QgsAnnotationPolygonItem *item = new QgsAnnotationPolygonItem( poly );
            item->setSymbol( createPolygonSymbol() );
            mGeometryItemIds << QgsProject::instance()->mainAnnotationLayer()->addItem( item );
          }
          break;
        }
        case Qgis::GeometryType::Unknown:
        case Qgis::GeometryType::Null:
          break;
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
  for ( const QString &itemId : std::as_const( mGeometryItemIds ) )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( itemId );
  }
  mGeometryItemIds.clear();
}
