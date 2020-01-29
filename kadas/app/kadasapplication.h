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

#include <qgis/qgis.h>
#include <qgis/qgsapplication.h>

#include <kadas/gui/kadasfeaturepicker.h>

class QgsMapLayer;
class QgsMapLayerConfigWidgetFactory;
class QgsMapTool;
class KadasMessageLogViewer;
class QgsMessageOutput;
class QgsPrintLayout;
class QgsRasterLayer;
class QStackedWidget;
class QgsVectorLayer;
class KadasClipboard;
class KadasGpxIntegration;
class KadasMainWindow;
class KadasMapToolPan;
class KadasPythonIntegration;
class KadasPluginInterface;
class KadasRedliningIntegration;

#define kApp KadasApplication::instance()

class KadasApplication : public QgsApplication
{
    Q_OBJECT

  public:

    static KadasApplication *instance();
    static bool isRunningFromBuildDir();

    KadasApplication( int &argc, char **argv );

    void init();

    KadasMainWindow *mainWindow() const { return mMainWindow; }
    KadasPythonIntegration *pythonIntegration() { return mPythonIntegration; }

    QgsRasterLayer *addRasterLayer( const QString &uri, const QString &baseName, const QString &providerKey ) const;
    QgsVectorLayer *addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey ) const;
    void addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType )  const;
    QPair<KadasMapItem *, KadasItemLayerRegistry::StandardLayer> addImageItem( const QString &filename ) const;
    KadasItemLayer *selectItemLayer();

    bool projectNew( bool askToSave );
    bool projectCreateFromTemplate( const QString &templateFile );
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

  public slots:
    void displayMessage( const QString &message, Qgis::MessageLevel level = Qgis::Info );
    void unsetMapTool();

  signals:
    void projectWillBeClosed();
    void projectRead();
    void activeLayerChanged( QgsMapLayer *layer );
    void printLayoutAdded( QgsPrintLayout *layout );
    void printLayoutWillBeRemoved( QgsPrintLayout *layout );

  private:
    KadasPluginInterface *mPythonInterface = nullptr;
    KadasPythonIntegration *mPythonIntegration = nullptr;
    KadasMainWindow *mMainWindow = nullptr;
    KadasMessageLogViewer *mMessageLogViewer = nullptr;
    bool mBlockActiveLayerChanged = false;
    KadasMapToolPan *mMapToolPan = nullptr;
    QList<QgsMapLayerConfigWidgetFactory *> mMapLayerPanelFactories;
    QTimer mAutosaveTimer;
    bool mAutosaving = false;

    QList<QgsMapLayer *> showGDALSublayerSelectionDialog( QgsRasterLayer *layer ) const;
    QList<QgsMapLayer *> showOGRSublayerSelectionDialog( QgsVectorLayer *layer ) const;
    void loadPythonSupport();
    bool showZipSublayerSelectionDialog( const QString &path ) const;
    QString migrateDatasource( const QString &path ) const;
    QMap<QString, QString> dataSourceMigrationMap() const;
    void cleanupAutosave();
    int hideDialogPanel( const QString &name, QStackedWidget *stackedWidget );

    static QgsMessageOutput *messageOutputViewer();

  private slots:
    void autosave();
    void onActiveLayerChanged( QgsMapLayer *layer );
    void onFocusChanged( QWidget * /*old*/, QWidget *now );
    void onMapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool );
    void handleItemPicked( const KadasFeaturePicker::PickResult &result );
    void projectDirtySet();
    void showCanvasContextMenu( const QPoint &screenPos, const QgsPointXY &mapPos );
    void updateWindowTitle();
    void cleanup();
    void updateWmtsZoomResolutions() const;
    void unsetMapToolOnSave();
};

#endif // KADASAPPLICATION_H
