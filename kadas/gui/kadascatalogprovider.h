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

#include <kadas/gui/kadas_gui.h>

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
    void parseWMTSLayerCapabilities( const QDomNode &layerItem, const QMap<QString, QString> &tileMatrixSetMap, const QString &url, const QString &layerInfoUrl, const QString &extraParams, QString &title, QString &layerid, QMimeData *&mimeData ) const;
    QStringList parseWMSFormats( const QDomDocument &doc ) const;
    QString parseWMSNestedLayer( const QDomNode &layerItem ) const;
    bool parseWMSLayerCapabilities( const QDomNode &layerItem, const QStringList &imgFormats, const QStringList &parentCrs, const QString &url, const QString &layerInfoUrl, QString &title, QMimeData *&mimeData ) const;
    QStandardItem *getCategoryItem( const QStringList &titles, const QStringList &sortIndices );
};

#endif // KADASCATALOGPROVIDER_H
