/***************************************************************************
    kadasremotedatasearchprovider.h
    -------------------------------
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


#ifndef KADASREMOTEDATASEARCHPROVIDER_H
#define KADASREMOTEDATASEARCHPROVIDER_H

#include <QMap>
#include <QRegExp>
#include <QTimer>

#include <qgis/qgslocatorfilter.h>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;


class KADAS_GUI_EXPORT KadasRemoteDataSearchProvider : public QgsLocatorFilter
{
    Q_OBJECT
  public:
    KadasRemoteDataSearchProvider( QgsMapCanvas *mapCanvas );


    virtual QgsLocatorFilter *clone() const override;
    QString name() const override { return QStringLiteral( "remote-data" ); }
    QString displayName() const override { return tr( "Remote Data" ); }
    virtual Priority priority() const override { return Priority::Medium; }
    virtual void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    virtual void triggerResult( const QgsLocatorResult &result ) override;
    virtual void clearPreviousResults() override;

  private:
    static const int sSearchTimeout;
    static const int sResultCountLimit;
    static const QByteArray sGeoAdminUrl;

    QgsMapCanvas *mMapCanvas = nullptr;
    QString mPinItemId;
    QRegExp mPatBox;

};

#endif // KADASREMOTEDATASEARCHPROVIDER_H
