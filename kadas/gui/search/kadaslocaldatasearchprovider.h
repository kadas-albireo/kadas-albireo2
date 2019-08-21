/***************************************************************************
    kadaslocaldatasearchprovider.h
    ------------------------------
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


#ifndef KADASLOCALDATASEARCHPROVIDER_H
#define KADASLOCALDATASEARCHPROVIDER_H

#include <QMutex>
#include <QPointer>

#include <kadas/gui/kadassearchprovider.h>

class QgsFeature;
class QgsMapLayer;
class QgsVectorLayer;
class KadasLocalDataSearchCrawler;

class KADAS_GUI_EXPORT KadasLocalDataSearchProvider : public KadasSearchProvider
{
    Q_OBJECT
  public:
    KadasLocalDataSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString &searchtext, const SearchRegion &searchRegion ) override;
    void cancelSearch() override;

  private:
    QPointer<KadasLocalDataSearchCrawler> mCrawler;
};


class KADAS_GUI_EXPORT KadasLocalDataSearchCrawler : public QObject
{
    Q_OBJECT
  public:
    KadasLocalDataSearchCrawler( const QString &searchText,
                                 const KadasSearchProvider::SearchRegion &searchRegion,
                                 QList<QgsMapLayer *> layers, QObject *parent = 0 )
      : QObject( parent ), mSearchText( searchText ), mSearchRegion( searchRegion ), mLayers( layers ), mAborted( false ) {}

    void abort();

  public slots:
    void run();

  signals:
    void searchResultFound( KadasSearchProvider::SearchResult result );
    void searchFinished();

  private:
    static const int sResultCountLimit;

    QString mSearchText;
    KadasSearchProvider::SearchRegion mSearchRegion;
    QList<QgsMapLayer *> mLayers;
    QMutex mAbortMutex;
    bool mAborted;

    void buildResult( const QgsFeature &feature, QgsVectorLayer *layer );
};

#endif // KADASLOCALDATASEARCHPROVIDER_H
