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
#include <kadas/gui/maptools/kadasmaptoolmeasure.h>

class QgsMapLayer;
class QgsMapLayerConfigWidgetFactory;
class QgsMapTool;
class QgsPrintLayout;
class QgsRasterLayer;
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
    ~KadasApplication();

    KadasMainWindow *mainWindow() const { return mMainWindow; }

    QgsRasterLayer *addRasterLayer( const QString &uri, const QString &baseName, const QString &providerKey ) const;
    QgsVectorLayer *addVectorLayer( const QString &uri, const QString &layerName, const QString &providerKey ) const;
    void addVectorLayers( const QStringList &layerUris, const QString &enc, const QString &dataSourceType )  const;
    KadasItemLayer *getItemLayer( const QString &layerName ) const;
    KadasItemLayer *getOrCreateItemLayer( const QString &layerName );
    KadasItemLayer *selectItemLayer();

    void exportToKml();
    void importFromKml();

    void projectNew( bool askToSave );
    bool projectCreateFromTemplate( const QString &templateFile );
    bool projectOpen( const QString &projectFile = QString() );
    void projectClose();
    bool projectSave( const QString &fileName = QString(), bool promptFileName = false );

    void saveMapAsImage();
    void saveMapToClipboard();

    void showLayerAttributeTable( const QgsMapLayer *layer );
    void showLayerProperties( QgsMapLayer *layer );
    void showLayerInfo( const QgsMapLayer *layer );

    QgsMapLayer *currentLayer() const;

    void registerMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory );
    void unregisterMapLayerPropertiesFactory( QgsMapLayerConfigWidgetFactory *factory );

    QgsPrintLayout *createNewPrintLayout( const QString &title );
    bool deletePrintLayout( QgsPrintLayout *layout );
    QList<QgsPrintLayout *> printLayouts() const;

    QgsMapTool *paste( QgsPointXY *mapPos = nullptr );
    QgsMapTool *addPictureTool() const;
    QgsMapTool *addPinTool() const;
    QgsMapTool *deleteItemsTool() const;
    QgsMapTool *measureTool( KadasMapToolMeasure::MeasureMode mode, const QgsAbstractGeometry *geom = nullptr, const QgsCoordinateReferenceSystem &geomCrs = QgsCoordinateReferenceSystem() ) const;
    QgsMapTool *measureHeightProfileTool( const QgsAbstractGeometry *geom = nullptr, const QgsCoordinateReferenceSystem &geomCrs = QgsCoordinateReferenceSystem() ) const;
    QgsMapTool *terrainHillshadeTool( const QgsRectangle &rect = QgsRectangle(), const QgsCoordinateReferenceSystem &rectCrs = QgsCoordinateReferenceSystem() ) const;
    QgsMapTool *terrainSlopeTool( const QgsRectangle &rect = QgsRectangle(), const QgsCoordinateReferenceSystem &rectCrs = QgsCoordinateReferenceSystem() ) const;
    QgsMapTool *terrainViewshedTool() const;

    KadasRedliningIntegration *redliningIntegration() const { return mRedliningIntegration; }
    KadasGpxIntegration *gpxIntegration() { return mGpxIntegration; }

  public slots:
    void displayMessage( const QString &message, Qgis::MessageLevel level = Qgis::Info );
    void unsetMapTool();

  signals:
    void projectRead();
    void activeLayerChanged( QgsMapLayer *layer );
    void printLayoutAdded( QgsPrintLayout *layout );
    void printLayoutWillBeRemoved( QgsPrintLayout *layout );

  private:
    KadasPluginInterface *mPythonInterface = nullptr;
    KadasPythonIntegration *mPythonIntegration = nullptr;
    KadasMainWindow *mMainWindow = nullptr;
    KadasRedliningIntegration *mRedliningIntegration = nullptr;
    KadasGpxIntegration *mGpxIntegration = nullptr;
    bool mBlockActiveLayerChanged = false;
    QDateTime mProjectLastModified;
    KadasMapToolPan *mMapToolPan = nullptr;
    QMap<QString, QString> mItemLayerMap;
    QList<QgsMapLayerConfigWidgetFactory *> mMapLayerPanelFactories;

    QList<QgsMapLayer *> showGDALSublayerSelectionDialog( QgsRasterLayer *layer ) const;
    QList<QgsMapLayer *> showOGRSublayerSelectionDialog( QgsVectorLayer *layer ) const;
    void loadPythonSupport();
    bool showZipSublayerSelectionDialog( const QString &path ) const;
    bool projectSaveDirty();

  private slots:
    void onActiveLayerChanged( QgsMapLayer *layer );
    void onFocusChanged( QWidget * /*old*/, QWidget *now );
    void onMapToolChanged( QgsMapTool *newTool, QgsMapTool *oldTool );
    void handleItemPicked( const KadasFeaturePicker::PickResult &result );
    void showCanvasContextMenu( const QPoint &screenPos, const QgsPointXY &mapPos );
    void updateWindowTitle();
    void cleanup();
    void updateWmtsZoomResolutions() const;
};

#endif // KADASAPPLICATION_H
