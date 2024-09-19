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

#include <qgis/qgslocatorfilter.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;
class QgsFeature;
class QgsMapLayer;
class QgsVectorLayer;

class KADAS_GUI_EXPORT KadasLocalDataSearchFilter : public QgsLocatorFilter
{
    Q_OBJECT
  public:
    KadasLocalDataSearchFilter( QgsMapCanvas *mapCanvas );

    virtual QgsLocatorFilter *clone() const override;
    QString name() const override { return QStringLiteral( "local-data" ); }
    QString displayName() const override { return tr( "Local data" ); }
    virtual Priority priority() const override { return Priority::Medium; }
    virtual void fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback ) override;
    virtual void triggerResult( const QgsLocatorResult &result ) override;


  private:
    void buildResult( const QgsFeature &feature, QgsVectorLayer *layer, const QString &searchText );
    QgsMapCanvas *mMapCanvas = nullptr;
    static const int sResultCountLimit;
};


#endif // KADASLOCALDATASEARCHPROVIDER_H
