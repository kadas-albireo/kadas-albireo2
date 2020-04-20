/***************************************************************************
    kadasmapcanvasitemmanager.h
    ---------------------------
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

#ifndef KADASMAPCANVASITEMMANAGER_H
#define KADASMAPCANVASITEMMANAGER_H

#include <QObject>

#include <qgis/qgis_sip.h>

#include <kadas/gui/kadas_gui.h>

class QDomDocument;

class QgsMapCanvas;

class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapCanvasItemManager : public QObject
{
    Q_OBJECT

  public:
    static KadasMapCanvasItemManager *instance();
    static void addItem( KadasMapItem *item );
    static void removeItem( KadasMapItem *item );
    static void removeItemAfterRefresh( KadasMapItem *item, QgsMapCanvas *canvas );
    static const QList<KadasMapItem *> &items();
    static void clear();

  signals:
    void itemAdded( const KadasMapItem *item );
    void itemWillBeRemoved( const KadasMapItem *item );

  private:
    KadasMapCanvasItemManager() SIP_FORCE;

    QList<KadasMapItem *> mMapItems;

  private slots:
    void itemAboutToBeDestroyed();
    void readFromProject( const QDomDocument &doc );
    void writeToProject( QDomDocument &doc );
};

#endif // KADASMAPCANVASITEMMANAGER_H
