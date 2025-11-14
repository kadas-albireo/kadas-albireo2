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
#include "kadas/gui/kadascatalogprovider.h"

class QStandardItem;

class KADAS_GUI_EXPORT KadasArcGisPortalCatalogProvider : public KadasCatalogProvider
{
    Q_OBJECT
  public:
    KadasArcGisPortalCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> &params, const QString &authId = QString() );

    void load() override;

  private slots:
    void replyFinished();

  private:
    struct IsoTopicGroup
    {
        QString category;
        QString sortIndices;
    };
    QMap<QString, IsoTopicGroup> mIsoTopics;

    QString mBaseUrl;
    QString mServicePreference;
    QString mCatalogTag;
    int mPendingTasks;

    QMap<QString, QMap<QString, ResultEntry>> mLayers;

    void endTask();

    void readWMTSDetail( const ResultEntry &entry );
    void readWMSDetail( const ResultEntry &entry );
    void addVTSlayer( const ResultEntry &entry );
    void readWMTSCapabilities();
    void readWMSCapabilities();
    void readAMSCapabilities( const ResultEntry &entry );

    void readWMSSublayers( const QDomElement &layerItem, const QString &parentName, QVariantList &sublayers );

  private slots:
    void readWMTSCapabilitiesDo();
    void readWMSCapabilitiesDo();
    void readAMSCapabilitiesDo();

  private:
    QString mAuthId;
};

#endif // KADASARCGISPORTALCATALOGPROVIDER_H
