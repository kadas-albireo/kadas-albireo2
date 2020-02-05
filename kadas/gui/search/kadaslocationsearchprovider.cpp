/***************************************************************************
    kadaslocationsearchprovider.cpp
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/search/kadaslocationsearchprovider.h>

const int KadasLocationSearchProvider::sSearchTimeout = 2000;
const int KadasLocationSearchProvider::sResultCountLimit = 50;


KadasLocationSearchProvider::KadasLocationSearchProvider( QgsMapCanvas *mapCanvas )
  : KadasSearchProvider( mapCanvas )
{
  mNetReply = 0;

  mCategoryMap.insert( "gg25", qMakePair( tr( "Municipalities" ), 20 ) );
  mCategoryMap.insert( "kantone", qMakePair( tr( "Cantons" ), 21 ) );
  mCategoryMap.insert( "district", qMakePair( tr( "Districts" ), 22 ) );
  mCategoryMap.insert( "sn25", qMakePair( tr( "Places" ), 23 ) );
  mCategoryMap.insert( "zipcode", qMakePair( tr( "Zip Codes" ), 24 ) );
  mCategoryMap.insert( "address", qMakePair( tr( "Address" ), 25 ) );
  mCategoryMap.insert( "gazetteer", qMakePair( tr( "General place name directory" ), 26 ) );

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, &QTimer::timeout, this, &KadasLocationSearchProvider::replyFinished );
}

void KadasLocationSearchProvider::startSearch( const QString &searchtext, const SearchRegion & /*searchRegion*/ )
{
  QString serviceUrl;
  if ( QgsSettings().value( "/Qgis/isOffline" ).toBool() )
  {
    serviceUrl = QgsSettings().value( "search/locationofflinesearchurl", "http://localhost:5000/SearchServerCh" ).toString();
  }
  else
  {
    serviceUrl = QgsSettings().value( "search/locationsearchurl", "https://api3.geo.admin.ch/rest/services/api/SearchServer" ).toString();
  }
  QgsDebugMsg( serviceUrl );

  QUrl url( serviceUrl );
  QUrlQuery query( serviceUrl );
  query.addQueryItem( "type", "locations" );
  query.addQueryItem( "searchText", searchtext );
  query.addQueryItem( "sr", "4326" );
  query.addQueryItem( "limit", QString::number( sResultCountLimit ) );
  url.setQuery( query );

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, &QNetworkReply::finished, this, &KadasLocationSearchProvider::replyFinished );
  mTimeoutTimer.start( sSearchTimeout );
}

void KadasLocationSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, &QNetworkReply::finished, this, &KadasLocationSearchProvider::replyFinished );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
  }
}

void KadasLocationSearchProvider::replyFinished()
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
  QVariantMap resultMap = doc.object().toVariantMap();
  bool fuzzy = resultMap["fuzzy"] == "true";
  for ( const QVariant &item : resultMap["results"].toList() )
  {
    QVariantMap itemMap = item.toMap();
    QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

    QString origin = itemAttrsMap["origin"].toString();

    SearchResult searchResult;
    if ( mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                        mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    }
    // When bbox is empty, fallback to pos + zoomScale is used
    searchResult.pos = QgsPointXY( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
    searchResult.zoomScale = origin == "address" ? 5000 : 25000;

    searchResult.category = mCategoryMap.contains( origin ) ? mCategoryMap[origin].first : origin;
    searchResult.categoryPrecedence = mCategoryMap.contains( origin ) ? mCategoryMap[origin].second : 100;
    searchResult.text = itemAttrsMap["label"].toString();
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" );   // Remove HTML tags
    searchResult.crs = "EPSG:4326";
    searchResult.showPin = true;
    searchResult.fuzzy = fuzzy;
    emit searchResultFound( searchResult );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  emit searchFinished();
}
