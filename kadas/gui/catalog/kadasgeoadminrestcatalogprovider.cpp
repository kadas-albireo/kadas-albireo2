/***************************************************************************
    kadasgeoadmincatalogprovider.cpp
    --------------------------------
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
#include <QDomElement>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QNetworkReply>

#include <QFile>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/kadascatalogbrowser.h"
#include "kadas/gui/catalog/kadasgeoadminrestcatalogprovider.h"

KadasGeoAdminRestCatalogProvider::KadasGeoAdminRestCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> & /*params*/ )
  : KadasCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void KadasGeoAdminRestCatalogProvider::load()
{
  QUrl url( mBaseUrl );
  QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toUpper();
  QUrlQuery query( url );
  query.addQueryItem( "lang", lang );
  url.setQuery( query );
  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, &QNetworkReply::finished, this, &KadasGeoAdminRestCatalogProvider::replyGeoCatalogFinished );
}

void KadasGeoAdminRestCatalogProvider::parseTheme( QStandardItem *parent, const QDomElement &theme, QMap<QString, QStandardItem *> &layerParentMap )
{
  parent = mBrowser->addItem( parent, theme.firstChildElement( "ows:Title" ).text(), -1 );
  QDomNodeList layerRefs = theme.toElement().elementsByTagName( "LayerRef" );
  for ( int iLayerRef = 0, nLayerRefs = layerRefs.count(); iLayerRef < nLayerRefs; ++iLayerRef )
  {
    QDomNode layerRef = layerRefs.item( iLayerRef );
    layerParentMap.insert( layerRef.toElement().text(), parent );
  }

  for ( const QDomNode &theme : childrenByTagName( theme.toElement(), "Theme" ) )
  {
    parseTheme( parent, theme.toElement(), layerParentMap );
  }
}

void KadasGeoAdminRestCatalogProvider::replyGeoCatalogFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
  if ( reply->error() != QNetworkReply::NoError )
  {
    emit finished();
    return;
  }
  QJsonDocument doc;

  QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
  QVariantList topCategories = rootMap.value( "results" ).toMap().value( "root" ).toMap().value( "children" ).toList();
  for ( const QVariant &topCategorie : topCategories )
  {
    QString topCategoryLabel = topCategorie.toMap().value( "label" ).toString();
    QVariantList subCategories = topCategorie.toMap().value( "children" ).toList();

    QStandardItem *topCategoryItem = mBrowser->addItem( 0, topCategoryLabel, 0, false );


    for ( const QVariant &subCategory : subCategories )
    {
      //layerObj.toMap()
      //mBrowser->addItem()
      QString subCategoryLabel = subCategory.toMap().value( "label" ).toString();
      QStandardItem *subCategoryItem = mBrowser->addItem( topCategoryItem, subCategoryLabel, 0, false );

      QVariantList layerList = subCategory.toMap().value( "children" ).toList();
      for ( const QVariant &layerObj : layerList )
      {
        //QString layerLabel = layerObj.toMap().value( "label" ).toString();
        QString layerBodId = layerObj.toMap().value( "layerBodId" ).toString();
        QMimeData *mimeData = nullptr;

        //mimeData = QgsMimeDataUtils::Uri(layerObj.toMap().value( "layerBodId" ).toString());
        //mBrowser->addItem( subCategoryItem, layerBodId, 0, false /*,  mimeData */ );

        mMapLayerCategory.insert( layerBodId, topCategoryLabel + "/" + subCategoryLabel );
      }

      //mBrowser->addItem( 0, tr( "Uncategorized" ), -1 );
    }
  }

  // QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap()
  QVariantList layers = rootMap.value( "layers" ).toList();
  for ( const QVariant &layerObj : layers )
  {
    QVariantMap layerMap = layerObj.toMap();
    //QVariantMap layerMap = layerObj.toMap();
  }

  //for ( QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter )
  //{
  // qDebug() << iter.key() << iter.value();
  //}
  qDebug() << "id?:" << rootMap.value( "results" ).toMap().value( "root" ).toMap().value( "id" ).toString();


  QUrl url( "https://wms.geo.admin.ch/?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities" );
  QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toLower();
  QUrlQuery query( url );
  query.addQueryItem( "lang", lang );
  url.setQuery( query );
  QNetworkRequest req( url );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply *replyWMS = QgsNetworkAccessManager::instance()->get( req );
  replyWMS->setProperty( "url", url );
  connect( replyWMS, &QNetworkReply::finished, this, &KadasGeoAdminRestCatalogProvider::replyWMSGeoAdminFinished );
}


void KadasGeoAdminRestCatalogProvider::replyWMSGeoAdminFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
  QString url = reply->property( "url" ).toString();
  if ( reply->error() != QNetworkReply::NoError )
  {
    emit finished();
    return;
  }

  QDomDocument doc;
  doc.setContent( reply->readAll() );

  if ( doc.isNull() )
  {
    emit finished();
    return;
  }


  QFile file( "C:\\Users\\Valentin\\Documents\\kadas\\simple.xml" );
  if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    qDebug( "Failed to open file for writing." );
  }
  QTextStream stream( &file );
  stream << doc.toString();
  file.close();


  QString referer = QgsSettings().value( "search/referer", "http://localhost" ).toString();

  // Categories
  QMap<QString, QStandardItem *> layerParentMap;
  QDomElement themes = doc.firstChildElement( "Capabilities" ).firstChildElement( "Themes" );
  for ( const QDomNode &theme : childrenByTagName( themes, "Theme" ) )
  {
    parseTheme( 0, theme.toElement(), layerParentMap );
  }

  // Tile matrix sets
  QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );

  //// Layers
  //QList<QDomNode> layerItems = childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" );
  //for ( const QDomNode &layerItem : layerItems )
  //{
  //  QString title, layerid;
  //  QMimeData *mimeData;
  //  parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, mBaseUrl, "", QString( "&referer=%1" ).arg( referer ), title, layerid, QString(), mimeData );

  //  // Determine parent
  //  QStandardItem *parent = 0;
  //  if ( layerParentMap.contains( layerid ) )
  //  {
  //    parent = layerParentMap.value( layerid );
  //  }
  //  else
  //  {
  //    parent = mBrowser->addItem( 0, tr( "Uncategorized" ), -1 );
  //  }
  //  mBrowser->addItem( parent, title, true, mimeData );
  //}

  QStringList imgFormats = parseWMSFormats( doc );
  QStringList parentCrs;
  for ( const QDomNode &layerItem : childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ).firstChildElement( "Layer" ), "Layer" ) )
  {
    QMimeData *mimeData;
    QString title = layerItem.firstChildElement( "Title" ).text();
    QString layerBodId = layerItem.firstChildElement( "Name" ).text();

    parseWMSLayerCapabilities( layerItem, title, imgFormats, parentCrs, url, "", QString(), mimeData );

    QStandardItem *parent;
    if ( mMapLayerCategory.contains( layerBodId ) )
    {
      parent = getCategoryItem( mMapLayerCategory.value( layerBodId ).split( "/" ), QStringList() );
    }
    else
    {
      parent = mBrowser->addItem( 0, tr( "Uncategorized" ), -1 );
    }
    //mBrowser->addItem( 0, title, -1, true, mimeData );
    mBrowser->addItem( parent, title, -1, true, mimeData );
    //mBrowser->addItem( /*getCategoryItem( catTitles, QStringList() )*/ 0, title, -1, mimeData );
  }
  emit finished();
}