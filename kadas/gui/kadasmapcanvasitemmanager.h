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

#include <kadas/gui/kadas_gui.h>

class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapCanvasItemManager : public QObject
{
    Q_OBJECT

  public:
    static KadasMapCanvasItemManager *instance();
    static void addItem( const KadasMapItem *item );
    static void removeItem( const KadasMapItem *item );
    static const QList<const KadasMapItem *> &items();
    static void clear();

  signals:
    void itemAdded( const KadasMapItem *item );
    void itemWillBeRemoved( const KadasMapItem *item );

  private:
    KadasMapCanvasItemManager() {}

    QList<const KadasMapItem *> mMapItems;

  private slots:
    void itemAboutToBeDestroyed();
};

#endif // KADASMAPCANVASITEMMANAGER_H
