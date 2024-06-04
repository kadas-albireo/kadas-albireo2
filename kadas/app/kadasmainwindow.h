/***************************************************************************
    kadasmainwindow.h
    -----------------
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

#ifndef KADASMAINWINDOW_H
#define KADASMAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

#include <qgis/qgsmessagebaritem.h>

#include "ui_kadaswindowbase.h"
#include "ui_kadastopwidget.h"
#include "ui_kadasstatuswidget.h"

class QSplashScreen;
class QgsDecorationGrid;
class QgsLayerTreeMapCanvasBridge;
class QgsMessageBar;
class KadasCoordinateDisplayer;
class KadasGpsIntegration;
class KadasGpxIntegration;
class KadasHelpViewer;
class KadasKmlIntegration;
class KadasMilxIntegration;
class KadasMapItem;
class KadasMapWidgetManager;
class KadasProjectTemplateSelectionDialog;
class KadasPluginManager;
class KadasRedliningIntegration;


class KadasMainWindow : public QMainWindow, private Ui::KadasWindowBase, private Ui::KadasTopWidget, private Ui::KadasStatusWidget
{
    Q_OBJECT

  public:
    explicit KadasMainWindow();
    ~KadasMainWindow();
    void init();

    QgsMapCanvas *mapCanvas() const { return mMapCanvas; }
    QgsMessageBar *messageBar() const { return mInfoBar; }
    QgsLayerTreeView *layerTreeView() const { return mLayerTreeView; }
    QgsLayerTreeMapCanvasBridge *layerTreeMapCanvasBridge() const { return mLayerTreeCanvasBridge; }
    KadasMapWidgetManager *mapWidgetManager() const { return mMapWidgetManager; }
    void resetMagnification() { mMagnifierSpinBox->clear(); }
    int messageTimeout() const;

    QWidget *addRibbonTab( const QString &name );
    void addActionToTab( QAction *action, QWidget *tabWidget );
    void addMenuButtonToTab( const QString &text, const QIcon &icon, QMenu *menu, QWidget *tabWidget );
    void removeActionFromTab( QAction *action, QWidget *tabWidget );
    void removeMenuButtonFromTab( QMenu *menu, QWidget *tabWidget );
    QMenu *pluginsMenu();

    QTabWidget *ribbonTabWidget() const { return mRibbonWidget; }
    QWidget *mapsTab() const { return mRibbonWidget->widget( 0 ); }
    QWidget *viewTab() const { return mRibbonWidget->widget( 1 ); }
    QWidget *analysisTab() const { return mRibbonWidget->widget( 2 ); }
    QWidget *drawTab() const { return mRibbonWidget->widget( 3 ); }
    QWidget *gpsTab() const { return mRibbonWidget->widget( 4 ); }
    QWidget *mssTab() const { return mRibbonWidget->widget( 5 ); }
    QWidget *settingsTab() const { return mRibbonWidget->widget( 6 ); }
    QWidget *helpTab() const { return mRibbonWidget->widget( 7 ); }

    QAction *actionBullseye() const { return mActionBullseye; }
    QAction *actionGuideGrid() const { return mActionGuideGrid; }
    QAction *actionMapGrid() const { return mActionGrid; }
    QAction *actionDrawWaypoint() const { return mActionDrawWaypoint; }
    QAction *actionDrawRoute() const { return mActionDrawRoute; }
    QAction *actionDeleteItems() const { return mActionDeleteItems; }
    QAction *actionExportGPX() const { return mActionExportGPX; }
    QAction *actionImportGPX() const { return mActionImportGPX; }
    QAction *actionPin() const { return mActionPin; }
    QAction *actionMeasureLine() const { return mActionDistance; }
    QAction *actionMeasureArea() const { return mActionArea; }
    QAction *actionMeasureCircle() const { return mActionCircle; }
    QAction *actionMeasureAzimuth() const { return mActionAzimuth; }
    QAction *actionMeasureHeightProfile() const { return mActionProfile; }
    QAction *actionMeasureMinMax() const { return mActionMinMax; }
    QAction *actionTerrainSlope() const { return mActionSlope; }
    QAction *actionTerrainHillshade() const { return mActionHillshade; }
    QAction *actionTerrainViewshed() const { return mActionViewshed; }

    QAction *actionShowPythonConsole() const { return mActionShowPythonConsole; }

    KadasRedliningIntegration *redliningIntegration() const { return mRedliningIntegration; }
    KadasGpxIntegration *gpxIntegration() { return mGpxIntegration; }
    KadasCatalogBrowser *catalogBrowser() { return mCatalogBrowser; }

    void addCustomDropHandler( QgsCustomDropHandler *handler );
    void removeCustomDropHandler( QgsCustomDropHandler *handler );

    void showAuthenticatedUser( const QString &user );

  signals:
    void closed();

  public slots:
    void zoomFull();
    void zoomIn();
    void zoomNext();
    void zoomOut();
    void zoomPrev();
    void zoomToLayerExtent();

  private slots:
    void addCatalogLayer( const QgsMimeDataUtils::Uri &uri, const QString &metadataUrl, const QVariantList &sublayers );
    void addMapCanvasItem( const KadasMapItem *item );
    void removeMapCanvasItem( const KadasMapItem *item );
    void checkLayerProjection( QgsMapLayer *layer );
    void layerTreeViewDoubleClicked( const QModelIndex &index );
    void onDecimalPlacesChanged( int places );
    void onLanguageChanged( int idx );
    void onNumericInputCheckboxToggled( bool checked );
    void onSnappingChanged( bool enabled );
    void setMapScale();
    void setMapMagnifier( double val );
    void toggleScaleLock( bool active );
    void showFavoriteContextMenu( const QPoint &p );
    void showProjectSelectionWidget();
    void showScale( double scale );
    void switchToTabForTool( QgsMapTool *tool );
    void toggleLayerTree();
    void toggleFullscreen();
    void endFullscreen();
    void checkOnTheFlyProjection();
    void showPluginManager( bool show );
    void addLocalPicture();
    void addRemotePicture();
    void updateBgLayerZoomResolutions() const;
    void showHelp() const;
    void showNewsletter();
    void toggleIgnoreDpiScale();

  private:
    bool eventFilter( QObject *obj, QEvent *ev ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;
    void dragEnterEvent( QDragEnterEvent *event ) override;
    void showEvent( QShowEvent * /*event*/ ) override;
    void closeEvent( QCloseEvent * /*event*/ ) override;

    QgsMapTool *addPinTool();
    KadasRibbonButton *addRibbonButton( QWidget *tabWidget );
    void configureButtons();
    void restoreFavoriteButton( QToolButton *button );
    void setActionToButton( QAction *action, QToolButton *button, const QKeySequence &shortcut = QKeySequence(), const std::function<QgsMapTool*() > &toolFactory = nullptr );
    void showSourceSelectDialog( const QString &provider );
    void updateWidgetPositions();

    QgsMessageBar *mInfoBar = nullptr;
    QPointer<QgsMessageBarItem> mReprojMsgItem;

    QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge = nullptr;
    KadasCoordinateDisplayer *mCoordinateDisplayer = nullptr;
    KadasGpsIntegration *mGpsIntegration = nullptr;
    KadasGpxIntegration *mGpxIntegration = nullptr;
    KadasKmlIntegration *mKmlIntegration = nullptr;
    KadasMilxIntegration *mMilxIntegration = nullptr;
    KadasMapWidgetManager *mMapWidgetManager = nullptr;
    KadasRedliningIntegration *mRedliningIntegration = nullptr;
    KadasPluginManager *mPluginManager = nullptr;
    QToolButton *mPluginsToolButton = nullptr;
    QAction *mActionShowPythonConsole = nullptr;
    KadasProjectTemplateSelectionDialog *mProjectTemplateDialog = nullptr;

    QTimer mLoadingTimer;
    QPoint mResizePressPos;
    QPoint mDragStartPos;
    QMap<QString, QAction *> mAddedActions;
    QList<QgsCustomDropHandler *> mCustomDropHandlers;
    bool mFullscreen = false;

    KadasHelpViewer *mHelpViewer = nullptr;

};

#endif // KADASMAINWINDOW_H
