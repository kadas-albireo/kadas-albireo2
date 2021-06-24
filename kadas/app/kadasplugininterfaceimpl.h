/***************************************************************************
    kadasplugininterfaceimpl.h
    --------------------------
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

#ifndef KADASPLUGININTERFACEIMPL_H
#define KADASPLUGININTERFACEIMPL_H

#include <QFont>

#include <kadas/gui/kadasplugininterface.h>

class KadasApplication;

class KadasPluginInterfaceImpl : public KadasPluginInterface
{
    Q_OBJECT
  public:
    KadasPluginInterfaceImpl( KadasApplication *app );

    // Supported QGIS interface
    virtual QgsPluginManagerInterface *pluginManagerInterface() override;
    virtual QgsLayerTreeView *layerTreeView() override;
    virtual void addCustomActionForLayerType( QAction *action, QString menu, QgsMapLayerType type, bool allLayers ) override;
    virtual void addCustomActionForLayer( QAction *action, QgsMapLayer *layer ) override;
    virtual bool removeCustomActionForLayerType( QAction *action ) override;
    virtual QList< QgsMapCanvas * > mapCanvases() override;
    virtual QgsMapCanvas *createNewMapCanvas( const QString &name ) override;
    virtual void closeMapCanvas( const QString &name ) override;
    virtual QSize iconSize( bool dockedToolbar = false ) const override;
    virtual QList<QgsMapLayer *> editableLayers( bool modified = false ) const override;
    virtual QgsMapLayer *activeLayer() override;
    virtual QgsMapCanvas *mapCanvas() override;
    virtual QgsLayerTreeMapCanvasBridge *layerTreeCanvasBridge() override;
    virtual QWidget *mainWindow() override;
    virtual QgsMessageBar *messageBar() override;
    virtual QList<QgsLayoutDesignerInterface *> openLayoutDesigners() override;
    virtual QgsVectorLayerTools *vectorLayerTools() override;

    virtual QMenu *projectMenu() override { return getClassicMenu( PROJECT_MENU ); }
    virtual QMenu *editMenu() override { return getClassicMenu( EDIT_MENU ); }
    virtual QMenu *viewMenu() override { return getClassicMenu( VIEW_MENU ); }
    virtual QMenu *layerMenu() override { return getClassicMenu( LAYER_MENU ); }
    virtual QMenu *newLayerMenu() override { return getClassicMenu( NEWLAYER_MENU ); }
    virtual QMenu *addLayerMenu() override { return getClassicMenu( ADDLAYER_MENU ); }
    virtual QMenu *settingsMenu() override { return getClassicMenu( SETTINGS_MENU ); }
    virtual QMenu *pluginMenu() override { return getClassicMenu( PLUGIN_MENU ); }
    virtual QMenu *rasterMenu() override { return getClassicMenu( RASTER_MENU ); }
    virtual QMenu *databaseMenu() override { return getClassicMenu( DATABASE_MENU ); }
    virtual QMenu *vectorMenu() override { return getClassicMenu( VECTOR_MENU ); }
    virtual QMenu *webMenu() override { return getClassicMenu( WEB_MENU ); }
    virtual QMenu *firstRightStandardMenu() override { return helpMenu(); }
    virtual QMenu *windowMenu() override { return getClassicMenu( WINDOW_MENU ); }
    virtual QMenu *helpMenu() override { return getClassicMenu( HELP_MENU ); }
    virtual QMenu *pluginHelpMenu() override { return getClassicMenu( HELP_MENU ); }

    virtual void addPluginToMenu( const QString &name, QAction *action ) override;
    virtual void removePluginMenu( const QString &name, QAction *action ) override;
    virtual void insertAddLayerAction( QAction *action ) override;
    virtual void removeAddLayerAction( QAction *action ) override;
    virtual void addPluginToDatabaseMenu( const QString &name, QAction *action ) override;
    virtual void removePluginDatabaseMenu( const QString &name, QAction *action ) override;
    virtual void addPluginToRasterMenu( const QString &name, QAction *action ) override;
    virtual void removePluginRasterMenu( const QString &name, QAction *action ) override;
    virtual void addPluginToVectorMenu( const QString &name, QAction *action ) override;
    virtual void removePluginVectorMenu( const QString &name, QAction *action ) override;
    virtual void addPluginToWebMenu( const QString &name, QAction *action ) override;
    virtual void removePluginWebMenu( const QString &name, QAction *action ) override;

    virtual int messageTimeout() override;
    virtual void zoomFull() override;
    virtual void zoomToPrevious() override;
    virtual void zoomToNext() override;
    virtual void zoomToActiveLayer() override;

    virtual QgsVectorLayer *addVectorLayer( const QString &vectorLayerPath, const QString &baseName, const QString &providerKey ) override;
    virtual QgsRasterLayer *addRasterLayer( const QString &rasterLayerPath, const QString &baseName = QString() ) override;
    virtual QgsRasterLayer *addRasterLayer( const QString &url, const QString &layerName, const QString &providerKey ) override;
    virtual QgsVectorTileLayer *addVectorTileLayer( const QString &url, const QString &baseName ) override;
    virtual QgsPointCloudLayer *addPointCloudLayer( const QString &url, const QString &baseName, const QString &providerKey ) override;

    virtual QgsVectorLayer *addVectorLayerQuiet( const QString &vectorLayerPath, const QString &baseName, const QString &providerKey ) override;
    virtual QgsRasterLayer *addRasterLayerQuiet( const QString &url, const QString &layerName, const QString &providerKey ) override;
    virtual QgsVectorTileLayer *addVectorTileLayerQuiet( const QString &url, const QString &baseName ) override;
    virtual QgsPointCloudLayer *addPointCloudLayerQuiet( const QString &url, const QString &baseName, const QString &providerKey ) override;

    virtual QgsMeshLayer *addMeshLayer( const QString &url, const QString &baseName, const QString &providerKey ) override;

    virtual bool addProject( const QString &project ) override;
    virtual bool newProject( bool promptToSaveFlag = false ) override;

    virtual void reloadConnections( ) override;
    virtual bool setActiveLayer( QgsMapLayer *layer ) override;
    virtual void showLayerProperties( QgsMapLayer *l, const QString &page = QString() ) override;
    virtual QDialog *showAttributeTable( QgsVectorLayer *l, const QString &filterExpression = QString() ) override;
    virtual bool openFeatureForm( QgsVectorLayer *l, QgsFeature &f, bool updateFeatureOnly = false, bool showModal = true ) override;
    virtual QgsAttributeDialog *getFeatureForm( QgsVectorLayer *l, QgsFeature &f ) override;

    virtual void copySelectionToClipboard( QgsMapLayer * ) override;
    virtual void pasteFromClipboard( QgsMapLayer * ) override;

    virtual void openURL( const QString &url, bool useQgisDocDirectory = true ) SIP_DEPRECATED override;
    virtual void openMessageLog() override;
    virtual void showLayoutManager() override;

    bool saveProject() override;

    virtual QgsLayoutDesignerInterface *openLayoutDesigner( QgsMasterLayoutInterface *layout ) override;

    virtual void registerCustomDropHandler( QgsCustomDropHandler *handler ) override;
    virtual void unregisterCustomDropHandler( QgsCustomDropHandler *handler ) override;
    virtual void registerCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler ) override;
    virtual void unregisterCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler ) override;

    virtual bool registerMainWindowAction( QAction *action, const QString &defaultShortcut ) override;
    virtual bool unregisterMainWindowAction( QAction *action ) override;

    // Unsupported QGIS interface
    virtual QMap<QString, QVariant> defaultStyleSheetOptions() override { return QMap<QString, QVariant>(); }
    virtual QFont defaultStyleSheetFont() override { return QFont(); }
    virtual void buildStyleSheet( const QMap<QString, QVariant> &opts ) override { }
    virtual void saveStyleSheetOptions( const QMap<QString, QVariant> &opts ) override { }
    virtual QgsAdvancedDigitizingDockWidget *cadDockWidget() override { return nullptr; }

    virtual QToolBar *fileToolBar() override { return dummyToolbar(); }
    virtual QToolBar *layerToolBar() override { return dummyToolbar(); }
    virtual QToolBar *dataSourceManagerToolBar() override { return dummyToolbar(); }
    virtual QToolBar *mapNavToolToolBar() override { return dummyToolbar(); }
    virtual QToolBar *digitizeToolBar() override { return dummyToolbar(); }
    virtual QToolBar *advancedDigitizeToolBar() override { return dummyToolbar(); }
    virtual QToolBar *shapeDigitizeToolBar() override { return dummyToolbar(); }
    virtual QToolBar *attributesToolBar() override { return dummyToolbar(); }
    virtual QToolBar *pluginToolBar() override { return dummyToolbar(); }
    virtual QToolBar *helpToolBar() override { return dummyToolbar(); }
    virtual QToolBar *rasterToolBar() override { return dummyToolbar(); }
    virtual QToolBar *vectorToolBar() override { return dummyToolbar(); }
    virtual QToolBar *databaseToolBar() override { return dummyToolbar(); }
    virtual QToolBar *webToolBar() override { return dummyToolbar(); }
    virtual QToolBar *selectionToolBar() override { return dummyToolbar(); }
    QToolBar *dummyToolbar();

    virtual int addToolBarIcon( QAction *qAction ) override { return -1; }
    virtual QAction *addToolBarWidget( QWidget *widget SIP_TRANSFER ) override { return nullptr; }
    virtual void removeToolBarIcon( QAction *qAction ) override { }
    virtual QAction *addRasterToolBarWidget( QWidget *widget SIP_TRANSFER ) override { return nullptr; }
    virtual int addRasterToolBarIcon( QAction *qAction ) override { return -1; }
    virtual void removeRasterToolBarIcon( QAction *qAction ) override { }
    virtual int addVectorToolBarIcon( QAction *qAction ) override { return -1; }
    virtual QAction *addVectorToolBarWidget( QWidget *widget SIP_TRANSFER ) override {  return nullptr; }
    virtual void removeVectorToolBarIcon( QAction *qAction ) override { }
    virtual int addDatabaseToolBarIcon( QAction *qAction ) override { return -1; }
    virtual QAction *addDatabaseToolBarWidget( QWidget *widget SIP_TRANSFER ) override { return nullptr; }
    virtual void removeDatabaseToolBarIcon( QAction *qAction ) override { }
    virtual int addWebToolBarIcon( QAction *qAction ) override { return -1; }
    virtual QAction *addWebToolBarWidget( QWidget *widget SIP_TRANSFER ) override { return nullptr; }
    virtual void removeWebToolBarIcon( QAction *qAction ) override { }
    virtual QToolBar *addToolBar( const QString &name ) SIP_FACTORY override{ return nullptr; }
    virtual void addToolBar( QToolBar *toolbar SIP_TRANSFER, Qt::ToolBarArea area = Qt::TopToolBarArea ) override { }

    virtual QAction *actionNewProject() override { return nullptr; }
    virtual QAction *actionOpenProject() override { return nullptr; }
    virtual QAction *actionSaveProject() override { return nullptr; }
    virtual QAction *actionSaveProjectAs() override { return nullptr; }
    virtual QAction *actionSaveMapAsImage() override { return nullptr; }
    virtual QAction *actionProjectProperties() override { return nullptr; }
    virtual QAction *actionCreatePrintLayout() override { return nullptr; }
    virtual QAction *actionShowLayoutManager() override { return nullptr; }
    virtual QAction *actionExit() override { return nullptr; }
    virtual QAction *actionCutFeatures() override { return nullptr; }
    virtual QAction *actionCopyFeatures() override { return nullptr; }
    virtual QAction *actionPasteFeatures() override { return nullptr; }
    virtual QAction *actionAddFeature() override { return nullptr; }
    virtual QAction *actionDeleteSelected() override { return nullptr; }
    virtual QAction *actionMoveFeature() override { return nullptr; }
    virtual QAction *actionSplitFeatures() override { return nullptr; }
    virtual QAction *actionSplitParts() override { return nullptr; }
    virtual QAction *actionAddRing() override { return nullptr; }
    virtual QAction *actionAddPart() override { return nullptr; }
    virtual QAction *actionSimplifyFeature() override { return nullptr; }
    virtual QAction *actionDeleteRing() override { return nullptr; }
    virtual QAction *actionDeletePart() override { return nullptr; }
    virtual QAction *actionVertexTool() override { return nullptr; }
    virtual QAction *actionVertexToolActiveLayer() override { return nullptr; }
    virtual QAction *actionPan() override { return nullptr; }
    virtual QAction *actionPanToSelected() override { return nullptr; }
    virtual QAction *actionZoomIn() override { return nullptr; }
    virtual QAction *actionZoomOut() override { return nullptr; }
    virtual QAction *actionSelect() override { return nullptr; }
    virtual QAction *actionSelectRectangle() override { return nullptr; }
    virtual QAction *actionSelectPolygon() override { return nullptr; }
    virtual QAction *actionSelectFreehand() override { return nullptr; }
    virtual QAction *actionSelectRadius() override { return nullptr; }
    virtual QAction *actionIdentify() override { return nullptr; }
    virtual QAction *actionFeatureAction() override { return nullptr; }
    virtual QAction *actionMeasure() override { return nullptr; }
    virtual QAction *actionMeasureArea() override { return nullptr; }
    virtual QAction *actionZoomFullExtent() override { return nullptr; }
    virtual QAction *actionZoomToLayer() override { return nullptr; }
    virtual QAction *actionZoomToSelected() override { return nullptr; }
    virtual QAction *actionZoomLast() override { return nullptr; }
    virtual QAction *actionZoomNext() override { return nullptr; }
    virtual QAction *actionZoomActualSize() override { return nullptr; }
    virtual QAction *actionMapTips() override { return nullptr; }
    virtual QAction *actionNewBookmark() override { return nullptr; }
    virtual QAction *actionShowBookmarks() override { return nullptr; }
    virtual QAction *actionDraw() override { return nullptr; }
    virtual QAction *actionNewVectorLayer() override { return nullptr; }
    virtual QAction *actionAddOgrLayer() override { return nullptr; }
    virtual QAction *actionAddRasterLayer() override { return nullptr; }
    virtual QAction *actionAddPgLayer() override { return nullptr; }
    virtual QAction *actionAddWmsLayer() override { return nullptr; }
    virtual QAction *actionAddAfsLayer() override { return nullptr; }
    virtual QAction *actionAddAmsLayer() override { return nullptr; }
    virtual QAction *actionCopyLayerStyle() override { return nullptr; }
    virtual QAction *actionPasteLayerStyle() override { return nullptr; }
    virtual QAction *actionOpenTable() override { return nullptr; }
    virtual QAction *actionOpenFieldCalculator() override { return nullptr; }
    virtual QAction *actionOpenStatisticalSummary() override { return nullptr; }
    virtual QAction *actionToggleEditing() override { return nullptr; }
    virtual QAction *actionSaveActiveLayerEdits() override { return nullptr; }
    virtual QAction *actionAllEdits() override { return nullptr; }
    virtual QAction *actionSaveEdits() override { return nullptr; }
    virtual QAction *actionSaveAllEdits() override { return nullptr; }
    virtual QAction *actionRollbackEdits() override { return nullptr; }
    virtual QAction *actionRollbackAllEdits() override { return nullptr; }
    virtual QAction *actionCancelEdits() override { return nullptr; }
    virtual QAction *actionCancelAllEdits() override { return nullptr; }
    virtual QAction *actionLayerSaveAs() override { return nullptr; }
    virtual QAction *actionDuplicateLayer() override { return nullptr; }
    virtual QAction *actionLayerProperties() override { return nullptr; }
    virtual QAction *actionAddToOverview() override { return nullptr; }
    virtual QAction *actionAddAllToOverview() override { return nullptr; }
    virtual QAction *actionRemoveAllFromOverview() override { return nullptr; }
    virtual QAction *actionHideAllLayers() override { return nullptr; }
    virtual QAction *actionShowAllLayers() override { return nullptr; }
    virtual QAction *actionHideSelectedLayers() override { return nullptr; }
    virtual QAction *actionHideDeselectedLayers() override { return nullptr; }
    virtual QAction *actionShowSelectedLayers() override { return nullptr; }
    virtual QAction *actionManagePlugins() override { return nullptr; }
    virtual QAction *actionPluginListSeparator() override { return nullptr; }
    virtual QAction *actionShowPythonDialog() override;
    virtual QAction *actionToggleFullScreen() override { return nullptr; }
    virtual QAction *actionOptions() override { return nullptr; }
    virtual QAction *actionCustomProjection() override { return nullptr; }
    virtual QAction *actionHelpContents() override { return nullptr; }
    virtual QAction *actionQgisHomePage() override { return nullptr; }
    virtual QAction *actionCheckQgisVersion() override { return nullptr; }
    virtual QAction *actionAbout() override { return nullptr; }
    virtual QgsStatusBar *statusBarIface() override { return nullptr; }
    virtual QAction *actionZoomToLayers() override { return nullptr; }
    virtual QAction *actionAddXyzLayer() override { return nullptr; }
    virtual QAction *actionAddVectorTileLayer() override { return nullptr; }
    virtual QAction *actionAddPointCloudLayer() override { return nullptr; }
    virtual QAction *actionToggleSelectedLayers() override { return nullptr; }
    virtual QAction *actionToggleSelectedLayersIndependently() override { return nullptr; }
    virtual QAction *actionCircle2Points() override { return nullptr; }
    virtual QAction *actionCircle3Points() override { return nullptr; }
    virtual QAction *actionCircle3Tangents() override { return nullptr; }
    virtual QAction *actionCircle2TangentsPoint() override { return nullptr; }
    virtual QAction *actionCircleCenterPoint() override { return nullptr; }
    virtual QAction *actionEllipseCenter2Points()override { return nullptr; }
    virtual QAction *actionEllipseCenterPoint() override { return nullptr; }
    virtual QAction *actionEllipseExtent() override { return nullptr; }
    virtual QAction *actionEllipseFoci() override { return nullptr; }
    virtual QAction *actionRectangleCenterPoint() override { return nullptr; }
    virtual QAction *actionRectangleExtent() override { return nullptr; }
    virtual QAction *actionRectangle3PointsDistance() override { return nullptr; }
    virtual QAction *actionRectangle3PointsProjected() override { return nullptr; }
    virtual QAction *actionRegularPolygon2Points() override { return nullptr; }
    virtual QAction *actionRegularPolygonCenterPoint() override { return nullptr; }
    virtual QAction *actionRegularPolygonCenterCorner() override { return nullptr; }

    virtual QActionGroup *mapToolActionGroup() override { return nullptr; }

    virtual void preloadForm( const QString &uifile ) override { }
    virtual void registerLocatorFilter( QgsLocatorFilter *filter SIP_TRANSFER ) override { }
    virtual void deregisterLocatorFilter( QgsLocatorFilter *filter ) override { }
    virtual void invalidateLocatorResults() override { }
    virtual bool askForDatumTransform( QgsCoordinateReferenceSystem sourceCrs, QgsCoordinateReferenceSystem destinationCrs ) override { return false;}
    virtual QgsBrowserGuiModel *browserModel() override { return nullptr; }

    virtual void addWindow( QAction *action ) override { }
    virtual void removeWindow( QAction *action ) override { }

    virtual void addUserInputWidget( QWidget *widget ) override { }
    virtual void showOptionsDialog( QWidget *parent = nullptr, const QString &currentPage = QString() ) override;

    virtual void addDockWidget( Qt::DockWidgetArea area, QDockWidget *dockwidget ) override { }
    virtual void removeDockWidget( QDockWidget *dockwidget ) override { }

    virtual void registerMapLayerConfigWidgetFactory( QgsMapLayerConfigWidgetFactory *factory ) override { }
    virtual void unregisterMapLayerConfigWidgetFactory( QgsMapLayerConfigWidgetFactory *factory ) override { }
    virtual void registerOptionsWidgetFactory( QgsOptionsWidgetFactory *factory ) override { }
    virtual void unregisterOptionsWidgetFactory( QgsOptionsWidgetFactory *factory ) override { }

    virtual void locatorSearch( const QString &searchText ) override { }
    virtual QgsLayerTreeRegistryBridge::InsertionPoint layerTreeInsertionPoint() override;

    virtual void showProjectPropertiesDialog( const QString &currentPage = QString() ) override {};
    virtual void addTabifiedDockWidget( Qt::DockWidgetArea area, QDockWidget *dockwidget, const QStringList &tabifyWith = QStringList(), bool raiseTab = false ) override {};
    virtual void registerProjectPropertiesWidgetFactory( QgsOptionsWidgetFactory *factory ) override {};
    virtual void unregisterProjectPropertiesWidgetFactory( QgsOptionsWidgetFactory *factory ) override {};
    virtual void registerDevToolWidgetFactory( QgsDevToolWidgetFactory *factory ) override {};
    virtual void unregisterDevToolWidgetFactory( QgsDevToolWidgetFactory *factory ) override {};
    virtual void registerApplicationExitBlocker( QgsApplicationExitBlockerInterface *blocker ) override {};
    virtual void unregisterApplicationExitBlocker( QgsApplicationExitBlockerInterface *blocker ) override {};
    virtual void registerMapToolHandler( QgsAbstractMapToolHandler *handler ) override {};
    virtual void unregisterMapToolHandler( QgsAbstractMapToolHandler *handler ) override {};
    virtual void registerCustomProjectOpenHandler( QgsCustomProjectOpenHandler *handler ) override {};
    virtual void unregisterCustomProjectOpenHandler( QgsCustomProjectOpenHandler *handler ) override {};
    virtual void setGpsPanelConnection( QgsGpsConnection *connection ) override {};


    // KADAS specific interface
    QMenu *getClassicMenu( ActionClassicMenuLocation classicMenuLocation, const QString &customName = QString() ) override;
    QMenu *getSubMenu( QMenu *menu, const QString &submenuName ) override;
    QWidget *getRibbonWidget() override;
    QWidget *getRibbonTabWidget( ActionRibbonTabLocation ribbonTabLocation, const QString &customName ) override;

    //! Generic action adder
    void addAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString(), QgsMapTool *associatedMapTool = nullptr ) override;
    void addActionMenu( const QString &text, const QIcon &icon, QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString() ) override;

    //! Generic action remover
    void removeAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString(), QgsMapTool *associatedMapTool = nullptr ) override;
    void removeActionMenu( QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName = QString() ) override;

    //! Generic action finder
    QAction *findAction( const QString &name ) override;

    //! Generic object finder
    QObject *findObject( const QString &name ) override;

    QgsPrintLayout *createNewPrintLayout( const QString &title ) override;
    bool deletePrintLayout( QgsPrintLayout *layout ) override;
    QList<QgsPrintLayout *> printLayouts() const override;
    void showLayoutDesigner( QgsPrintLayout *layout ) override;
};


#endif // KADASPLUGININTERFACEIMPL_H
