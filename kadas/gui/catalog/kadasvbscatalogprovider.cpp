/***************************************************************************
    kadasvbscatalogprovider.cpp
    ---------------------------
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

#include <kadas/gui/kadascatalogbrowser.h>
#include <kadas/gui/catalog/kadasvbscatalogprovider.h>

KadasVBSCatalogProvider::KadasVBSCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser )
  : KadasCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void KadasVBSCatalogProvider::load()
{
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
  connect( reply, &QNetworkReply::finished, this, &KadasVBSCatalogProvider::replyFinished );
}

void KadasVBSCatalogProvider::replyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( reply->error() == QNetworkReply::NoError )
  {
    QVariantMap listData = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    QMap<QString, EntryMap> wmtsLayers;
    QMap<QString, EntryMap> wmsLayers;
    QMap<QString, EntryMap> amsLayers;
    emit userChanged( listData["user"].toString() );
    for ( const QVariant &resultData : listData["results"].toList() )
    {
      QVariantMap resultMap = resultData.toMap();
      if ( resultMap["type"].toString() == "wmts" )
      {
        wmtsLayers[resultMap["serviceUrl"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString(), resultMap["position"].toString(), resultMap["metadataUrl"].toString() ) );
      }
      else if ( resultMap["type"].toString() == "wms" )
      {
        wmsLayers[resultMap["serviceUrl"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString(), resultMap["position"].toString(), resultMap["metadataUrl"].toString() ) );
      }
      else if ( resultMap["type"].toString() == "ams" )
      {
        amsLayers[resultMap["serviceUrl"].toString()].insert( resultMap["layerName"].toString(), ResultEntry( resultMap["category"].toString(), resultMap["title"].toString(), resultMap["position"].toString(), resultMap["metadataUrl"].toString() ) );
      }
    }

    for ( const QString &wmtsUrl : wmtsLayers.keys() )
    {
      readWMTSCapabilities( wmtsUrl, wmtsLayers[wmtsUrl] );
    }
    for ( const QString &wmsUrl : wmsLayers.keys() )
    {
      readWMSCapabilities( wmsUrl, wmsLayers[wmsUrl] );
    }
    for ( const QString &amsUrl : amsLayers.keys() )
    {
      readAMSCapabilities( amsUrl, amsLayers[amsUrl] );
    }
  }
  reply->deleteLater();
  endTask();
}

void KadasVBSCatalogProvider::endTask()
{
  mPendingTasks -= 1;
  if ( mPendingTasks == 0 )
  {
    emit finished();
  }
}

void KadasVBSCatalogProvider::readWMTSCapabilities( const QString &wmtsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( ( QUrl( wmtsUrl ) ) );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasVBSCatalogProvider::readWMTSCapabilitiesDo );
}

void KadasVBSCatalogProvider::readWMTSCapabilitiesDo()
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

void KadasVBSCatalogProvider::readWMSCapabilities( const QString &wmsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( wmsUrl + "?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0" ) );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "url", wmsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasVBSCatalogProvider::readWMSCapabilitiesDo );
}

void KadasVBSCatalogProvider::readWMSCapabilitiesDo()
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
    for ( const QDomNode &layerItem : childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ), "Layer" ) )
    {
      searchMatchingWMSLayer( layerItem, *entries, url, imgFormats, parentCrs );
    }
  }

  delete entries;
  endTask();
}

void KadasVBSCatalogProvider::readAMSCapabilities( const QString &amsUrl, const EntryMap &entries )
{
  mPendingTasks += 1;
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  QUrl url( amsUrl + "?f=json" );

  QNetworkRequest req( url );
  QNetworkReply *reply = nam->get( req );
  reply->setProperty( "url", amsUrl );
  reply->setProperty( "entries", QVariant::fromValue<void *> ( reinterpret_cast<void *>( new EntryMap( entries ) ) ) );
  connect( reply, &QNetworkReply::finished, this, &KadasVBSCatalogProvider::readAMSCapabilitiesDo );
}

void KadasVBSCatalogProvider::readAMSCapabilitiesDo()
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
      QStringList sortIndices = entry.sortIndices.split( "/" );
      mBrowser->addItem( getCategoryItem( entry.category.split( "/" ), sortIndices ), mimeDataUri.name, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
    }
  }

  delete entries;
  endTask();
}

void KadasVBSCatalogProvider::searchMatchingWMSLayer( const QDomNode &layerItem, const EntryMap &entries, const QString &url, const QStringList &imgFormats, QStringList parentCrs )
{
  QString layerid = layerItem.firstChildElement( "Name" ).text();
  if ( entries.contains( layerid ) )
  {
    QString title;
    QMimeData *mimeData;
    const ResultEntry &entry = entries[layerid];
    if ( parseWMSLayerCapabilities( layerItem, imgFormats, parentCrs, url, entry.metadataUrl, title, mimeData ) )
    {
      QStringList sortIndices = entry.sortIndices.split( "/" );
      mBrowser->addItem( getCategoryItem( entry.category.split( "/" ), sortIndices ), entries[layerid].title, sortIndices.isEmpty() ? -1 : sortIndices.last().toInt(), true, mimeData );
    }
  }
  for ( const QDomNode &crsItem : childrenByTagName( layerItem.toElement(), "CRS" ) )
  {
    parentCrs.append( crsItem.toElement().text() );
  }
  QDomElement srsElement = layerItem.firstChildElement( "SRS" );
  if ( !srsElement.isNull() )
  {
    for ( const QString &authId : srsElement.text().split( "", QString::SkipEmptyParts ) )
    {
      parentCrs.append( authId );
    }
  }
  for ( const QDomNode &subLayerItem : childrenByTagName( layerItem.toElement(), "Layer" ) )
  {

    searchMatchingWMSLayer( subLayerItem, entries, url, imgFormats, parentCrs );
  }
}
