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
#include "kadas/gui/kadasbottombar.h"

class QProgressBar;
class KadasRibbonButton;


class KadasPluginManagerInstallButton : public QWidget
{
    Q_OBJECT
  public:
    enum Status
    {
      Install,
      Installing,
      Uninstall,
      Uninstalling,
      Update
    };
    KadasPluginManagerInstallButton( Status status, QWidget *parent = nullptr );
    void setStatus( Status status, int progress = 0 );
    Status status() const { return mStatus; }

  signals:
    void clicked();

  private:
    Status mStatus = Status::Install;
    QPushButton *mButton = nullptr;
    QProgressBar *mProgressbar = nullptr;
};


class KadasPluginManager : public KadasBottomBar, private Ui::KadasPluginManagerBase
{
    Q_OBJECT
  public:
    KadasPluginManager( QgsMapCanvas *canvas, QAction *action );

    void loadPlugins();
    void installMandatoryPlugins();
    void updateAllPlugins();

  private slots:
    void installButtonClicked();
    void updateButtonClicked();
    void on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column );
    void on_mCloseButton_clicked();

  private:
    const int INSTALLED_TREEWIDGET_COLUMN_NAME = 0;
    const int INSTALLED_TREEWIDGET_COLUMN_VERSION = 1;

    const int AVAILABLE_TREEWIDGET_COLUMN_NAME = 0;
    const int AVAILABLE_TREEWIDGET_COLUMN_BUTTON = 1;

    const char *PROPERTY_PLUGIN_NAME = "PluginName";

    struct PluginInfo
    {
        QString name;
        QString description;
        QString version;
        QString downloadLink;
        bool mandatory;
    };

    KadasPluginManager();

    QMap<QString, PluginInfo> mAvailablePlugins;

    QMap<QString, PluginInfo> availablePlugins();
    bool installPlugin( const QString &pluginName, const QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion, KadasPluginManagerInstallButton *b );
    bool uninstallPlugin( const QString &pluginName, const QString &moduleName, KadasPluginManagerInstallButton *b );
    bool updatePlugin( const QString &pluginName, const QString &moduleName, const QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion, KadasPluginManagerInstallButton *b );

    //tree widget state
    void setItemInstallable( QTreeWidgetItem *item, const QString &version );
    void setItemRemoveable( QTreeWidgetItem *item );
    void changeItemInstallationState( QTreeWidgetItem *item, KadasPluginManagerInstallButton::Status buttonStatus );
    void setItemActivatable( QTreeWidgetItem *item );
    void setItemDeactivatable( QTreeWidgetItem *item );
    void setItemUpdateable( QTreeWidgetItem *item );
    void setItemNotUpdateable( QTreeWidgetItem *item );

    static bool updateAvailable( const QString &installedVersion, const QString &repoVersion );

    KadasPluginManagerInstallButton *getInstallButton( const QString &pluginName );

    //main window button to show/hide the plugin manager
    QAction *mAction;
};
