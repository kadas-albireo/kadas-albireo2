/***************************************************************************
    kadaslayertreeviewmenuprovider.h
    --------------------------------
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

#ifndef KADASLAYERTREEVIEWMENUPROVIDER_H
#define KADASLAYERTREEVIEWMENUPROVIDER_H

#include <qgis/qgslayertreeview.h>

class KadasMainWindow;

class KadasLayerTreeViewMenuProvider: public QObject, public QgsLayerTreeViewMenuProvider
{
    Q_OBJECT
  public:
    KadasLayerTreeViewMenuProvider( QgsLayerTreeView* view, KadasMainWindow* mainWindow );
    QMenu* createContextMenu() override;

  private:
    QAction* actionLayerTransparency(QMenu *parent);
    QAction* actionLayerUseAsHeightmap(QMenu *parent);

    QgsLayerTreeView* mView = nullptr;
    KadasMainWindow* mMainWindow = nullptr;

private slots:
    void setLayerTransparency(int value);
    void setLayerUseAsHeightmap(bool enabled);
    void showLayerAttributeTable();
    void showLayerInfo();
    void showLayerProperties();
};

#endif // KADASLAYERTREEVIEWMENUPROVIDER_H
