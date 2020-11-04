/***************************************************************************
    kadasarcgisportalcatalogprovider.h
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

#ifndef KADASARCGISPORTALCATALOGPROVIDER_H
#define KADASARCGISPORTALCATALOGPROVIDER_H

#include <QSet>
#include <kadas/gui/kadascatalogprovider.h>

class QStandardItem;

class KADAS_GUI_EXPORT KadasArcGisPortalCatalogProvider : public KadasCatalogProvider
{
    Q_OBJECT
  public:
    KadasArcGisPortalCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> &params );
    void load() override;

  signals:
    void userChanged( const QString &user );

  private slots:
    void replyFinished();

  private:
    struct ResultEntry
    {
      ResultEntry() {}
      ResultEntry( const QString &_category, const QString &_title, const QString &_sortIndices, const QString &_metadataUrl, bool _flatten = false )
        : category( _category ), title( _title ), sortIndices( _sortIndices ), metadataUrl( _metadataUrl ), flatten( _flatten ) {}
      ResultEntry( const ResultEntry &entry )
        : category( entry.category ), title( entry.title ), sortIndices( entry.sortIndices ), metadataUrl( entry.metadataUrl ) {}
      QString category;
      QString title;
      QString sortIndices;
      QString metadataUrl;
      bool flatten;
    };
    typedef QMap< QString, ResultEntry > EntryMap;

    struct IsoTopicGroup
    {
      QString category;
      QString sortIndices;
    };
    QMap<QString, IsoTopicGroup> mIsoTopics;

    QString mBaseUrl;
    QString mServicePreference;
    int mPendingTasks;

    QMap<QString, EntryMap> mAmsLayers;
    QMap<QString, EntryMap> mWmtsLayers;
    QMap<QString, EntryMap> mWmsLayers;
    QMap<QString, QPair<QString, QString>> mAmsLayerIds;
    QMap<QString, QPair<QString, QString>> mWmsLayerIds;

    void endTask();

    void readWMTSCapabilities( const QString &wmtsUrl, const EntryMap &entries );
    void readWMSCapabilities( const QString &wmsUrl, const EntryMap &entries );
    void readAMSCapabilities( const QString &amsUrl, const EntryMap &entries );

    void readWMSSublayers( const QDomElement &layerItem, const QString &parentName, QVariantList &sublayers );

  private slots:
    void readWMTSCapabilitiesDo();
    void readWMSCapabilitiesDo();
    void readAMSCapabilitiesDo();
};

#endif // KADASARCGISPORTALCATALOGPROVIDER_H
