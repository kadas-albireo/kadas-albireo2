/***************************************************************************
    kadasmaptoolguidegrid.h
    -----------------------
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

#ifndef KADASMAPTOOLGUIDEGRID_H
#define KADASMAPTOOLGUIDEGRID_H

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmaptool.h>

#include <kadas/gui/kadasbottombar.h>
#include "ui_kadasguidegridwidgetbase.h"

class QgsLayerTreeView;
class KadasGuideGridLayer;
class KadasGuideGridWidget;
class KadasLayerSelectionWidget;


class KadasMapToolGuideGrid : public QgsMapTool
{
    Q_OBJECT

  public:
    enum PickMode {PICK_NONE, PICK_TOP_LEFT, PICK_BOTTOM_RIGHT};
    KadasMapToolGuideGrid( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer );
    ~KadasMapToolGuideGrid();

    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    KadasGuideGridWidget *mWidget = nullptr;
    PickMode mPickMode = PICK_NONE;
    QAction *mActionEditLayer = nullptr;

  private slots:
    void setPickMode( PickMode pickMode );
    void close();
};

class KadasGuideGridWidget : public KadasBottomBar
{
    Q_OBJECT

  public:
    KadasGuideGridWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer );
    void pointPicked( KadasMapToolGuideGrid::PickMode pickMode, const QgsPointXY &pos );

    QgsMapLayer *createLayer( QString layerName = QString() );

  private:
    QgsCoordinateReferenceSystem mCrs;
    QgsRectangle mCurRect;
    Ui::KadasGuideGridWidgetBase ui;
    KadasLayerSelectionWidget *mLayerSelectionWidget = nullptr;
    KadasGuideGridLayer *mCurrentLayer = nullptr;

    void updateGrid();

  signals:
    void requestPick( KadasMapToolGuideGrid::PickMode pickMode );
    void close();

  private slots:
    void setCurrentLayer( QgsMapLayer *layer );
    void topLeftEdited();
    void bottomRightEdited();
    void switchLabels();
    void updateIntervals();
    void updateBottomRight();
    void updateLockIcon( bool locked );
    void updateColor( const QColor &color );
    void updateLineWidth( int width );
    void updateFontSize( int fontSize );
    void updateLabeling();
    void pickTopLeftPos() { emit requestPick( KadasMapToolGuideGrid::PICK_TOP_LEFT ); }
    void pickBottomRight() { emit requestPick( KadasMapToolGuideGrid::PICK_BOTTOM_RIGHT ); }
};

#endif // KADASMAPTOOLGUIDEGRID_H
