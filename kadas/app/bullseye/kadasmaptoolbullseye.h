/***************************************************************************
    kadasmaptoolbullseye.h
    ----------------------
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

#ifndef KADASMAPTOOLBULLSEYE_H
#define KADASMAPTOOLBULLSEYE_H

#include <qgis/qgsmaptool.h>

#include <kadas/gui/kadasbottombar.h>
#include "ui_kadasbullseyewidgetbase.h"

class QgsLayerTreeView;
class KadasBullseyeLayer;
class KadasBullseyeWidget;
class KadasLayerSelectionWidget;

class KadasMapToolBullseye : public QgsMapTool
{
    Q_OBJECT
  public:
    KadasMapToolBullseye( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer = nullptr );
    ~KadasMapToolBullseye();

    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    KadasBullseyeWidget *mWidget;
    bool mPicking = false;

  private slots:
    void setPicking( bool picking = true );
    void close();
};


class KadasBullseyeWidget : public KadasBottomBar
{
    Q_OBJECT

  public:
    KadasBullseyeWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer );
    void centerPicked( const QgsPointXY &pos );

  public slots:
    KadasBullseyeLayer *createLayer( QString layerName );

  private:
    Ui::KadasBullseyeWidgetBase ui;
    KadasLayerSelectionWidget *mLayerSelectionWidget;
    KadasBullseyeLayer *mCurrentLayer = nullptr;

    void updateGrid();

  signals:
    void requestPickCenter( bool active = true );
    void close();

  private slots:
    void setCurrentLayer( QgsMapLayer *layer );
    void updateLayer();
    void updateColor( const QColor &color );
    void updateFontSize( int fontSize );
    void updateLabeling();
    void updateLineWidth( int width );
};

#endif // KADASMAPTOOLBULLSEYE_H
