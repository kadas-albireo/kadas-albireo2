/***************************************************************************
    kadaslayerrefreshmanager.h
    --------------------------
    copyright            : (C) 2022 by Sandro Mani
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

#ifndef KADASLAYERREFRESHMANAGER_H
#define KADASLAYERREFRESHMANAGER_H

class QDomDocument;
class QTimer;

#include <QObject>

class KadasLayerRefreshManager : public QObject
{
    Q_OBJECT
  public:
    KadasLayerRefreshManager( QObject *parent = nullptr );
    void setLayerRefreshInterval( const QString &layerId, int refreshIntervalSec );
    int layerRefreshInterval( const QString &layerId ) const;

  private:
    QMap<QString, QTimer *> mLayerTimers;

  private slots:
    void clear();
    void clearLayer( const QString &layerId );
    void writeProjectSettings( QDomDocument &doc );
    void readProjectSettings( const QDomDocument &doc );
};

#endif // KADASLAYERREFRESHMANAGER_H
