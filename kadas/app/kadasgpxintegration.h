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

class QAction;

class KadasMapItem;
class KadasItemLayer;
class KadasMainWindow;

class KadasGpxIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasGpxIntegration( QAction *actionWaypoint, QAction *actionRoute, QAction *actionExportGpx, QAction *actionImportGpx, KadasMainWindow *parent );
    KadasItemLayer *getOrCreateLayer();

  private:
    void toggleCreateItem( bool active, const std::function<KadasMapItem*() > &itemFactory );

  private slots:
    void importGpx();
    void exportGpx();
};

#endif // KADASGPXINTEGRATION_H
