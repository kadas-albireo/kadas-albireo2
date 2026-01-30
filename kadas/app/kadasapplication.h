/***************************************************************************
    kadasapplication.cpp
    --------------------
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

#ifndef KADASAPPLICATION_H
#define KADASAPPLICATION_H

#include <QDateTime>
#include <QTimer>

#include <qgis/qgis.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgslayertreeregistrybridge.h>

#include "kadas/gui/kadasfeaturepicker.h"

class QNetworkRequest;
class QTemporaryDir;

class QStackedWidget;
class QgsLayerTreeGroup;
class QgsMapLayer;
class QgsMapLayerConfigWidgetFactory;
class QgsMapTool;
class QgsMessageOutput;
class QgsPointCloudLayer;
class QgsPrintLayout;
class QgsRasterLayer;
class QgsVectorLayer;
class QgsVectorTileLayer;

class KadasClipboard;
class KadasGpxIntegration;
class KadasLayerRefreshManager;
class KadasMainWindow;
class KadasMapToolPan;
class KadasMessageLogViewer;
class KadasPluginInterface;
class KadasPortalAuth;
class KadasPythonIntegration;
class KadasRedliningIntegration;

#define kApp KadasApplication::instance()

class KadasApplication : public QgsApplication
{
    Q_OBJECT

  public:
    static KadasApplication *instance();
    static bool isRunningFromBuildDir();

    KadasApplication( int &argc, char **argv );
    ~KadasApplication();

    void init();

    KadasMainWindow *mainWindow() const { return mMainWindow; }
    KadasPythonIntegration *pythonIntegration() const { return mPythonIntegration; }
    KadasLayerRefreshManager *layerRefreshManager() const { return mLayerRefreshManager; }

    QgsRasterLayer *addRasterLayer( const QString &uri, const QString &baseName, const QString &providerKey, bool quiet = false, int insOffset = 0, bool adjustInsertionPoint = true ) const;
    QgsVectorLayer *addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey, bool quiet = false, int insOffset = 0, bool adjustInsertionPoint = true ) const;
    void addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType, bool quiet = false ) const;
    void addRasterLayers( const QStringList &layerUris, bool quiet = false ) const;
    QgsVectorTileLayer *addVectorTileLayer( const QString &url, const QString &baseName, bool quiet = false, bool forceUpdateUriSources = true );
    QgsPointCloudLayer *addPointCloudLayer( const QString &uri, const QString &baseName, const QString &providerKey, bool quiet = false );
    QPair<KadasMapItem *, KadasItemLayerRegistry::StandardLayer> addImageItem( const QString &filename ) const;
    KadasItemLayer *selectPasteTargetItemLayer( const QList<KadasMapItem *> &items );
    bool askUserForDatumTransform( const QgsCoordinateReferenceSystem &sourceCrs, const QgsCoordinateReferenceSystem &destinationCrs, const QgsMapLayer *layer );
    bool checkTasksDependOnProject();

    bool projectNew( bool askToSave );
    bool projectCreateFromTemplate( const QString &templateFile, const QUrl &templateUrl );
    bool projectOpen( const QString &projectFile = QString() );
    void projectClose();
    bool projectSave( const QString &fileName = QString(), bool promptFileName = false );
    bool projectSaveDirty();

    void addDefaultPrintTemplates();

    void saveMapAsImage();
    void saveMapToClipboard();

    void showLayerAttributeTable( QgsMapLayer *layer );
    void showLayerProperties( QgsMapLayer *layer );
    void showLayerInfo( const QgsMapLayer *layer );
    void showMessageLog();

    QgsMapLayer *currentLayer() const;

    void registerMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory );
    void unregisterMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory );

    QgsPrintLayout *createNewPrintLayout( const QString &title = QString() );
    bool deletePrintLayout( QgsPrintLayout *layout );
    QList<QgsPrintLayout *> printLayouts() const;
    void showLayoutDesigner( QgsPrintLayout *layout );

    QgsMapTool *paste( QgsPointXY *mapPos = nullptr );

    int computeLayerGroupInsertionOffset( QgsLayerTreeGroup *group ) const;

    QgsLayerTreeRegistryBridge::InsertionPoint layerTreeInsertionPoint() const;

  public slots:
    void displayMessage( const QString &message, Qgis::MessageLevel level = Qgis::Info );
    void showPythonConsole();
    void unsetMapTool();

    void initAfterExec();

  signals:
    void projectWillBeClosed();
    void projectRead();
    void activeLayerChanged( QgsMapLayer *layer );
    void printLayoutAdded( QgsPrintLayout *layout );
    void printLayoutWillBeRemoved( QgsPrintLayout *layout );

  private:
    struct DataSourceMigrations
    {
        QMap<QString, QString> files;
        QMap<QString, QMap<QString, QPair<QString, QString>>> wms;
        QMap<QString, QString> ams;
        QList<QPair<QString, QString>> strings;
    };

    KadasPluginInterface *mPythonInterface = nullptr;
    KadasPythonIntegration *mPythonIntegration = nullptr;
    KadasMainWindow *mMainWindow = nullptr;
    KadasLayerRefreshManager *mLayerRefreshManager = nullptr;
    KadasMessageLogViewer *mMessageLogViewer = nullptr;
    bool mBlockActiveLayerChanged = false;
    KadasMapToolPan *mMapToolPan = nullptr;
    QList<QgsMapLayerConfigWidgetFactory *> mMapLayerPanelFactories;
    QTimer mAutosaveTimer;
    bool mAutosaving = false;
    QList<QgsPluginLayerType *> mKadasPluginLayerTypes;
    QTemporaryDir *mProjectTempDir = nullptr;
    KadasPortalAuth *mPortalAuth = nullptr;

    void loadPythonSupport();
    QString migrateDatasource( const QString &path ) const;
    DataSourceMigrations dataSourceMigrationMap() const;
    void cleanupAutosave();
    int dialogPanelIndex( const QString &name, QStackedWidget *stackedWidget );
    void mergeChildSettingsGroups( QgsSettings &settings, QgsSettings &newSettings );

    static QgsMessageOutput *messageOutputViewer();
    static void injectAuthToken( QNetworkRequest *request );


  private slots:
    void loadStartupProject();
    void autosave();
    void onActiveLayerChanged( QgsMapLayer *layer );
    void onFocusChanged( QWidget * /*old*/, QWidget *now );
    void onMapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool );
    void handleItemPicked( const KadasFeaturePicker::PickResult &result );
    void projectDirtySet();
    void showCanvasContextMenu( const QPoint &screenPos, const QgsPointXY &mapPos );
    void updateWindowTitle();
    void cleanup();
    void unsetMapToolOnSave();
    void extentChanged();
    void saveAttributeTableDocks( QDomDocument &doc );
    void restoreAttributeTables( const QDomDocument &doc );
};

#endif // KADASAPPLICATION_H
