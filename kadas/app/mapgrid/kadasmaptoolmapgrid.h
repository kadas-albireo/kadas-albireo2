/***************************************************************************
    kadasmaptoolmapgrid.h
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

#ifndef KADASMAPTOOLMAPGRID_H
#define KADASMAPTOOLMAPGRID_H

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmaptool.h>

#include "kadas/gui/kadasbottombar.h"
#include "ui_kadasmapgridwidgetbase.h"

class QgsLayerTreeView;
class KadasMapGridLayer;
class KadasMapGridWidget;
class KadasLayerSelectionWidget;


class KadasMapToolMapGrid : public QgsMapTool
{
    Q_OBJECT

  public:
    KadasMapToolMapGrid( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer );
    ~KadasMapToolMapGrid();

    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    KadasMapGridWidget *mWidget = nullptr;
    QAction *mActionEditLayer = nullptr;

  private slots:
    void close();
};

class KadasMapGridWidget : public KadasBottomBar
{
    Q_OBJECT

  public:
    KadasMapGridWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer );

    QgsMapLayer *createLayer( QString layerName = QString() );

  private:
    Ui::KadasMapGridWidgetBase ui;
    KadasLayerSelectionWidget *mLayerSelectionWidget = nullptr;
    KadasMapGridLayer *mCurrentLayer = nullptr;
    QLabel *mCellSizeLabel = nullptr;
    QComboBox *mCellSizeCombo = nullptr;


  signals:
    void close();

  private slots:
    void setCurrentLayer( QgsMapLayer *layer );
    void updateGrid();
    void updateType( int idx, bool updateValues );
    void updateColor( const QColor &color );
    void updateFontSize( int fontSize );
    void updateLabeling( int labelingMode );
};

#endif // KADASMAPTOOLMAPGRID_H
