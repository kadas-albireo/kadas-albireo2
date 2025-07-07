/***************************************************************************
    kadasworldlocationsearchprovider.h
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

#ifndef KADASWORLDVBSLOCATIONSEARCHPROVIDER_H
#define KADASWORLDVBSLOCATIONSEARCHPROVIDER_H

#include <QMap>

#include <qgis/qgslocatorfilter.h>

#include "kadas/gui/kadas_gui.h"


class QNetworkReply;
class QEventLoop;

class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasWorldLocationSearchProvider : public QgsLocatorFilter
{
    Q_OBJECT
  public:
    KadasWorldLocationSearchProvider( QgsMapCanvas *mapCanvas );

    virtual QgsLocatorFilter *clone() const override;
    QString name() const override { return QStringLiteral( "world-location-search" ); }
    QString displayName() const override { return tr( " World Location Search" ); }
    virtual Priority priority() const override { return Priority::Low; }
    virtual void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    virtual void triggerResult( const QgsLocatorResult &result ) override;
    virtual void clearPreviousResults() override;

  private slots:
    void handleNetworkReply();

  private:
    static const int sResultCountLimit;

    QString mPinItemId;
    QStringList mGeometryItemIds;
    QgsMapCanvas *mMapCanvas = nullptr;
    QMap<QString, QPair<QString, int>> mCategoryMap;

    QNetworkReply *mCurrentReply = nullptr;
    QgsFeedback *mFeedback = nullptr;
    QEventLoop *mEventLoop = nullptr;
};

#endif // KADASWORLDVBSLOCATIONSEARCHPROVIDER_H
