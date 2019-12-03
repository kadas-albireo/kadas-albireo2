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

#include <kadas/gui/kadasfeaturepicker.h>

class QgsGeometryRubberBand;
class KadasSelectionRectItem;


class KadasCanvasContextMenu : public QMenu
{
    Q_OBJECT
  public:
    KadasCanvasContextMenu( QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPointXY &mapPos );
    ~KadasCanvasContextMenu();

  private slots:
    void convertPinToWaypoint();
    void convertWaypointToPin();
    void convertCircleToPolygon();
    void copyCoordinates( const QgsPointXY &mapPos );
    void copyItemPosition();
    void copyMap();

    void copyFeature();
    void copyItem();
    void cutItem();
    void deleteItem();
    void deleteItems();
    void editItem();
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

    void identify();

    void measureLine();
    void measurePolygon();
    void measureCircle();
    void measureAzimuth();
    void measureHeightProfile();
    void terrainSlope();
    void terrainHillshade();
    void terrainViewshed();

    void print();

  private:
    QgsPointXY mMapPos;
    QgsMapCanvas *mCanvas = nullptr;
    KadasFeaturePicker::PickResult mPickResult;
    QgsGeometryRubberBand *mGeomSel = nullptr;
    KadasSelectionRectItem *mSelRect = nullptr;
};

#endif // KADASCANVASCONTEXTMENU_H
