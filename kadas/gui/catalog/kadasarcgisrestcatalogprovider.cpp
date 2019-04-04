/***************************************************************************
    kadasarcgisrestcatalogprovider.cpp
    ----------------------------------
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
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <qgis/qgsnetworkaccessmanager.h>

#include <kadas/gui/kadascatalogbrowser.h>

#include "kadasarcgisrestcatalogprovider.h"


KadasArcGisRestCatalogProvider::KadasArcGisRestCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser )
    : KadasCatalogProvider( browser ), mBaseUrl( baseUrl )
{
}

void KadasArcGisRestCatalogProvider::load()
{
  mPendingTasks = 0;
  parseFolder( "/" );
}

void KadasArcGisRestCatalogProvider::endTask()
{
  mPendingTasks -= 1;
  if ( mPendingTasks == 0 )
  {
    emit finished();
  }
}

void KadasArcGisRestCatalogProvider::parseFolder( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( mBaseUrl + QString( "/rest/services%1?f=json" ).arg( path ) ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisRestCatalogProvider::parseFolderDo );
}

void KadasArcGisRestCatalogProvider::parseFolderDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QVariantMap folderData = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    QString catName = QFileInfo( path ).baseName();
    if ( !catName.isEmpty() )
    {
      catTitles.append( catName );
    }
    for ( const QVariant& folderName : folderData["folders"].toList() )
    {
      parseFolder( path + "/" + folderName.toString(), catTitles );
    }
    for ( const QVariant& serviceData : folderData["services"].toList() )
    {
      parseService( QString( "/" ) + serviceData.toMap()["name"].toString(), catTitles );
    }
  }

  endTask();
}

void KadasArcGisRestCatalogProvider::parseService( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QNetworkRequest req( QUrl( mBaseUrl + QString( "/rest/services%1/MapServer?f=json" ).arg( path ) ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisRestCatalogProvider::parseServiceDo );
}

void KadasArcGisRestCatalogProvider::parseServiceDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QVariantMap serviceData = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    if ( serviceData.contains( "singleFusedMapCache" ) )
    {
      QString catName = serviceData["documentInfo"].toMap()["Title"].toString();
      if ( catName.isEmpty() )
      {
        catName = QFileInfo( path ).baseName();
      }
      catTitles.append( catName );

      bool isWMTS = serviceData.value( "singleFusedMapCache", 0 ).toInt();
      if ( isWMTS )
      {
        parseWMTS( path, catTitles );
      }
      else
      {
        parseWMS( path, catTitles );
      }
    }
  }

  endTask();
}

void KadasArcGisRestCatalogProvider::parseWMTS( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QString url = mBaseUrl + QString( "/rest/services/%1/MapServer/WMTS/1.0.0/WMTSCapabilities.xml" ).arg( path );
  QNetworkRequest req = QNetworkRequest( QUrl( url ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  reply->setProperty( "url", url );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisRestCatalogProvider::parseWMTSDo );
}

void KadasArcGisRestCatalogProvider::parseWMTSDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  QString path = reply->property( "path" ).toString();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    if ( !doc.isNull() )
    {
      QMap<QString, QString> tileMatrixSetMap = parseWMTSTileMatrixSets( doc );

      // Layers
      for ( const QDomNode& layerItem : childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" ) )
      {
        QString title, layerid;
        QMimeData* mimeData;
        parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, url, "", "", title, layerid, mimeData );
        mBrowser->addItem( getCategoryItem( catTitles, QStringList() ), title, -1, true, mimeData );
      }
    }
  }

  endTask();
}

void KadasArcGisRestCatalogProvider::parseWMS( const QString& path, const QStringList& catTitles )
{
  mPendingTasks += 1;
  QString url = mBaseUrl + QString( "/services/%1/MapServer/WMSServer?request=GetCapabilities&service=WMS" ).arg( path );
  QNetworkRequest req = QNetworkRequest( QUrl( url ) );
  QNetworkReply* reply = QgsNetworkAccessManager::instance()->get( req );
  reply->setProperty( "path", path );
  reply->setProperty( "catTitles", catTitles );
  reply->setProperty( "url", url );
  connect( reply, &QNetworkReply::finished, this, &KadasArcGisRestCatalogProvider::parseWMSDo );
}

void KadasArcGisRestCatalogProvider::parseWMSDo()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>( QObject::sender() );
  reply->deleteLater();
  QStringList catTitles = reply->property( "catTitles" ).toStringList();
  QString url = reply->property( "url" ).toString();

  if ( reply->error() == QNetworkReply::NoError )
  {
    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QStringList imgFormats = parseWMSFormats( doc );
    QStringList parentCrs;
    for ( const QDomNode& layerItem : childrenByTagName( doc.firstChildElement( "WMS_Capabilities" ).firstChildElement( "Capability" ), "Layer" ) )
    {
      QString title;
      QMimeData* mimeData;
      parseWMSLayerCapabilities( layerItem, imgFormats, parentCrs, url, "", title, mimeData );
      mBrowser->addItem( getCategoryItem( catTitles, QStringList() ), title, -1, mimeData );
    }
  }

  endTask();
}
