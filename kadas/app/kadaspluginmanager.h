#include "ui_kadaspluginmanager.h"
#include <kadas/gui/kadasbottombar.h>

class KadasPluginManager: public KadasBottomBar, private Ui::KadasPluginManagerBase
{
    Q_OBJECT
    public:
        KadasPluginManager( QgsMapCanvas* canvas );

    private slots:
        void on_mAvailableTreeWidget_itemClicked( QTreeWidgetItem *item, int column );
        void on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column );

    private:
        KadasPluginManager();

        /**plugin name, download link*/
        QMap< QString, QString > mAvailablePlugins;

        QMap< QString, QString > availablePlugins();
        void installPlugin( const QString& pluginName, const  QString& downloadUrl ) const;
        void uninstallPlugin( const QString& pluginName, const QString& moduleName );
        void activatePlugin( const QString& pluginName ) const;
        void deactivatePlugin( const QString& pluginName ) const;
};
