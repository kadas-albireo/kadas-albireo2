/***************************************************************************
    kadasglobewidget.h
    ------------------
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

#ifndef KADASGLOBEWIDGET_H
#define KADASGLOBEWIDGET_H

#include <QDockWidget>

class QMenu;

class KadasGlobeWidget : public QDockWidget
{
    Q_OBJECT
  public:
    KadasGlobeWidget( QAction *action3D, QWidget *parent = nullptr );
    QStringList getSelectedLayerIds() const;

  signals:
    void layersChanged();
    void showSettings();
    void refresh();
    void takeScreenshot();
    void syncExtent();

  private:
    QMenu *mLayerSelectionMenu = nullptr;

    void contextMenuEvent( QContextMenuEvent *e ) override;
    bool eventFilter( QObject *obj, QEvent *ev ) override;
    void buildLayerSelectionMenu( bool syncMainLayers );

  private slots:
    void updateLayerSelectionMenu() { buildLayerSelectionMenu( false ); }
};

#endif // KADASGLOBEWIDGET_H
