/***************************************************************************
    kadaspluginmanager.h
    --------------------
    copyright            : (C) 2019 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ui_kadaspluginmanager.h"
#include <kadas/gui/kadasbottombar.h>

class KadasRibbonButton;

class KadasPluginManager: public KadasBottomBar, private Ui::KadasPluginManagerBase
{
    Q_OBJECT
  public:
    KadasPluginManager( QgsMapCanvas *canvas, QAction* action );

  private slots:
    void installButtonClicked();
    void on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column );
    void on_mCloseButton_clicked();

  private:
    KadasPluginManager();

    /**plugin name, pair<description, download link>*/
    QMap< QString, QPair< QString, QString> > mAvailablePlugins;

    QMap< QString, QPair< QString, QString > > availablePlugins();
    void installPlugin( const QString &pluginName, const  QString &downloadUrl, const QString &pluginTooltip );
    void uninstallPlugin( const QString &pluginName, const QString &moduleName );

    //tree widget state
    void setItemInstallable( QTreeWidgetItem *item );
    void setItemRemoveable( QTreeWidgetItem *item );
    void changeItemInstallationState( QTreeWidgetItem *item, const QString &buttonText );
    void setItemActivatable( QTreeWidgetItem *item );
    void setItemDeactivatable( QTreeWidgetItem *item );

    //main window button to show/hide the plugin manager
    QAction* mAction;
};
