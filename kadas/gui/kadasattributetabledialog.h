/***************************************************************************
    kadasattributetabledialog.h
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

#ifndef KADASATTRIBUTETABLEDIALOG_H
#define KADASATTRIBUTETABLEDIALOG_H

#include <QDockWidget>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;
class QgsMessageBar;
class QgsVectorLayer;
class QgsVectorLayerSelectionManager;


class KADAS_GUI_EXPORT KadasAttributeTableDialog : public QDockWidget
{
    Q_OBJECT

  public:
    KadasAttributeTableDialog( QgsVectorLayer *layer, QgsMapCanvas *canvas, QgsMessageBar *messageBar, QMainWindow *parent = nullptr );
    ~KadasAttributeTableDialog();
    QgsMapCanvas *mCanvas = nullptr;
    QgsMessageBar *mMessageBar = nullptr;

  protected:
    void showEvent( QShowEvent *ev ) override;

  private:
    QgsVectorLayerSelectionManager *mFeatureSelectionManager = nullptr;


  private slots:
    void storeDockLocation( Qt::DockWidgetArea area );
    void deselectAll();
    void invertSelection();
    void panToSelected();
    void selectAll();
    void selectByExpression();
    void zoomToSelected();
};

#endif // KADASATTRIBUTETABLEDIALOG_H
