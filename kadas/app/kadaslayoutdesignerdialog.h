/***************************************************************************
    kadaslayoutdesignerdialog.h
    ---------------------------
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASLAYOUTDESIGNERDIALOG_H
#define KADASLAYOUTDESIGNERDIALOG_H

#include <qgis/qgslayoutdesignerinterface.h>
#include <qgis/qgslayoutexporter.h>
#include <qgis/qgslayoutpagecollection.h>

#include "ui_kadaslayoutdesignerbase.h"

class QComboBox;
class QLabel;
class QProgressBar;
class QSlider;
class QToolButton;
class QUndoView;

class QgsDockWidget;
class QgsLayoutAppMenuProvider;
class QgsLayoutItemsListView;
class QgsLayoutPropertiesWidget;
class QgsLayoutRuler;
class QgsLayoutViewToolAddItem;
class QgsLayoutViewToolAddNodeItem;
class QgsLayoutViewToolEditNodes;
class QgsLayoutViewToolMoveItemContent;
class QgsLayoutViewToolPan;
class QgsLayoutViewToolSelect;
class QgsLayoutViewToolZoom;
class QgsPanelWidgetStack;
class QgsPrintLayout;

class KadasAppLayoutDesignerInterface;

class KadasLayoutDesignerDialog : public QMainWindow, public Ui::KadasLayoutDesignerBase
{
    Q_OBJECT

  public:

    KadasLayoutDesignerDialog( QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr );

    /**
     * Returns the designer interface for the dialog.
     */
    KadasAppLayoutDesignerInterface *iface();

    /**
     * Returns the current layout associated with the designer.
     * \see setCurrentLayout()
     */
    QgsPrintLayout *currentLayout();

    /**
     * Returns the layout view utilized by the designer.
     */
    QgsLayoutView *view();

    /**
     * Sets the current \a layout to edit in the designer.
     * \see currentLayout()
     */
    void setCurrentLayout( QgsPrintLayout *layout );

    /**
     * Shows the configuration widget for the specified layout \a item.
     *
     * If \a bringPanelToFront is true, then the item properties panel will be automatically
     * shown and raised to the top of the interface.
     */
    void showItemOptions( QgsLayoutItem *item, bool bringPanelToFront = true );

    /**
     * Selects the specified \a items.
     */
    void selectItems( const QList<QgsLayoutItem *> &items );

    /**
     * Returns the designer's message bar.
     */
    QgsMessageBar *messageBar();

    /**
     * Sets a section \a title, to use to update the dialog title to display
     * the currently edited section.
     */
    void setSectionTitle( const QString &title );

    /**
     * Returns the dialog's guide manager widget, if it exists.
     */
    QgsLayoutGuideWidget *guideWidget();

    /**
     * Toggles the visibility of the guide manager dock widget.
     */
    void showGuideDock( bool show );

  public slots:

    /**
     * Opens the dialog, and sets default view.
     */
    void open();

    /**
     * Raise, unminimize and activate this window.
     */
    void activate();

    /**
     * Toggles whether or not the rulers should be \a visible.
     */
    void showRulers( bool visible );

    /**
     * Toggles whether the page grid should be \a visible.
     */
    void showGrid( bool visible );

    /**
     * Toggles whether the item bounding boxes should be \a visible.
     */
    void showBoxes( bool visible );

    /**
     * Toggles whether the layout pages should be \a visible.
     */
    void showPages( bool visible );

    /**
     * Toggles whether snapping to the page grid is \a enabled.
     */
    void snapToGrid( bool enabled );

    /**
     * Toggles whether the page guides should be \a visible.
     */
    void showGuides( bool visible );

    /**
     * Toggles whether snapping to the page guides is \a enabled.
     */
    void snapToGuides( bool enabled );

    /**
     * Toggles whether snapping to the item guides ("smart" guides) is \a enabled.
     */
    void snapToItems( bool enabled );

    /**
     * Forces the layout, and all items contained within it, to refresh. For instance, this causes maps to redraw
     * and rebuild cached images, html items to reload their source url, and attribute tables
     * to refresh their contents. Calling this also triggers a recalculation of all data defined
     * attributes within the layout.
     */
    void refreshLayout();

    /**
     * Pastes items from the clipboard to the current layout.
     * \see pasteInPlace()
     */
    void paste();

    /**
     * Pastes item (in place) from the clipboard to the current layout.
     * \see paste()
     */
    void pasteInPlace();

  signals:

    /**
     * Emitted when the dialog is about to close.
     */
    void aboutToClose();

  protected:

    void closeEvent( QCloseEvent * ) override;

  private slots:

    void setTitle( const QString &title );

    void itemTypeAdded( int id );
    void statusZoomCombo_currentIndexChanged( int index );
    void statusZoomCombo_zoomEntered();
    void sliderZoomChanged( int value );

    void updateStatusZoom();
    void updateStatusCursorPos( QPointF position );

    void toggleFullScreen( bool enabled );

    void addPages();
    void statusMessageReceived( const QString &message );
    void undoRedoOccurredForItems( const QSet< QString > &itemUuids );
    void saveAsTemplate();
    void addItemsFromTemplate();
    void newLayout();
    void renameLayout();
    void deleteLayout();
    void print();
    void exportToRaster();
    void exportToPdf();

    void pageSetup();
    void pageOrientationChanged();

    void updateWindowTitle();

    void backgroundTaskCountChanged( int total );

  private:

    static bool sInitializedRegistry;

    KadasAppLayoutDesignerInterface *mInterface = nullptr;

    QgsPrintLayout *mLayout = nullptr;

    QgsMessageBar *mMessageBar = nullptr;

    QActionGroup *mToolsActionGroup = nullptr;

    QgsLayoutView *mView = nullptr;
    QgsLayoutRuler *mHorizontalRuler = nullptr;
    QgsLayoutRuler *mVerticalRuler = nullptr;
    QWidget *mRulerLayoutFix = nullptr;

    //! Combobox in status bar which shows/adjusts current zoom level
    QComboBox *mStatusZoomCombo = nullptr;
    QSlider *mStatusZoomSlider = nullptr;

    //! Labels in status bar which shows current mouse position
    QLabel *mStatusCursorXLabel = nullptr;
    QLabel *mStatusCursorYLabel = nullptr;
    QLabel *mStatusCursorPageLabel = nullptr;

    static QList<double> sStatusZoomLevelsList;

    QgsLayoutViewToolAddItem *mAddItemTool = nullptr;
    QgsLayoutViewToolAddNodeItem *mAddNodeItemTool = nullptr;
    QgsLayoutViewToolPan *mPanTool = nullptr;
    QgsLayoutViewToolZoom *mZoomTool = nullptr;
    QgsLayoutViewToolSelect *mSelectTool = nullptr;
    QgsLayoutViewToolEditNodes *mNodesTool = nullptr;
    QgsLayoutViewToolMoveItemContent *mMoveContentTool = nullptr;

    QMap< QString, QToolButton * > mItemGroupToolButtons;
    QMap< QString, QMenu * > mItemGroupSubmenus;

    QgsLayoutAppMenuProvider *mMenuProvider = nullptr;

    QgsDockWidget *mItemDock = nullptr;
    QgsPanelWidgetStack *mItemPropertiesStack = nullptr;
    QgsDockWidget *mGeneralDock = nullptr;
    QgsPanelWidgetStack *mGeneralPropertiesStack = nullptr;
    QgsDockWidget *mGuideDock = nullptr;
    QgsPanelWidgetStack *mGuideStack = nullptr;

    QgsLayoutPropertiesWidget *mLayoutPropertiesWidget = nullptr;

    QUndoView *mUndoView = nullptr;
    QgsDockWidget *mUndoDock = nullptr;

    QgsDockWidget *mItemsDock = nullptr;
    QgsLayoutItemsListView *mItemsTreeView = nullptr;

    QAction *mUndoAction = nullptr;
    QAction *mRedoAction = nullptr;
    //! Copy/cut/paste actions
    QAction *mActionCut = nullptr;
    QAction *mActionCopy = nullptr;
    QAction *mActionPaste = nullptr;
    QProgressBar *mStatusProgressBar = nullptr;

    struct PanelStatus
    {
      PanelStatus( bool visible = true, bool active = false )
        : isVisible( visible )
        , isActive( active )
      {}
      bool isVisible;
      bool isActive;
    };
    QMap< QString, PanelStatus > mPanelStatus;

    bool mBlockItemOptions = false;

    //! Page & Printer Setup
    std::unique_ptr< QPrinter > mPrinter;
    bool mSetPageOrientation = false;

    QString mTitle;
    QString mSectionTitle;

    QgsLayoutGuideWidget *mGuideWidget = nullptr;

    //! Save window state
    void saveWindowState();

    //! Restore the window and toolbar state
    void restoreWindowState();

    //! Switch to new item creation tool, for a new item of the specified \a id.
    void activateNewItemCreationTool( int id, bool nodeBasedItem );

    void createLayoutPropertiesWidget();

    void initializeRegistry();

    bool getRasterExportSettings( QgsLayoutExporter::ImageExportSettings &settings, QSize &imageSize );
    bool getPdfExportSettings( QgsLayoutExporter::PdfExportSettings &settings );

    //! Load predefined scales from the project's properties
    void loadPredefinedScalesFromProject();
    QVector<double> predefinedScales() const;

    void setPrinterPageOrientation( QgsLayoutItemPage::Orientation orientation );
    QPrinter *printer();

    QString defaultExportPath() const;
    void setLastExportPath( const QString &path ) const;
};


class KadasAppLayoutDesignerInterface : public QgsLayoutDesignerInterface
{
    Q_OBJECT

  public:
    KadasAppLayoutDesignerInterface( KadasLayoutDesignerDialog *dialog );
    QWidget *window() override { return mDesigner; }
    QgsLayout *layout() override;
    void setCurrentLayout( QgsLayout *layout ) override;
    QgsMasterLayoutInterface *masterLayout() override;
    QgsLayoutView *view() override { return mDesigner->view(); }
    QgsMessageBar *messageBar() override { return mDesigner->messageBar(); }
    void selectItems( const QList< QgsLayoutItem * > &items ) override { mDesigner->selectItems( items ); }
    void setAtlasPreviewEnabled( bool enabled ) override { }
    bool atlasPreviewEnabled() const override { return false; }
    void showItemOptions( QgsLayoutItem *item, bool bringPanelToFront = true ) override { mDesigner->showItemOptions( item, bringPanelToFront ); }
    QMenu *layoutMenu() override { return mDesigner->mLayoutMenu; }
    QMenu *editMenu() override { return mDesigner->menuEdit; }
    QMenu *viewMenu() override { return mDesigner->mMenuView; }
    QMenu *itemsMenu() override { return mDesigner->menuLayout; }
    QMenu *atlasMenu() override { return nullptr; }
    QMenu *reportMenu() override { return nullptr; }
    QMenu *settingsMenu() override { return mDesigner->menuSettings; }
    QToolBar *layoutToolbar() override { return mDesigner->mLayoutToolbar; }
    QToolBar *navigationToolbar() override { return mDesigner->mNavigationToolbar; }
    QToolBar *actionsToolbar() override { return mDesigner->mActionsToolbar; }
    QToolBar *atlasToolbar() override { return nullptr; }
    void addDockWidget( Qt::DockWidgetArea area, QDockWidget *dock ) override { mDesigner->addDockWidget( area, dock ); }
    void removeDockWidget( QDockWidget *dock ) override { mDesigner->removeDockWidget( dock ); }
    void activateTool( StandardTool tool ) override;
    void setSectionTitle( const QString &title ) override { mDesigner->setSectionTitle( title ); }
    QgsLayoutGuideWidget *guideWidget() override { return mDesigner->guideWidget(); }
    void showGuideDock( bool show ) override { mDesigner->showGuideDock( show ); }

  public slots:

    void close() override { mDesigner->close(); }
    void showRulers( bool visible ) override { mDesigner->showRulers( visible ); }

  private:

    KadasLayoutDesignerDialog *mDesigner = nullptr;
};

#endif // KADASLAYOUTDESIGNERDIALOG_H
