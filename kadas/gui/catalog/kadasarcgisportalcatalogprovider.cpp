/***************************************************************************
    kadasarcgisportalcatalogprovider.cpp
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

#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsmimedatautils.h>
#include <qgis/qgssettings.h>

#include <kadas/core/kadas.h>
#include <kadas/gui/kadascatalogbrowser.h>
#include <kadas/gui/catalog/kadasarcgisportalcatalogprovider.h>

KadasArcGisPortalCatalogProvider::KadasArcGisPortalCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> &params )
  : KadasCatalogProvider( browser ), mBaseUrl( baseUrl ), mServicePreference( params.value( "preferred", "wms" ) )
{
  QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toLower();
  QFile isoTopics( QDir( Kadas::pkgDataPath() ).absoluteFilePath( QString( "catalog/isoTopics_%1.csv" ).arg( lang ) ) );
  if ( isoTopics.open( QIODevice::ReadOnly ) )
  {
    QStringList lines = QString::fromUtf8( isoTopics.readAll() ).replace( "\r", "" ).split( "\n" );
    for ( int line = 1, nLines = lines.size(); line < nLines; ++line ) // Skip first line (header)
    {
      QStringList fields = lines[line].split( ";" );
      if ( fields.length() >= 4 )
      {
        mIsoTopics.insert( fields[0], {fields[1] + "/" + fields[2], fields[3]} );
      }
    }
  }
}

void KadasArcGisPortalCatalogProvider::load()
{
  mAmsLayers.clear();
  mWmtsLayers.clear();
  mWmsLayers.clear();
  mAmsLayerIds.clear();
  mWmsLayerIds.clear();

  mPendingTasks = 1;
  QUrl url( mBaseUrl );
  QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toUpper();
  QUrlQuery query( url );
  query.addQueryItem( "lang", lang );
  query.addQueryItem( "timestamp", QString::number( QDateTime::currentSecsSinceEpoch() ) );
  url.setQuery( query );
  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::replyFinished );
}

void KadasArcGisPortalCatalogProvider::replyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  QUrl reqUrl = reply->request().url();
  QString portalBaseUrl = reqUrl.scheme() + "://" + reqUrl.authority();
  bool lastRequest = true;
  QString nextStart;
  QString num;

  if ( reply->error() == QNetworkReply::NoError )
  {
    QList<QByteArray> setCookieFields = reply->rawHeader( "Set-Cookie" ).split( ';' );
    QgsDebugMsg( QString( "Set-Cookie header: %1" ).arg( QString::fromUtf8( reply->rawHeader( "Set-Cookie" ) ) ) );
    if ( setCookieFields.length() > 0 && setCookieFields[0].startsWith( "esri_auth=" ) )
    {
      QJsonDocument esriAuth = QJsonDocument::fromJson( QUrl::fromPercentEncoding( setCookieFields[0] ).toUtf8().mid( 10 ) );
      QString username = esriAuth.object()["email"].toString().replace( QRegExp( "@.*$" ), "" );
      QgsDebugMsg( QString( "Extracted username from Set-Cookie: %1" ).arg( username ) );
      emit userChanged( username );

      QString token = esriAuth.object()["token"].toString();
      QgsDebugMsg( QString( "Extracted token from Set-Cookie: %1" ).arg( token ) );
      if ( !token.isEmpty() )
      {
        QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
        QString cookie = QString( "esri_auth=\"token\": \"%1\"" ).arg( token );
        QStringList cookieUrls = QgsSettings().value( "/iamauth/cookieurls", "" ).toString().split( ";" );
        for ( const QString &url : cookieUrls )
        {
          QgsDebugMsg( QString( "Setting cookie for url %1: %2" ).arg( url, cookie ) );
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url );
        }
      }
    }

    QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();

    for ( const QVariant &resultData : rootMap["results"].toList() )
    {
      QVariantMap resultMap = resultData.toMap();
      QString category;
      QString position;
      QString categoryId;
      for ( const QVariant &tagv : resultMap["tags"].toList() )
      {
        QString tag = tagv.toString();
        if ( tag.startsWith( "milcatalog:", Qt::CaseInsensitive ) )
        {
          categoryId = tag;
          auto it = mIsoTopics.find( tag.mid( 11 ).toUpper() );
          if ( it != mIsoTopics.end() )
          {
            category = it.value().category;
            position = it.value().sortIndices;
          }
          break;
        }
      }

      QString metadataUrl = QgsSettings().value( "kadas/metadataBaseUrl" ).toString().arg( resultMap["id"].toString() );
      bool flatten = false;
      if ( resultMap["type"].toString() == "Map Service" )
      {
        mAmsLayerIds.insert( categoryId + ":" + resultMap["title"].toString(), qMakePair( resultMap["url"].toString(), resultMap["id"].toString() ) );
        mAmsLayers[resultMap["url"].toString()].insert( resultMap["id"].toString(), ResultEntry( category, resultMap["title"].toString(), position, metadataUrl, flatten ) );
      }
      else if ( resultMap["type"].toString() == "WMS" )
      {
        mWmsLayerIds.insert( categoryId + ":" + resultMap["title"].toString(), qMakePair( resultMap["url"].toString(), resultMap["id"].toString() ) );
        mWmsLayers[resultMap["url"].toString()].insert( resultMap["id"].toString(), ResultEntry( category, resultMap["title"].toString(), position, metadataUrl, flatten ) );
      }
//      else if( resultMap["type"].toString() == "WMTS" )
//      {
//        wmtsLayers[resultMap["url"].toString()].insert( resultMap["id"].toString(), ResultEntry( category, resultMap["title"].toString(), position, metadataUrl, flatten ) );
//      }
    }

    if ( rootMap["nextStart"].toInt() >= 0 )
    {
      lastRequest = false;
      nextStart = rootMap["nextStart"].toString();
      num = rootMap["num"].toString();
    }
  }
  reply->deleteLater();
  if ( lastRequest )
  {
    // Prevent adding same layer (as identified by the milcatalog tag + title) twice in different service variants (WMS or MapServer). Preference according to config.
    if ( mServicePreference == "wms" )
    {
      for ( auto it = mAmsLayerIds.begin(), itEnd = mAmsLayerIds.end(); it != itEnd; ++it )
      {
        if ( mWmsLayerIds.contains( it.key() ) )
        {
          mAmsLayers[it.value().first].remove( it.value().second );
          if ( mAmsLayers[it.value().first].isEmpty() )
          {
            mAmsLayers.remove( it.value().first );
          }
        }
      }
    }
    else if ( mServicePreference == "arcgismapserver" )
    {
      for ( auto it = mWmsLayerIds.begin(), itEnd = mWmsLayerIds.end(); it != itEnd; ++it )
      {
        if ( mAmsLayerIds.contains( it.key() ) )
        {
          mWmsLayers[it.value().first].remove( it.value().second );
          if ( mWmsLayers[it.value().first].isEmpty() )
          {
            mWmsLayers.remove( it.value().first );
          }
        }
      }
    }

    for ( const QString &amsUrl : mAmsLayers.keys() )
    {
      readAMSCapabilities( amsUrl, mAmsLayers[amsUrl] );
    }
    for ( const QString &wmsUrl : mWmsLayers.keys() )
    {
      readWMSCapabilities( wmsUrl, mWmsLayers[wmsUrl] );
    }
    //    for ( const QString &wmtsUrl : wmtsLayers.keys() )
    //    {
    //      readWMTSCapabilities( wmtsUrl, wmtsLayers[wmtsUrl] );
    //    }

    mAmsLayers.clear();
    mWmtsLayers.clear();
    mWmsLayers.clear();
    mAmsLayerIds.clear();
    mWmsLayerIds.clear();

    endTask();
  }
  else
  {
    QUrlQuery query( reqUrl );
    query.removeAllQueryItems( "start" );
    query.removeAllQueryItems( "num" );
    query.addQueryItem( "start", nextStart );
    query.addQueryItem( "num", num );
    reqUrl.setQuery( query );
    QNetworkRequest req( reqUrl );
    req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::replyFinished );
  }
}

void KadasArcGisPortalCatalogProvider::endTask()
{
  mPendingTasks -= 1;
  if ( mPendingTasks == 0 )
  {
    emit finished();
  }
}

void KadasArcGisPortalCatalogProvider::readWMTSCapabilities( const QString &wmtsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( ( QUrl( wmtsUrl ) ) );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMTSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readWMTSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  reply->deleteLater();
  EntryMap *entries = reinterpret_cast<EntryMap *>( reply->property( "entries" ).value<void *>() );
  QString referer = QgsSettings().value( "search/referer", "http://localhost" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    if ( !doc.isNull() )
    {
      QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );
      for ( const QDomNode &layerItem : childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" ) )
      {
        QString layerid = layerItem.firstChildElement( "ows:Identifier" ).text();
        if ( entries->contains( layerid ) )
        {
          QString title;
          QMimeData *mimeData;
          const ResultEntry &entry = ( *entries ) [layerid];
          parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, reply->request().url().toString(), entry.metadataUrl, QString( "&referer=%1" ).arg( referer ), title, layerid, mimeData );
          QStringList sortIndices = entry.sortIndices.split( "/" );
          mBrowser->addItem( getCategoryItem( entry.category.split( "/" ), sortIndices ), entry.title, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
        }
      }
    }
  }

  delete entries;
  endTask();
}

void KadasArcGisPortalCatalogProvider::readWMSCapabilities( const QString &wmsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( wmsUrl + "?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0" ) );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "url", wmsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readWMSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  reply->deleteLater();
  EntryMap *entries = reinterpret_cast<EntryMap *>( reply->property( "entries" ).value<void *>() );
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QStringList imgFormats = parseWMSFormats( doc );
    QStringList parentCrs;

    QDomElement layerItem = doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ).firstChildElement( "Layer" );
    QString layerName = layerItem.firstChildElement( "Name" ).text();

    QString title;
    QMimeData *mimeData;
    // Assume entries contains just one entry (every WMS service just referenced by one item in the portal)
    const ResultEntry &entry = entries->first();

    if ( parseWMSLayerCapabilities( layerItem, entry.title, imgFormats, parentCrs, url, entry.metadataUrl, mimeData ) )
    {

      // Parse sublayers
      QVariantList sublayers;
      readWMSSublayers( layerItem, "-1", sublayers );
      mimeData->setProperty( "sublayers", sublayers );
      QStringList sortIndices = entry.sortIndices.split( "/" );
      mBrowser->addItem( getCategoryItem( entry.category.split( "/" ), sortIndices ), entry.title, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
    }
  }

  delete entries;
  endTask();
}

void KadasArcGisPortalCatalogProvider::readWMSSublayers( const QDomElement &layerItem, const QString &parentName, QVariantList &sublayers )
{
  for ( const QDomNode &subLayerItem : childrenByTagName( layerItem, "Layer" ) )
  {
    QVariantMap sublayer;
    QString layerId = subLayerItem.firstChildElement( "Name" ).text();
    sublayer["id"] = layerId;
    sublayer["parentLayerId"] = parentName;
    sublayer["name"] = subLayerItem.firstChildElement( "Title" ).text();
    sublayers.append( sublayer );
    readWMSSublayers( subLayerItem.toElement(), layerId, sublayers );
  }
}

void KadasArcGisPortalCatalogProvider::readAMSCapabilities( const QString &amsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  QUrl url( amsUrl + "?f=json" );

  QNetworkRequest req( url );
  QNetworkReply *reply = nam->get( req );
  reply->setProperty( "url", amsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readAMSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readAMSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  reply->deleteLater();
  EntryMap *entries = reinterpret_cast<EntryMap *>( reply->property( "entries" ).value<void *>() );
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QVariantMap serviceInfoMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();

    if ( !serviceInfoMap["error"].isNull() )
    {
      // Something went wrong
      delete entries;
      endTask();
      return;
    }

    // Parse spatial reference
    QVariantMap spatialReferenceMap = serviceInfoMap["spatialReference"].toMap();
    QString spatialReference = spatialReferenceMap["latestWkid"].toString();
    if ( spatialReference.isEmpty() )
    {
      spatialReference = spatialReferenceMap["wkid"].toString();
    }
    if ( spatialReference.isEmpty() )
    {
      spatialReference = spatialReferenceMap["wkt"].toString();
    }
    else
    {
      spatialReference = QString( "EPSG:%1" ).arg( spatialReference );
    }
    QgsCoordinateReferenceSystem crs;
    crs.createFromString( spatialReference );
    if ( crs.authid().startsWith( "USER:" ) )
    {
      crs.createFromString( "EPSG:4326" );    // If we can't recognize the SRS, fall back to WGS84
    }

    // Parse formats
    QSet<QString> filteredEncodings;
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    for ( const QString &encoding : serviceInfoMap["supportedImageFormatTypes"].toString().split( "," ) )
    {
      for ( const QByteArray &fmt : supportedFormats )
      {
        if ( encoding.startsWith( fmt, Qt::CaseInsensitive ) )
        {
          filteredEncodings.insert( encoding.toLower() );
        }
      }
    }

    // Parse sublayers
    QList<QVariantMap> sublayers;
    for ( QVariant variant : serviceInfoMap["layers"].toList() )
    {
      QVariantMap entry = variant.toMap();
      QVariantMap sublayer;
      sublayer["id"] = entry["id"];
      sublayer["parentLayerId"] = entry["parentLayerId"];
      sublayer["name"] = entry["name"];
      sublayers.append( sublayer );
    }

    for ( const QString &layerName : entries->keys() )
    {
      QgsMimeDataUtils::Uri mimeDataUri;
      mimeDataUri.layerType = "raster";
      mimeDataUri.providerKey = "arcgismapserver";
      const ResultEntry &entry = ( *entries ) [layerName];
      mimeDataUri.name = entry.title;
      QString format = filteredEncodings.isEmpty() || filteredEncodings.contains( "png" ) ? "png" : filteredEncodings.values().front();
      mimeDataUri.uri = QString( "crs='%1' format='%2' url='%3' layer='%4'" ).arg( crs.authid() ).arg( format ).arg( url ).arg( layerName );
      QMimeData *mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
      mimeData->setProperty( "metadataUrl", entry.metadataUrl );
      if ( !entry.flatten )
      {
        QVariantList entrySublayers;
        for ( const QVariantMap &sublayer : sublayers )
        {
          if ( sublayer["id"].toInt() >= ( layerName.isEmpty() ? -1 : layerName.toInt() ) )
          {
            entrySublayers.append( sublayer );
          }
        }
        mimeData->setProperty( "sublayers", entrySublayers );
      }
      QStringList sortIndices = entry.sortIndices.split( "/" );
      mBrowser->addItem( getCategoryItem( entry.category.split( "/" ), sortIndices ), mimeDataUri.name, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
    }
  }

  delete entries;
  endTask();
}
