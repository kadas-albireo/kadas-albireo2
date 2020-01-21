/***************************************************************************
    kadasgpxintegration.h
    ---------------------
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

#ifndef KADASGPXINTEGRATION_H
#define KADASGPXINTEGRATION_H

#include <functional>

#include <QObject>

#include <qgis/qgscustomdrophandler.h>

class QAction;

class KadasMapItem;
class KadasItemLayer;
class KadasMainWindow;


class KadasGpxDropHandler : public QgsCustomDropHandler
{
    Q_OBJECT

  public:
    bool canHandleMimeData( const QMimeData *data ) override;
    bool handleMimeDataV2( const QMimeData *data ) override;
};

class KadasGpxIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasGpxIntegration( QAction *actionWaypoint, QAction *actionRoute, QAction *actionExportGpx, QAction *actionImportGpx, QObject *parent );
    ~KadasGpxIntegration();
    KadasItemLayer *getOrCreateLayer();

    static bool importGpx( const QString &filename, QString &errorMsg );

  private:
    void toggleCreateItem( bool active, const std::function<KadasMapItem*() > &itemFactory );

    KadasGpxDropHandler mDropHandler;

  private slots:
    void openGpx();
    void saveGpx();
};

#endif // KADASGPXINTEGRATION_H
