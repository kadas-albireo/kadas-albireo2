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
class KadasMapItem;
class KadasMapWidgetManager;


class KadasMainWindow : public QMainWindow, private Ui::KadasWindowBase, private Ui::KadasTopWidget, private Ui::KadasStatusWidget
{
    Q_OBJECT

  public:
    explicit KadasMainWindow( QSplashScreen *splash );
    void init();

    QgsMapCanvas *mapCanvas() const { return mMapCanvas; }
    QgsMessageBar *messageBar() const { return mInfoBar; }
    QgsLayerTreeView *layerTreeView() const { return mLayerTreeView; }
    QgsLayerTreeMapCanvasBridge *layerTreeMapCanvasBridge() const { return mLayerTreeCanvasBridge; }
    KadasMapWidgetManager *mapWidgetManager() const { return mMapWidgetManager; }
    int messageTimeout() const;

    QWidget *addRibbonTab( const QString &name );
    void addActionToTab( QAction *action, QWidget *tabWidget );
    void addMenuButtonToTab( const QString &text, const QIcon &icon, QMenu *menu, QWidget *tabWidget );
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

  public slots:
    void zoomFull();
    void zoomIn();
    void zoomNext();
    void zoomOut();
    void zoomPrev();
    void zoomToLayerExtent();

  private slots:
    void addMapCanvasItem( const KadasMapItem *item );
    void removeMapCanvasItem( const KadasMapItem *item );
    void checkLayerProjection( QgsMapLayer *layer );
    void onDecimalPlacesChanged( int places );
    void onLanguageChanged( int idx );
    void onNumericInputCheckboxToggled( bool checked );
    void onSnappingChanged( bool enabled );
    void setMapScale();
    void showFavoriteContextMenu( const QPoint &p );
    void showProjectSelectionWidget();
    void showScale( double scale );
    void switchToTabForTool( QgsMapTool *tool );
    void toggleLayerTree();
    void checkOnTheFlyProjection();

  private:
    bool eventFilter( QObject *obj, QEvent *ev ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void dropEvent( QDropEvent *event ) override;
    void dragEnterEvent( QDragEnterEvent *event ) override;
    void showEvent( QShowEvent * /*event*/ ) override;
    void restoreFavoriteButton( QToolButton *button );
    void configureButtons();
    void setActionToButton( QAction *action, QToolButton *button, const QKeySequence &shortcut = QKeySequence(), const std::function<QgsMapTool*() > &toolFactory = nullptr );
    void updateWidgetPositions();
    KadasRibbonButton *addRibbonButton( QWidget *tabWidget );
    void showSourceSelectDialog( const QString &provider );
    QgsMapTool *addPinTool();
    QgsMapTool *addPictureTool();
    QgsMapTool *paste();

    QgsMessageBar *mInfoBar = nullptr;
    QPointer<QgsMessageBarItem> mReprojMsgItem;

    QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge = nullptr;
    KadasCoordinateDisplayer *mCoordinateDisplayer = nullptr;
    KadasGpsIntegration *mGpsIntegration = nullptr;
    KadasMapWidgetManager *mMapWidgetManager = nullptr;
    QgsDecorationGrid *mDecorationGrid = nullptr;

    QTimer mLoadingTimer;
    QPoint mResizePressPos;
    QPoint mDragStartPos;
    QMap<QString, QAction *> mAddedActions;

    friend class KadasGpsIntegration;
};

#endif // KADASMAINWINDOW_H
