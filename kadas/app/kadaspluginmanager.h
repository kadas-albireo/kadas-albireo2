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
    KadasPluginManager( QgsMapCanvas *canvas, QAction *action );

  private slots:
    void installButtonClicked();
    void updateButtonClicked();
    void on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column );
    void on_mCloseButton_clicked();

  private:

    struct PluginInfo
    {
      QString name;
      QString description;
      QString version;
      QString downloadLink;
    };

    KadasPluginManager();

    /**plugin name, pair<description, download link>*/
    QMap< QString, PluginInfo > mAvailablePlugins;

    QMap< QString, PluginInfo > availablePlugins();
    bool installPlugin( const QString &pluginName, const  QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion );
    bool uninstallPlugin( const QString &pluginName, const QString &moduleName );
    void updatePlugin( const QString &pluginName, const QString &moduleName, const  QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion );

    //tree widget state
    void setItemInstallable( QTreeWidgetItem *item, const QString &version );
    void setItemRemoveable( QTreeWidgetItem *item );
    void changeItemInstallationState( QTreeWidgetItem *item, const QString &buttonText );
    void setItemActivatable( QTreeWidgetItem *item );
    void setItemDeactivatable( QTreeWidgetItem *item );
    void setItemUpdateable( QTreeWidgetItem *item );
    void setItemNotUpdateable( QTreeWidgetItem *item );

    static bool updateAvailable( const QString &installedVersion, const QString &repoVersion );

    //main window button to show/hide the plugin manager
    QAction *mAction;
};
