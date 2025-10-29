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
  int topCategoriesIndice = 1;

  for ( const QVariant &topCategorie : topCategories )
  {
    QString topCategoryLabel = topCategorie.toMap().value( "label" ).toString();
    QVariantList subCategories = topCategorie.toMap().value( "children" ).toList();

    QStandardItem *topCategoryItem = mBrowser->addItem( 0, topCategoryLabel, 0, false );

    int subCategoriesIndice = 1;
    for ( const QVariant &subCategory : subCategories )
    {
      QString subCategoryLabel = subCategory.toMap().value( "label" ).toString();

      QVariantList layerList = subCategory.toMap().value( "children" ).toList();
      int layerIndice = 1;
      for ( const QVariant &layerObj : layerList )
      {
        QString layerBodId = layerObj.toMap().value( "layerBodId" ).toString();

        QString sortIndices = QString( "%1/%2/%3" ).arg( topCategoriesIndice ).arg( subCategoriesIndice ).arg( layerIndice );
        ResultEntry entry = ResultEntry( layerBodId, topCategoryLabel + "/" + subCategoryLabel, sortIndices );

        mLayersEntriesMap.insert( layerBodId, entry );

        layerIndice++;
      }
      subCategoriesIndice++;
    }
    topCategoriesIndice++;
  }
  mLastTopCategoriesIndice = topCategoriesIndice;

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


  QStringList imgFormats = parseWMSFormats( doc );
  QStringList parentCrs;
  for ( const QDomNode &layerItem : childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ).firstChildElement( "Layer" ), "Layer" ) )
  {
    QMimeData *mimeData;
    QString title = layerItem.firstChildElement( "Title" ).text();
    QString layerBodId = layerItem.firstChildElement( "Name" ).text();
    parseWMSLayerCapabilities( layerItem, title, imgFormats, parentCrs, url, "", QString(), mimeData );

    QStandardItem *parent;
    ResultEntry entry = mLayersEntriesMap.value( layerBodId );
    if ( mLayersEntriesMap.contains( layerBodId ) )
    {
      parent = getCategoryItem( entry.category.split( "/" ), entry.sortIndices.split( "/" ) );
    }
    else
    {
      parent = mBrowser->addItem( 0, tr( "Uncategorized" ), mLastTopCategoriesIndice + 1 );
    }

    mBrowser->addItem( parent, title, entry.sortIndices.split( "/" ).last().toInt(), true, mimeData );
  }
  emit finished();
}