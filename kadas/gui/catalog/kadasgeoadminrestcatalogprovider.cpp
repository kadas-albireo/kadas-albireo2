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
#include <QNetworkRequest>
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
  QNetworkRequest req( mBaseUrl );
  req.setRawHeader( "Referer", QgsSettings().value( "search/referer", "http://localhost" ).toByteArray() );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, &QNetworkReply::finished, this, &KadasGeoAdminRestCatalogProvider::replyFinished );
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

void KadasGeoAdminRestCatalogProvider::replyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  reply->deleteLater();
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

  // Layers
  QList<QDomNode> layerItems = childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "Layer" );
  for ( const QDomNode &layerItem : layerItems )
  {
    QString title, layerid;
    QMimeData *mimeData;
    parseWMTSLayerCapabilities( layerItem, tileMatrixSetMap, mBaseUrl, "", QString( "&referer=%1" ).arg( referer ), title, layerid, QString(), mimeData );

    // Determine paren
    QStandardItem *parent = 0;
    if ( layerParentMap.contains( layerid ) )
    {
      parent = layerParentMap.value( layerid );
    }
    else
    {
      parent = mBrowser->addItem( 0, tr( "Uncategorized" ), -1 );
    }
    mBrowser->addItem( parent, title, true, mimeData );
  }
  emit finished();
}
