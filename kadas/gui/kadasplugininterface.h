/***************************************************************************
    kadasplugininterface.h
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

#ifndef KADASPLUGININTERFACE_H
#define KADASPLUGININTERFACE_H

#include <qgis/qgisinterface.h>

#include <kadas/gui/kadas_gui.h>

class QgsMapTool;
class QgsPrintLayout;

class KADAS_GUI_EXPORT KadasPluginInterface : public QgisInterface
{
    Q_OBJECT
  public:
    static KadasPluginInterface *cast( QgisInterface *iface ) { return dynamic_cast<KadasPluginInterface *>( iface ); }
    // KADAS specific interface
    enum ActionRibbonTabLocation
    {
      NO_TAB,
      MAPS_TAB,
      VIEW_TAB,
      ANALYSIS_TAB,
      DRAW_TAB,
      GPS_TAB,
      MSS_TAB,
      SETTINGS_TAB,
      HELP_TAB,
      CUSTOM_TAB
    };
    enum ActionClassicMenuLocation
    {
      NO_MENU,
      PROJECT_MENU,
      EDIT_MENU,
      VIEW_MENU,
      LAYER_MENU,
      NEWLAYER_MENU,
      ADDLAYER_MENU,
      SETTINGS_MENU,
      PLUGIN_MENU,
      RASTER_MENU,
      DATABASE_MENU,
      VECTOR_MENU,
      WEB_MENU,
      FIRST_RIGHT_STANDARD_MENU,
      WINDOW_MENU,
      HELP_MENU,
      CUSTOM_MENU
    };

    virtual QMenu *getClassicMenu( ActionClassicMenuLocation classicMenuLocation, const QString &customName = QString() ) = 0;
    virtual QMenu *getSubMenu( QMenu *menu, const QString &submenuName ) = 0;
    virtual QWidget *getRibbonWidget() = 0;
    virtual QWidget *getRibbonTabWidget( ActionRibbonTabLocation ribbonTabLocation, const QString &customName ) = 0;

    //! Generic action adder
    virtual void addAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString(), QgsMapTool *associatedMapTool = nullptr ) = 0;
    virtual void addActionMenu( const QString &text, const QIcon &icon, QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString() ) = 0;

    //! Generic action remover
    virtual void removeAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString(), QgsMapTool *associatedMapTool = nullptr ) = 0;
    virtual void removeActionMenu( QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString() ) = 0;

    //! Generic action finder
    virtual QAction *findAction( const QString &name ) = 0;

    //! Generic object finder
    virtual QObject *findObject( const QString &name ) = 0;

    virtual QgsPrintLayout *createNewPrintLayout( const QString &title ) = 0;
    virtual bool deletePrintLayout( QgsPrintLayout *layout ) = 0;
    virtual QList<QgsPrintLayout *> printLayouts() const = 0;
    virtual void showLayoutDesigner( QgsPrintLayout *layout ) = 0;

    virtual bool saveProject() = 0;

    virtual QgsVectorLayer *addVectorLayerQuiet( const QString &vectorLayerPath, const QString &baseName, const QString &providerKey ) = 0;
    virtual QgsRasterLayer *addRasterLayerQuiet( const QString &url, const QString &layerName, const QString &providerKey ) = 0;

  signals:
    void printLayoutAdded( QgsPrintLayout *layout );
    void printLayoutWillBeRemoved( QgsPrintLayout *layout );
    void projectWillBeClosed();
    void mainWindowClosed();
};


#endif // KADASPLUGININTERFACE_H
