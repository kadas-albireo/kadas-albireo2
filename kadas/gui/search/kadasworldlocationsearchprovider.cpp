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

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/search/kadasworldlocationsearchprovider.h>


const int KadasWorldLocationSearchProvider::sSearchTimeout = 2000;
const int KadasWorldLocationSearchProvider::sResultCountLimit = 50;


KadasWorldLocationSearchProvider::KadasWorldLocationSearchProvider( QgsMapCanvas *mapCanvas )
  : KadasSearchProvider( mapCanvas )
{
  mNetReply = 0;

  mCategoryMap.insert( "geonames", qMakePair( tr( "World Places" ), 30 ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, &QTimer::timeout, this, &KadasWorldLocationSearchProvider::replyFinished );
}

void KadasWorldLocationSearchProvider::startSearch( const QString &searchtext, const SearchRegion & /*searchRegion*/ )
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
  query.addQueryItem( "searchText", searchtext );
  query.addQueryItem( "sr", "4326" );
  if ( !query.hasQueryItem( "limit" ) )
  {
    query.addQueryItem( "limit", QString::number( sResultCountLimit ) );
  }
  url.setQuery( query );

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, &QNetworkReply::finished, this, &KadasWorldLocationSearchProvider::replyFinished );
  mTimeoutTimer.start( sSearchTimeout );
}

void KadasWorldLocationSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, &QNetworkReply::finished, this, &KadasWorldLocationSearchProvider::replyFinished );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
  }
}

void KadasWorldLocationSearchProvider::replyFinished()
{
  if ( !mNetReply )
  {
    return;
  }

  if ( mNetReply->error() != QNetworkReply::NoError || !mTimeoutTimer.isActive() )
  {
    mNetReply->deleteLater();
    mNetReply = 0;
    emit searchFinished();
    return;
  }

  QByteArray replyText = mNetReply->readAll();
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson( replyText, &err );
  if ( doc.isNull() )
  {
    QgsDebugMsg( QString( "Parsing error:" ).arg( err.errorString() ) );
  }
  QJsonObject resultMap = doc.object();
  for ( const QJsonValueRef &item : resultMap["results"].toArray() )
  {
    QJsonObject itemMap = item.toObject();
    QJsonObject itemAttrsMap = itemMap["attrs"].toObject();


    QString origin = itemAttrsMap["origin"].toString();


    SearchResult searchResult;
    searchResult.pos = QgsPointXY( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
    searchResult.zoomScale = 25000;

    searchResult.category = mCategoryMap.contains( origin ) ? mCategoryMap[origin].first : origin;
    searchResult.categoryPrecedence = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 100;
    searchResult.text = itemAttrsMap["label"].toString();
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" );   // Remove HTML tags
    searchResult.crs = "EPSG:4326";
    searchResult.showPin = !itemAttrsMap.contains( "geometryGeoJSON" );
    if ( itemAttrsMap.contains( "geometryGeoJSON" ) )
    {
      searchResult.geometry = QJsonDocument( itemAttrsMap["geometryGeoJSON"].toObject() ).toJson( QJsonDocument::Compact );
    }
    emit searchResultFound( searchResult );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  emit searchFinished();
}
