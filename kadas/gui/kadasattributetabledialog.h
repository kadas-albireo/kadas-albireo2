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
#include <kadas/core/kadassettingstree.h>

#include <qgis/qgssettingsentryenumflag.h>

class QDomElement;
class QDomDocument;

class QgsMapCanvas;
class QgsMessageBar;
class QgsVectorLayer;
class QgsVectorLayerSelectionManager;
class QgsDockableWidgetHelper;


class KADAS_GUI_EXPORT KadasAttributeTableDialog : public QDockWidget
{
    Q_OBJECT

  public:
    static const inline QgsSettingsEntryEnumFlag<Qt::DockWidgetArea> *settingsAttributeTableLocation = new QgsSettingsEntryEnumFlag<Qt::DockWidgetArea>(QStringLiteral("attribute-dock-location"), KadasSettingsTree::sTreeApp, Qt::DockWidgetArea::NoDockWidgetArea );

    KadasAttributeTableDialog( QgsVectorLayer *layer, QgsMapCanvas *canvas, QgsMessageBar *messageBar, QMainWindow *parent = nullptr, Qt::DockWidgetArea area = Qt::DockWidgetArea::RightDockWidgetArea );
    ~KadasAttributeTableDialog();

    QDomElement writeXml( QDomDocument &document );
    static void createFromXml(const QDomElement &element , QgsMapCanvas *canvas, QgsMessageBar *messageBar, QMainWindow *parent);

  protected:
    void showEvent( QShowEvent *ev ) override;

  private:
    QgsMapCanvas *mCanvas = nullptr;
    QgsMessageBar *mMessageBar = nullptr;
    QgsVectorLayer* mLayer = nullptr;
    QgsVectorLayerSelectionManager *mFeatureSelectionManager = nullptr;
    QMainWindow* mMainWindow = nullptr;

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
