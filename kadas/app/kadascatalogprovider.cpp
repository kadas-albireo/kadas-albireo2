/***************************************************************************
    kadascatalogprovider.cpp
    ------------------------
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

#include <QDomElement>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmimedatautils.h>

#include "kadascatalogbrowser.h"
#include "kadascatalogprovider.h"


KadasCatalogProvider::KadasCatalogProvider( KadasCatalogBrowser* browser )
    : QObject( browser ), mBrowser( browser )
{}

QList<QDomNode> KadasCatalogProvider::childrenByTagName( const QDomElement& element, const QString& tagName ) const
{
  QDomElement el = element.firstChildElement( tagName );
  QList<QDomNode> children;
  while ( !el.isNull() )
  {
    children.append( el );
    el = el.nextSiblingElement( tagName );
  }
  return children;
}

QMap<QString, QString> KadasCatalogProvider::parseWMTSTileMatrixSets( const QDomDocument& doc ) const
{
  QMap<QString, QString> tileMatrixSetMap;
  for ( const QDomNode& tileMatrixSet : childrenByTagName( doc.firstChildElement( "Capabilities" ).firstChildElement( "Contents" ), "TileMatrixSet" ) )
  {
    QString identifier = tileMatrixSet.firstChildElement( "ows:Identifier" ).text();
    QgsCoordinateReferenceSystem crs;
    crs.createFromOgcWmsCrs( tileMatrixSet.firstChildElement( "ows:SupportedCRS" ).text() );
    tileMatrixSetMap.insert( identifier, crs.authid() );
  }
  return tileMatrixSetMap;
}

void KadasCatalogProvider::parseWMTSLayerCapabilities( const QDomNode& layerItem, const QMap<QString, QString>& tileMatrixSetMap, const QString& url, const QString& layerInfoUrl, const QString& extraParams, QString& title, QString& layerid, QMimeData*& mimeData ) const
{
  title = layerItem.firstChildElement( "ows:Title" ).text();
  layerid = layerItem.firstChildElement( "ows:Identifier" ).text();
  QString imgFormat = layerItem.firstChildElement( "Format" ).text();
  QString tileMatrixSet = layerItem.firstChildElement( "TileMatrixSetLink" ).firstChildElement( "TileMatrixSet" ).text();
  QString styleId = layerItem.firstChildElement( "Style" ).firstChildElement( "ows:Identifier" ).text();
  QString supportedCrs = tileMatrixSetMap[tileMatrixSet];
  QString dimensionParams = "";

  for ( const QDomNode& dim : childrenByTagName( layerItem.toElement(), "Dimension" ) )
  {
    QString dimId = dim.firstChildElement( "ows:Identifier" ).text();
    QString dimVal = dim.firstChildElement( "Default" ).text();
    dimensionParams += dimId + "%3D" + dimVal;
  }

  QgsMimeDataUtils::Uri mimeDataUri;
  mimeDataUri.layerType = "raster";
  mimeDataUri.providerKey = "wms";
  mimeDataUri.name = title;
  mimeDataUri.supportedCrs.append( supportedCrs );
  mimeDataUri.supportedFormats.append( imgFormat );
  mimeDataUri.uri = QString(
                      "contextualWMSLegend=0&featureCount=10&dpiMode=7&SmoothPixmapTransform=1"
                      "&layers=%1&crs=%2&format=%3&tileMatrixSet=%4"
                      "&styles=%5&url=%6%7"
                    ).arg( layerid ).arg( supportedCrs ).arg( imgFormat ).arg( tileMatrixSet ).arg( styleId ).arg( url ).arg( extraParams );
  mimeDataUri.uri += "&tileDimensions=" + dimensionParams; // Add this last because it contains % (from %3D) which confuses .arg
//  mimeDataUri.layerInfoUrl = layerInfoUrl; // TODO
  mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
}

QStringList KadasCatalogProvider::parseWMSFormats( const QDomDocument& doc ) const
{
  QStringList formats;
  QDomElement getMapItem = doc.firstChildElement( "WMS_Capabilities" )
                           .firstChildElement( "Capability" )
                           .firstChildElement( "Request" )
                           .firstChildElement( "GetMap" );
  for ( const QDomNode& formatItem : childrenByTagName( getMapItem, "Format" ) )
  {
    formats.append( formatItem.toElement().text() );
  }
  return formats;
}

QString KadasCatalogProvider::parseWMSNestedLayer( const QDomNode& layerItem ) const
{
  QString subLayerParams;
  for ( const QDomNode& subLayerItem : childrenByTagName( layerItem.toElement(), "Layer" ) )
  {
    QString subLayerName = subLayerItem.firstChildElement( "Name" ).toElement().text();
    subLayerParams += "&layers=" + subLayerName;
    subLayerParams += "&styles=";
    subLayerParams += parseWMSNestedLayer( subLayerItem );
  }
  return subLayerParams;
}

bool KadasCatalogProvider::parseWMSLayerCapabilities( const QDomNode& layerItem, const QStringList& imgFormats, const QStringList& parentCrs, const QString& url, const QString& layerInfoUrl, QString& title, QMimeData*& mimeData ) const
{
  title = layerItem.firstChildElement( "Title" ).text();
  QString layerid = layerItem.firstChildElement( "Name" ).text();
  QString subLayerParams = QString( "&layers=%1&styles=" ).arg( layerid );
  QStringList supportedCrs;
  for ( const QDomNode& crsItem : childrenByTagName( layerItem.toElement(), "CRS" ) )
  {
    supportedCrs.append( crsItem.toElement().text() );
  }
  QDomElement srsElement = layerItem.firstChildElement( "SRS" );
  if ( !srsElement.isNull() )
  {
    for ( const QString& authId : srsElement.text().split( "", QString::SkipEmptyParts ) )
    {
      supportedCrs.append( authId );
    }
  }
  if ( supportedCrs.isEmpty() )
  {
    supportedCrs.append( parentCrs );
  }
  if ( supportedCrs.isEmpty() )
  {
    // Don't list layer if not crs is found
    return false;
  }

  QString imgFormat = imgFormats[0];
  // Prefer png or jpeg
  if ( imgFormats.contains( "image/png" ) )
  {
    imgFormat = "image/png";
  }
  else if ( imgFormats.contains( "image/jpeg" ) )
  {
    imgFormat = "image/jpeg";
  }

  QgsMimeDataUtils::Uri mimeDataUri;
  mimeDataUri.layerType = "raster";
  mimeDataUri.providerKey = "wms";
  mimeDataUri.name = title;
  mimeDataUri.supportedCrs = supportedCrs;
  mimeDataUri.supportedFormats = imgFormats;
  mimeDataUri.uri = QString(
                      "contextualWMSLegend=0&featureCount=10&dpiMode=7"
                      "&crs=%1&format=%2"
                      "%3&url=%4" ).arg( supportedCrs[0] ).arg( imgFormat ).arg( subLayerParams ).arg( url );
  // mimeDataUri.layerInfoUrl = layerInfoUrl; // TODO
  mimeData = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << mimeDataUri );
  return true;
}

QStandardItem* KadasCatalogProvider::getCategoryItem( const QStringList& titles, const QStringList& sortIndices )
{
  QStandardItem* cat = 0;
  int n = titles.size();
  int m = sortIndices.size();
  for ( int i = 0; i < n; ++i )
  {
    cat = mBrowser->addItem( cat, titles[i], i < m ? sortIndices[i].toInt() : -1 );
  }
  return cat;
}

