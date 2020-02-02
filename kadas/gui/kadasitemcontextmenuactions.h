/***************************************************************************
    kadasitemcontextmenuactions.h
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

#ifndef KADASITEMCONTEXTMENUACTIONS_H
#define KADASITEMCONTEXTMENUACTIONS_H

#include <QObject>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasitemlayer.h>

class QMenu;
class QgsMapCanvas;
class KadasItemLayer;
class KadasCircleItem;
class KadasMapItem;
class KadasPinItem;
class KadasPointItem;

class KADAS_GUI_EXPORT KadasItemContextMenuActions : public QObject
{
    Q_OBJECT
  public:
    enum ItemParent { ParentIsTool, ParentIsLayer };
    KadasItemContextMenuActions( QgsMapCanvas *canvas, QMenu *menu, KadasMapItem *item, KadasItemLayer *layer, KadasItemLayer::ItemId layerItemId = KadasItemLayer::ITEM_ID_NULL, QObject *parent = nullptr );

  private slots:
    void copyItem();
    void cutItem();
    void deleteItem();
    void copyItemPosition();
    void createPolygonFromCircle();
    void createPinFromPoint();
    void createWaypointFromPin();

  private:
    QgsMapCanvas *mCanvas = nullptr;
    KadasMapItem *mItem = nullptr;
    KadasItemLayer *mLayer = nullptr;
    KadasItemLayer::ItemId mLayerItemId = KadasItemLayer::ITEM_ID_NULL;
};

#endif // KADASITEMCONTEXTMENUACTIONS_H
