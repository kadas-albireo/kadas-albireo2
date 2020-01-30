/***************************************************************************
    kadasvbscatalogprovider.h
    -------------------------
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

#ifndef KADASVBSCATALOGPROVIDER_H
#define KADASVBSCATALOGPROVIDER_H

#include <kadas/gui/kadascatalogprovider.h>

class QStandardItem;

class KADAS_GUI_EXPORT KadasVBSCatalogProvider : public KadasCatalogProvider
{
    Q_OBJECT
  public:
    KadasVBSCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser );
    void load() override;

  signals:
    void userChanged( const QString &user );

  private slots:
    void replyFinished();

  private:
    struct ResultEntry
    {
      ResultEntry() {}
      ResultEntry( const QString &_category, const QString &_title, const QString &_sortIndices, const QString &_metadataUrl )
        : category( _category ), title( _title ), sortIndices( _sortIndices ), metadataUrl( _metadataUrl ) {}
      ResultEntry( const ResultEntry &entry )
        : category( entry.category ), title( entry.title ), sortIndices( entry.sortIndices ), metadataUrl( entry.metadataUrl ) {}
      QString category;
      QString title;
      QString sortIndices;
      QString metadataUrl;
    };
    typedef QMap< QString, ResultEntry > EntryMap;

    QString mBaseUrl;
    int mPendingTasks;

    void endTask();

    void readWMTSCapabilities( const QString &wmtsUrl, const EntryMap &entries );
    void readWMSCapabilities( const QString &wmsUrl, const EntryMap &entries );
    void readAMSCapabilities( const QString &amsUrl, const EntryMap &entries );
    void searchMatchingWMSLayer( const QDomNode &layerItem, const EntryMap &entries, const QString &url, const QStringList &imgFormats, QStringList parentCrs );

  private slots:
    void readWMTSCapabilitiesDo();
    void readWMSCapabilitiesDo();
    void readAMSCapabilitiesDo();
};

#endif // KADASVBSCATALOGPROVIDER_H
