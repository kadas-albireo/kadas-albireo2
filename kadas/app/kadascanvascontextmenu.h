/***************************************************************************
    kadascanvascontextmenu.h
    ------------------------
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

#ifndef KADASCANVASCONTEXTMENU_H
#define KADASCANVASCONTEXTMENU_H

#include <QMenu>

#include <qgis/qgspointxy.h>

#include "kadas/gui/kadasfeaturepicker.h"

class QgsGeometryRubberBand;
class KadasItemContextMenuActions;
class KadasSelectionRectItem;


class KadasCanvasContextMenu : public QMenu
{
    Q_OBJECT
  public:

    static const QString ACTION_PROPERTY_MAP_POSITION;

    enum class Menu
    {
      NONE,
      DRAW,
      MEASURE,
      TERRAIN_ANALYSIS
    };

    KadasCanvasContextMenu( QgsMapCanvas *canvas, const QgsPointXY &mapPos );
    ~KadasCanvasContextMenu();

    static void registerAction( QAction *action, Menu insertMenu = Menu::TERRAIN_ANALYSIS );
    static void unRegisterAction( QAction *action );

  private slots:
    void copyCoordinates();
    void copyMap();

    void copyFeature();
    void deleteItems();
    void editItem();
    void raiseItem();
    void lowerItem();
    void paste();

    void drawPin();
    void drawPointMarker();
    void drawSquareMarker();
    void drawTriangleMarker();
    void drawLine();
    void drawRectangle();
    void drawPolygon();
    void drawCircle();
    void drawText();
    void drawCoordinateCross();

    void identify();

    void measureLine();
    void measurePolygon();
    void measureCircle();
    void measureHeightProfile();
    void measureMinMax();
    void terrainSlope();
    void terrainHillshade();
    void terrainViewshed();

    void print();

  private:
    KadasItemContextMenuActions *mItemActions = nullptr;
    QgsPointXY mMapPos;
    QgsMapCanvas *mCanvas = nullptr;
    KadasFeaturePicker::PickResult mPickResult;
    QgsGeometryRubberBand *mGeomSel = nullptr;
    KadasSelectionRectItem *mSelRect = nullptr;

    static QMap<QAction *, Menu> sRegisteredActions;
};

#endif // KADASCANVASCONTEXTMENU_H
