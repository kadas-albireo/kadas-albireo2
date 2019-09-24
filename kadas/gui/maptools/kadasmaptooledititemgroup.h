/***************************************************************************
    kadasmaptooledititemgroup.h
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

#ifndef KADASMAPTOOLEDITITEMGROUP_H
#define KADASMAPTOOLEDITITEMGROUP_H

#include <qgis/qgsmaptool.h>
#include <qgis/qgspoint.h>

#include <kadas/gui/kadas_gui.h>

class QLabel;
class KadasBottomBar;
class KadasItemLayer;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapToolEditItemGroup : public QgsMapTool
{
  public:
    KadasMapToolEditItemGroup( QgsMapCanvas *canvas, const QList<KadasMapItem *> &items, KadasItemLayer *layer );

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

  private:
    QList<KadasMapItem *> mItems;
    KadasItemLayer *mLayer;
    QgsPointXY mMoveRefPos;
    QList<QgsPointXY> mItemRefPos;
    KadasBottomBar *mBottomBar = nullptr;
    QLabel *mStatusLabel = nullptr;

    void copyItems();
    void cutItems();
    void deleteItems();
    void deselectItem( KadasMapItem *item, bool triggerRepaint = true );
    void updateStatusLabel();
};

#endif // KADASMAPTOOLEDITITEMGROUP_H
