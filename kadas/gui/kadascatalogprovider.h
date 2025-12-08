/***************************************************************************
    kadascatalogprovider.h
    ----------------------
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

#ifndef KADASCATALOGPROVIDER_H
#define KADASCATALOGPROVIDER_H

#include <QObject>
#include <QMap>

#include "kadas/core/kadas.h"

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QDomElement;
class QDomNode;
class QMimeData;
class QStandardItem;
class KadasCatalogBrowser;


class KADAS_GUI_EXPORT KadasCatalogProvider : public QObject
{
    Q_OBJECT
  public:
    KadasCatalogProvider( KadasCatalogBrowser *browser );
    virtual void load() = 0;

  signals:
    void finished();

  protected:
    KadasCatalogBrowser *mBrowser;

    QList<QDomNode> childrenByTagName( const QDomElement &element, const QString &tagName ) const;
    QMap<QString, QString> parseWMTSTileMatrixSets( const QDomDocument &doc ) const;
    void parseWMTSLayerCapabilities( const QDomNode &layerItem, const QMap<QString, QString> &tileMatrixSetMap, const QString &url, const QString &layerInfoUrl, const QString &extraParams, QString &title, QString &layerid, const QString &authCfg, QMimeData *&mimeData ) const;
    QStringList parseWMSFormats( const QDomDocument &doc ) const;
    QString parseWMSNestedLayer( const QDomNode &layerItem ) const;
    bool parseWMSLayerCapabilities( const QDomNode &layerItem, const QString &title, const QStringList &imgFormats, const QStringList &parentCrs, const QString &url, const QString &layerInfoUrl, const QString &authCfg, QMimeData *&mimeData ) const;
    QStandardItem *getCategoryItem( const QStringList &titles, const QStringList &sortIndices );

#ifndef SIP_RUN
    struct ResultEntry
    {
        ResultEntry() {}
        ResultEntry( const QString &_id, const QString &_category, const QString &_sortIndices )
          : id( _id )
          , category( _category )
          , sortIndices( _sortIndices )
        {}

        ResultEntry( const QString &_url, const QString &_id, const QString &_category, const QString &_title, const QString &_sortIndices, const QString &_metadataUrl, const QString &_detailUrl, bool _flatten = false )
          : url( _url )
          , id( _id )
          , category( _category )
          , title( _title )
          , sortIndices( _sortIndices )
          , metadataUrl( _metadataUrl )
          , detailUrl( _detailUrl )
          , flatten( _flatten )
        {}

        ResultEntry( const ResultEntry &entry )
          : url( entry.url )
          , id( entry.id )
          , category( entry.category )
          , title( entry.title )
          , sortIndices( entry.sortIndices )
          , metadataUrl( entry.metadataUrl )
          , detailUrl( entry.detailUrl )
          , flatten( entry.flatten )
        {}

        QString url;
        QString id;
        QString category;
        QString title;
        QString sortIndices;
        QString metadataUrl;
        QString detailUrl;
        bool flatten;
    };
    typedef QMap<QString, ResultEntry> EntryMap;
#endif
};

#endif // KADASCATALOGPROVIDER_H
