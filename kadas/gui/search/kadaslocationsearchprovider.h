/***************************************************************************
    kadaslocationsearchprovider.h
    -----------------------------
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


#ifndef KADASLOCATIONSEARCHPROVIDER_H
#define KADASLOCATIONSEARCHPROVIDER_H

#include <QPointer>
#include <QMap>
#include <QRegExp>

#include <qgis/qgslocatorfilter.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;


class KADAS_GUI_EXPORT KadasLocationSearchFilter : public QgsLocatorFilter
{
    Q_OBJECT
  public:
    KadasLocationSearchFilter( QgsMapCanvas *mapCanvas );
    ~KadasLocationSearchFilter();

    virtual QgsLocatorFilter *clone() const override;
    QString name() const override { return QStringLiteral( "location-search" ); }
    QString displayName() const override { return tr( "Location Search" ); }
    virtual Priority priority() const override { return Priority::Medium; }
    virtual void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    virtual void triggerResult( const QgsLocatorResult &result ) override;
    virtual void clearPreviousResults() override;

  private:
    QgsMapCanvas *mMapCanvas = nullptr;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;

    QMap<QString, QPair<QString, int>> mCategoryMap;
    QRegExp mPatBox;

    QString mPinItemId;
    QString mGeometryItemId;
};

#endif // KADASLOCATIONSEARCHPROVIDER_H
