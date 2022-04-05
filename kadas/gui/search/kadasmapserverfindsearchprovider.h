/***************************************************************************
    kadasmapserverfindsearchprovider.h
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


#ifndef KADASMAPSERVERFINDSEARCHPROVIDER_H
#define KADASMAPSERVERFINDSEARCHPROVIDER_H

#include <QMap>
#include <QRegExp>
#include <QTimer>

#include <kadas/gui/kadassearchprovider.h>

class QNetworkAccessManager;
class QNetworkReply;

class KADAS_GUI_EXPORT KadasMapServerFindSearchProvider : public KadasSearchProvider
{
    Q_OBJECT
  public:
    KadasMapServerFindSearchProvider( QgsMapCanvas *mapCanvas );
    void startSearch( const QString &searchtext, const SearchRegion &searchRegion ) override;
    void cancelSearch() override;

  private:
    static const int sSearchTimeout;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;

    QList<QNetworkReply *> mNetReplies;
    QgsGeometry *mReplyFilter;
    QRegExp mPatBox;
    QTimer mTimeoutTimer;

  private slots:
    void replyFinished();
    void searchTimeout() { cancelSearch(); }
};

#endif // KADASMAPSERVERFINDSEARCHPROVIDER_H
