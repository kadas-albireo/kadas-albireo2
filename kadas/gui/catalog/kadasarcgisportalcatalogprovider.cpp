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

#include "kadas/core/kadas.h"
#include "kadas/gui/kadascatalogbrowser.h"
#include "kadas/gui/catalog/kadasarcgisportalcatalogprovider.h"

KadasArcGisPortalCatalogProvider::KadasArcGisPortalCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> &params )
  : KadasCatalogProvider( browser ), mBaseUrl( baseUrl ), mServicePreference( params.value( "preferred", "wms" ) ), mCatalogTag( params.value( "tag", "milcatalog" ) )
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
        mIsoTopics.insert( fields[0], { fields[1] + "/" + fields[2], fields[3] } );
      }
    }
  }
}

void KadasArcGisPortalCatalogProvider::load()
{
  mLayers.clear();

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
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  QUrl reqUrl = reply->request().url();
  QString detailBaseUrl = reqUrl.toString().replace( QRegularExpression( "/rest/search/?\?.*$" ), "/rest/content/items/" );
  bool lastRequest = true;
  QString nextStart;
  QString num;

  if ( reply->error() == QNetworkReply::NoError )
  {
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
        if ( tag.startsWith( mCatalogTag + ":", Qt::CaseInsensitive ) )
        {
          categoryId = tag;
          auto it = mIsoTopics.find( tag.mid( mCatalogTag.length() + 1 ).toUpper() );
          if ( it != mIsoTopics.end() )
          {
            category = it.value().category;
            position = it.value().sortIndices;
          }
          break;
        }
      }

      bool flatten = false;
      QString detailUrl = QString( "%1%2/data?f=json" ).arg( detailBaseUrl, resultMap["id"].toString() );
      ResultEntry entry( resultMap["url"].toString(), resultMap["id"].toString(), category, resultMap["title"].toString(), position, "", detailUrl, flatten );
      QString id = categoryId + ":" + resultMap["title"].toString();
      mLayers[id] = mLayers.value( id );
      mLayers[id][resultMap["type"].toString().toLower()] = entry;
    }

    if ( rootMap["nextStart"].toInt() >= 0 && rootMap["num"].toInt() >= 0 )
    {
      lastRequest = false;
      nextStart = rootMap["nextStart"].toString();
      num = rootMap["num"].toString();
    }
  }
  reply->deleteLater();
  if ( lastRequest )
  {
    QStringList typeOrder = { "vector tile service", "wmts", "map service", "wms" };
    int pos = 0;
    for ( const QString &entry : mServicePreference.split( "," ) )
    {
      int index = typeOrder.indexOf( entry.toLower() );
      if ( index != -1 )
      {
        typeOrder.removeAt( index );
        typeOrder.insert( pos, entry.toLower() );
      }
      ++pos;
    }
    QMap<QString, std::function<void( const ResultEntry & )>> typeHandlers;
    typeHandlers.insert( "vector tile service", [this]( const ResultEntry &entry ) { addVTSlayer( entry ); } );
    typeHandlers.insert( "map service", [this]( const ResultEntry &entry ) { readAMSCapabilities( entry ); } );
    typeHandlers.insert( "wmts", [this]( const ResultEntry &entry ) { readWMTSDetail( entry ); } );
    typeHandlers.insert( "wms", [this]( const ResultEntry &entry ) { readWMSDetail( entry ); } );

    for ( const auto &layerTypeMap : std::as_const( mLayers ) )
    {
      for ( const QString &type : typeOrder )
      {
        if ( layerTypeMap.contains( type ) )
        {
          const ResultEntry &entry = layerTypeMap[type];
          typeHandlers[type]( entry );
          break;
        }
      }
    }

    mLayers.clear();

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

void KadasArcGisPortalCatalogProvider::readWMTSDetail( const ResultEntry &entry )
{
  mPendingTasks += 1;
  QNetworkRequest req( entry.detailUrl );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "entry", QVariant::fromValue<void *>( reinterpret_cast<void *>( new ResultEntry( entry ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMTSCapabilities );
}

void KadasArcGisPortalCatalogProvider::readWMTSCapabilities()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  ResultEntry *entry = reinterpret_cast<ResultEntry *>( reply->property( "entry" ).value<void *>() );
  QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
  QString layerIdentifier = rootMap["wmtsInfo"].toMap()["layerIdentifier"].toString();

  QString WMTSCapUrl = QString( entry->url ).replace( QRegularExpression( "/WMTS/.*$" ), "/WMTS/WMTSCapabilities.xml" );
  QNetworkRequest req( ( QUrl( WMTSCapUrl ) ) );
  QNetworkReply *capReply = QgsNetworkAccessManager::instance()->get( req );
  capReply->setProperty( "entry", QVariant::fromValue<void *>( reinterpret_cast<void *>( entry ) ) );
  capReply->setProperty( "layeridentifier", layerIdentifier );
  QgsDebugMsgLevel( QString( "Reading WMTS capabilities %1 for layer %2" ).arg( WMTSCapUrl ).arg( layerIdentifier ), 2 );
  connect( capReply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMTSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readWMTSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
  ResultEntry *entry = reinterpret_cast<ResultEntry *>( reply->property( "entry" ).value<void *>() );
  QString layerIdentifier = reply->property( "layeridentifier" ).toString();
  QString referer = QgsSettings().value( "search/referer", "http://localhost" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    if ( !doc.isNull() )
    {
      QDomNodeList layerItems = doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ).elementsByTagName( "Layer" );
      QDomElement layerItem;
      for ( int i = 0, n = layerItems.size(); i < n; ++i )
      {
        if ( layerItems.at( i ).firstChildElement( "ows:Identifier" ).text() == layerIdentifier )
        {
          layerItem = layerItems.at( i ).toElement();
        }
      }
      if ( !layerItem.isNull() )
      {
        QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );
        QMimeData *mimeData = nullptr;
        parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, reply->request().url().toString(), "", QString( "&referer=%1" ).arg( referer ), entry->title, layerIdentifier, mimeData );
        QStringList sortIndices = entry->sortIndices.split( "/" );
        mBrowser->addItem( getCategoryItem( entry->category.split( "/" ), sortIndices ), entry->title, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
      }
    }
  }

  delete entry;
  endTask();
}

void KadasArcGisPortalCatalogProvider::readWMSDetail( const ResultEntry &entry )
{
  mPendingTasks += 1;
  QNetworkRequest req( entry.detailUrl );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "entry", QVariant::fromValue<void *>( reinterpret_cast<void *>( new ResultEntry( entry ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMSCapabilities );
}

void KadasArcGisPortalCatalogProvider::readWMSCapabilities()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  ResultEntry *entry = reinterpret_cast<ResultEntry *>( reply->property( "entry" ).value<void *>() );
  QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
  QVariantList layers = rootMap["layers"].toList();
  QString layerName;
  if ( !layers.isEmpty() )
  {
    layerName = layers[0].toMap()["name"].toString();
  }

  QNetworkRequest req( QUrl( entry->url + "?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0" ) );
  QNetworkReply *capReply = QgsNetworkAccessManager::instance()->get( req );
  capReply->setProperty( "entry", QVariant::fromValue<void *>( reinterpret_cast<void *>( entry ) ) );
  capReply->setProperty( "layername", layerName );
  QgsDebugMsgLevel( QString( "Reading WMS capabilities %1 for layer %2" ).arg( req.url().toString() ).arg( layerName ), 2 );
  connect( capReply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readWMSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readWMSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
  ResultEntry *entry = reinterpret_cast<ResultEntry *>( reply->property( "entry" ).value<void *>() );
  QString layerName = reply->property( "layername" ).toString();
  QString url = entry->url;

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QStringList imgFormats = parseWMSFormats( doc );
    QStringList parentCrs;

    QDomElement layerItem;
    if ( layerName.isEmpty() )
    {
      layerItem = doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ).firstChildElement( "Layer" );
      layerName = layerItem.firstChildElement( "Name" ).text();
    }
    else
    {
      QDomNodeList layerItems = doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ).elementsByTagName( "Layer" );
      for ( int i = 0, n = layerItems.size(); i < n; ++i )
      {
        if ( layerItems.at( i ).firstChildElement( "Name" ).text() == layerName )
        {
          layerItem = layerItems.at( i ).toElement();
        }
      }
    }

    QMimeData *mimeData = nullptr;
    if ( !layerItem.isNull() && parseWMSLayerCapabilities( layerItem, entry->title, imgFormats, parentCrs, url, "", mimeData ) )
    {
      // Parse sublayers
      QVariantList sublayers;
      readWMSSublayers( layerItem, "-1", sublayers );
      mimeData->setProperty( "sublayers", sublayers );
      QStringList sortIndices = entry->sortIndices.split( "/" );
      mBrowser->addItem( getCategoryItem( entry->category.split( "/" ), sortIndices ), entry->title, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
    }
  }

  delete entry;
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
    sublayers.prepend( sublayer );
    readWMSSublayers( subLayerItem.toElement(), layerId, sublayers );
  }
}

void KadasArcGisPortalCatalogProvider::readAMSCapabilities( const ResultEntry &entry )
{
  mPendingTasks += 1;
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  QUrl url( entry.url + "?f=json" );

  QNetworkRequest req( url );
  QNetworkReply *reply = nam->get( req );
  reply->setProperty( "entry", QVariant::fromValue<void *>( reinterpret_cast<void *>( new ResultEntry( entry ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisPortalCatalogProvider::readAMSCapabilitiesDo );
}

void KadasArcGisPortalCatalogProvider::readAMSCapabilitiesDo()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
  ResultEntry *entry = reinterpret_cast<ResultEntry *>( reply->property( "entry" ).value<void *>() );
  QString url = entry->url;

  if ( reply->error() == QNetworkReply::NoError )
  {
    QVariantMap serviceInfoMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();

    if ( !serviceInfoMap["error"].isNull() )
    {
      // Something went wrong
      delete entry;
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
      crs.createFromString( "EPSG:4326" ); // If we can't recognize the SRS, fall back to WGS84
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
    QVariantList sublayers;
    for ( QVariant variant : serviceInfoMap["layers"].toList() )
    {
      QVariantMap entry = variant.toMap();
      QVariantMap sublayer;
      sublayer["id"] = entry["id"];
      sublayer["parentLayerId"] = entry["parentLayerId"];
      sublayer["name"] = entry["name"];
      sublayers.append( sublayer );
    }

    QgsMimeDataUtils::Uri mimeDataUri;
    mimeDataUri.name = entry->title;

    if ( KadasCatalogBrowser::sSettingLoadArcgisLayerAsVector->value() )
    {
      mimeDataUri.layerType = "vector";
      mimeDataUri.providerKey = "arcgisfeatureserver";
      mimeDataUri.uri = QString( "crs='%1' url='%2/0'" ).arg( crs.authid() ).arg( url );
    }
    else
    {
      mimeDataUri.layerType = "raster";
      mimeDataUri.providerKey = "arcgismapserver";
      QString format = filteredEncodings.isEmpty() || filteredEncodings.contains( "png32" ) ? "png32" : filteredEncodings.values().front();
      mimeDataUri.uri = QString( "crs='%1' format='%2' url='%3' layer='0'" ).arg( crs.authid() ).arg( format ).arg( url );
    }

    QMimeData *mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
    if ( !entry->flatten )
    {
      mimeData->setProperty( "sublayers", sublayers );
    }
    QStringList sortIndices = entry->sortIndices.split( "/" );
    mBrowser->addItem( getCategoryItem( entry->category.split( "/" ), sortIndices ), mimeDataUri.name, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
  }

  delete entry;
  endTask();
}

void KadasArcGisPortalCatalogProvider::addVTSlayer( const ResultEntry &entry )
{
  QString gdiBaseUrl = QgsSettings().value( "kadas/gdiBaseUrl" ).toString();
  QString styleUrl = QString( "%1/sharing/rest/content/items/%2/resources/styles/root.json" ).arg( gdiBaseUrl, entry.id );
  QString metadataUrl = QStringLiteral( "%1/home/item.html?id=%2" ).arg( gdiBaseUrl ).arg( entry.id );
  QgsMimeDataUtils::Uri mimeDataUri;
  mimeDataUri.layerType = "vector";
  mimeDataUri.providerKey = "arcgisvectortileservice";
  mimeDataUri.name = entry.title;
  mimeDataUri.uri = QStringLiteral( "serviceType=arcgis&styleUrl=%1&type=xyz&url=" ).arg( styleUrl, entry.url );
  QMimeData *mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
  mimeData->setProperty( "metadataUrl", metadataUrl );

  QStringList sortIndices = entry.sortIndices.split( "/" );
  mBrowser->addItem(
    getCategoryItem( entry.category.split( "/" ), sortIndices ),
    entry.title,
    sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(),
    true,
    mimeData
  );
}
