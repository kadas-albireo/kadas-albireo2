/***************************************************************************
    kadaslayoutdesignerdialog.cpp
    -----------------------------
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

#include <QComboBox>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageSetupDialog>
#include <QPrintDialog>
#include <QProgressBar>
#include <QShortcut>
#include <QToolButton>
#include <QUndoView>
#include <QWidgetAction>

#include <qgis/qgsdockwidget.h>
#include <qgis/qgsgui.h>
#include <qgis/qgsfileutils.h>
#include <qgis/qgslayoutaddpagesdialog.h>
#include <qgis/qgslayoutguidewidget.h>
#include <qgis/qgslayoutimageexportoptionsdialog.h>
#include <qgis/qgslayoutitemguiregistry.h>
#include <qgis/qgslayoutitemslistview.h>
#include <qgis/qgslayoutitemmap.h>
#include <qgis/qgslayoutitemwidget.h>
#include <qgis/qgslayoutmanager.h>
#include <qgis/qgslayoutmousehandles.h>
#include <qgis/qgslayoutpagepropertieswidget.h>
#include <qgis/qgslayoutpdfexportoptionsdialog.h>
#include <qgis/qgslayoutpropertieswidget.h>
#include <qgis/qgslayoutruler.h>
#include <qgis/qgslayoutview.h>
#include <qgis/qgslayoutviewtooladditem.h>
#include <qgis/qgslayoutviewtooladdnodeitem.h>
#include <qgis/qgslayoutviewtooleditnodes.h>
#include <qgis/qgslayoutviewtoolmoveitemcontent.h>
#include <qgis/qgslayoutviewtoolpan.h>
#include <qgis/qgslayoutviewtoolselect.h>
#include <qgis/qgslayoutviewtoolzoom.h>
#include <qgis/qgslayoutundostack.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgspanelwidget.h>
#include <qgis/qgspanelwidgetstack.h>
#include <qgis/qgsprintlayout.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsprojectviewsettings.h>
#include <qgis/qgssettings.h>

#include "kadasapplication.h"
#include "kadaslayoutappmenuprovider.h"
#include "kadaslayoutdesignerdialog.h"
#include "kadasmainwindow.h"


//add some nice zoom levels for zoom comboboxes
QList<double> KadasLayoutDesignerDialog::sStatusZoomLevelsList { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
#define FIT_LAYOUT -101
#define FIT_LAYOUT_WIDTH -102

bool KadasLayoutDesignerDialog::sInitializedRegistry = false;


static bool cmpByText_( QAction *a, QAction *b )
{
  return QString::localeAwareCompare( a->text(), b->text() ) < 0;
}


KadasLayoutDesignerDialog::KadasLayoutDesignerDialog( QWidget *parent, Qt::WindowFlags flags )
  : QMainWindow( parent, flags )
  , mInterface( new KadasAppLayoutDesignerInterface( this ) )
  , mToolsActionGroup( new QActionGroup( this ) )
{
  if ( !sInitializedRegistry )
  {
    initializeRegistry();
  }
  QgsSettings settings;

  setupUi( this );
  mTitle = tr( "Kadas Layout Designer" );

  updateWindowTitle();

  setAttribute( Qt::WA_DeleteOnClose );
  setDockOptions( dockOptions() | QMainWindow::GroupedDragging );

  //create layout view
  QGridLayout *viewLayout = new QGridLayout();
  viewLayout->setSpacing( 0 );
  viewLayout->setMargin( 0 );
  viewLayout->setContentsMargins( 0, 0, 0, 0 );
  centralWidget()->layout()->setSpacing( 0 );
  centralWidget()->layout()->setMargin( 0 );
  centralWidget()->layout()->setContentsMargins( 0, 0, 0, 0 );

  mMessageBar = new QgsMessageBar( centralWidget() );
  mMessageBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
  static_cast< QGridLayout * >( centralWidget()->layout() )->addWidget( mMessageBar, 0, 0, 1, 1, Qt::AlignTop );

  mHorizontalRuler = new QgsLayoutRuler( nullptr, Qt::Horizontal );
  mVerticalRuler = new QgsLayoutRuler( nullptr, Qt::Vertical );
  mRulerLayoutFix = new QWidget();
  mRulerLayoutFix->setAttribute( Qt::WA_NoMousePropagation );
  mRulerLayoutFix->setBackgroundRole( QPalette::Window );
  mRulerLayoutFix->setFixedSize( mVerticalRuler->rulerSize(), mHorizontalRuler->rulerSize() );
  viewLayout->addWidget( mRulerLayoutFix, 0, 0 );
  viewLayout->addWidget( mHorizontalRuler, 0, 1 );
  viewLayout->addWidget( mVerticalRuler, 1, 0 );

  //initial state of rulers
  bool showRulers = settings.value( QStringLiteral( "LayoutDesigner/showRulers" ), true, QgsSettings::App ).toBool();
  mActionShowRulers->setChecked( showRulers );
  mHorizontalRuler->setVisible( showRulers );
  mVerticalRuler->setVisible( showRulers );
  mRulerLayoutFix->setVisible( showRulers );
  mActionShowRulers->blockSignals( false );
  connect( mActionShowRulers, &QAction::triggered, this, &KadasLayoutDesignerDialog::showRulers );

  QMenu *rulerMenu = new QMenu( this );
  rulerMenu->addAction( mActionShowGuides );
  rulerMenu->addAction( mActionSnapGuides );
  rulerMenu->addAction( mActionManageGuides );
  rulerMenu->addAction( mActionClearGuides );
  rulerMenu->addSeparator();
  rulerMenu->addAction( mActionShowRulers );
  mHorizontalRuler->setContextMenu( rulerMenu );
  mVerticalRuler->setContextMenu( rulerMenu );

  connect( mActionRefreshView, &QAction::triggered, this, &KadasLayoutDesignerDialog::refreshLayout );
  connect( mActionNewLayout, &QAction::triggered, this, &KadasLayoutDesignerDialog::newLayout );
  connect( mActionRemoveLayout, &QAction::triggered, this, &KadasLayoutDesignerDialog::deleteLayout );

  connect( mActionPrint, &QAction::triggered, this, &KadasLayoutDesignerDialog::print );
  connect( mActionExportAsImage, &QAction::triggered, this, &KadasLayoutDesignerDialog::exportToRaster );
  connect( mActionExportAsPDF, &QAction::triggered, this, &KadasLayoutDesignerDialog::exportToPdf );

  connect( mActionShowGrid, &QAction::triggered, this, &KadasLayoutDesignerDialog::showGrid );
  connect( mActionSnapGrid, &QAction::triggered, this, &KadasLayoutDesignerDialog::snapToGrid );

  connect( mActionShowGuides, &QAction::triggered, this, &KadasLayoutDesignerDialog::showGuides );
  connect( mActionSnapGuides, &QAction::triggered, this, &KadasLayoutDesignerDialog::snapToGuides );
  connect( mActionSmartGuides, &QAction::triggered, this, &KadasLayoutDesignerDialog::snapToItems );

  connect( mActionShowBoxes, &QAction::triggered, this, &KadasLayoutDesignerDialog::showBoxes );
  connect( mActionShowPage, &QAction::triggered, this, &KadasLayoutDesignerDialog::showPages );

  connect( mActionPasteInPlace, &QAction::triggered, this, &KadasLayoutDesignerDialog::pasteInPlace );

  connect( mActionPageSetup, &QAction::triggered, this, &KadasLayoutDesignerDialog::pageSetup );

  mView = new QgsLayoutView();
  //mView->setMapCanvas( mQgis->mapCanvas() );
  mView->setContentsMargins( 0, 0, 0, 0 );
  mView->setHorizontalRuler( mHorizontalRuler );
  mView->setVerticalRuler( mVerticalRuler );
  viewLayout->addWidget( mView, 1, 1 );
  //view does not accept focus via tab
  mView->setFocusPolicy( Qt::ClickFocus );
  mViewFrame->setLayout( viewLayout );
  mViewFrame->setContentsMargins( 0, 0, 0, 1 ); // 1 is deliberate!
  mView->setFrameShape( QFrame::NoFrame );

  connect( mActionClose, &QAction::triggered, this, &QWidget::close );

  // populate with initial items...
  const QList< int > itemMetadataIds = QgsGui::layoutItemGuiRegistry()->itemMetadataIds();
  for ( int id : itemMetadataIds )
  {
    itemTypeAdded( id );
  }
  //..and listen out for new item types
  connect( QgsGui::layoutItemGuiRegistry(), &QgsLayoutItemGuiRegistry::typeAdded, this, &KadasLayoutDesignerDialog::itemTypeAdded );

  QToolButton *orderingToolButton = new QToolButton( this );
  orderingToolButton->setPopupMode( QToolButton::InstantPopup );
  orderingToolButton->setAutoRaise( true );
  orderingToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  orderingToolButton->addAction( mActionRaiseItems );
  orderingToolButton->addAction( mActionLowerItems );
  orderingToolButton->addAction( mActionMoveItemsToTop );
  orderingToolButton->addAction( mActionMoveItemsToBottom );
  orderingToolButton->setDefaultAction( mActionRaiseItems );
  mActionsToolbar->addWidget( orderingToolButton );

  QToolButton *alignToolButton = new QToolButton( this );
  alignToolButton->setPopupMode( QToolButton::InstantPopup );
  alignToolButton->setAutoRaise( true );
  alignToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  alignToolButton->addAction( mActionAlignLeft );
  alignToolButton->addAction( mActionAlignHCenter );
  alignToolButton->addAction( mActionAlignRight );
  alignToolButton->addAction( mActionAlignTop );
  alignToolButton->addAction( mActionAlignVCenter );
  alignToolButton->addAction( mActionAlignBottom );
  alignToolButton->setDefaultAction( mActionAlignLeft );
  mActionsToolbar->addWidget( alignToolButton );

  QToolButton *distributeToolButton = new QToolButton( this );
  distributeToolButton->setPopupMode( QToolButton::InstantPopup );
  distributeToolButton->setAutoRaise( true );
  distributeToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  distributeToolButton->addAction( mActionDistributeLeft );
  distributeToolButton->addAction( mActionDistributeHCenter );
  distributeToolButton->addAction( mActionDistributeHSpace );
  distributeToolButton->addAction( mActionDistributeRight );
  distributeToolButton->addAction( mActionDistributeTop );
  distributeToolButton->addAction( mActionDistributeVCenter );
  distributeToolButton->addAction( mActionDistributeVSpace );
  distributeToolButton->addAction( mActionDistributeBottom );
  distributeToolButton->setDefaultAction( mActionDistributeLeft );
  mActionsToolbar->addWidget( distributeToolButton );

  QToolButton *resizeToolButton = new QToolButton( this );
  resizeToolButton->setPopupMode( QToolButton::InstantPopup );
  resizeToolButton->setAutoRaise( true );
  resizeToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
  resizeToolButton->addAction( mActionResizeNarrowest );
  resizeToolButton->addAction( mActionResizeWidest );
  resizeToolButton->addAction( mActionResizeShortest );
  resizeToolButton->addAction( mActionResizeTallest );
  resizeToolButton->addAction( mActionResizeToSquare );
  resizeToolButton->setDefaultAction( mActionResizeNarrowest );
  mActionsToolbar->addWidget( resizeToolButton );

  mAddItemTool = new QgsLayoutViewToolAddItem( mView );
  mAddNodeItemTool = new QgsLayoutViewToolAddNodeItem( mView );
  mPanTool = new QgsLayoutViewToolPan( mView );
  mPanTool->setAction( mActionPan );
  mToolsActionGroup->addAction( mActionPan );
  connect( mActionPan, &QAction::triggered, mPanTool, [ = ] { mView->setTool( mPanTool ); } );
  mZoomTool = new QgsLayoutViewToolZoom( mView );
  mZoomTool->setAction( mActionZoomTool );
  mToolsActionGroup->addAction( mActionZoomTool );
  connect( mActionZoomTool, &QAction::triggered, mZoomTool, [ = ] { mView->setTool( mZoomTool ); } );
  mSelectTool = new QgsLayoutViewToolSelect( mView );
  mSelectTool->setAction( mActionSelectMoveItem );
  mToolsActionGroup->addAction( mActionSelectMoveItem );
  connect( mActionSelectMoveItem, &QAction::triggered, mSelectTool, [ = ] { mView->setTool( mSelectTool ); } );
  // after creating an item with the add item tool, switch immediately to select tool
  connect( mAddItemTool, &QgsLayoutViewToolAddItem::createdItem, this, [ = ] { mView->setTool( mSelectTool ); } );
  connect( mAddNodeItemTool, &QgsLayoutViewToolAddNodeItem::createdItem, this, [ = ] { mView->setTool( mSelectTool ); } );

  mNodesTool = new QgsLayoutViewToolEditNodes( mView );
  mNodesTool->setAction( mActionEditNodesItem );
  mToolsActionGroup->addAction( mActionEditNodesItem );
  connect( mActionEditNodesItem, &QAction::triggered, mNodesTool, [ = ] { mView->setTool( mNodesTool ); } );

  mMoveContentTool = new QgsLayoutViewToolMoveItemContent( mView );
  mMoveContentTool->setAction( mActionMoveItemContent );
  mToolsActionGroup->addAction( mActionMoveItemContent );
  connect( mActionMoveItemContent, &QAction::triggered, mMoveContentTool, [ = ] { mView->setTool( mMoveContentTool ); } );

  //Ctrl+= should also trigger zoom in
  QShortcut *ctrlEquals = new QShortcut( QKeySequence( QStringLiteral( "Ctrl+=" ) ), this );
  connect( ctrlEquals, &QShortcut::activated, mActionZoomIn, &QAction::trigger );
  //Backspace should also trigger delete selection
  QShortcut *backSpace = new QShortcut( QKeySequence( QStringLiteral( "Backspace" ) ), this );
  connect( backSpace, &QShortcut::activated, mActionDeleteSelection, &QAction::trigger );

  mActionPreviewModeOff->setChecked( true );
  connect( mActionPreviewModeOff, &QAction::triggered, this, [ = ]
  {
    mView->setPreviewModeEnabled( false );
  } );
  connect( mActionPreviewModeGrayscale, &QAction::triggered, this, [ = ]
  {
    mView->setPreviewMode( QgsPreviewEffect::PreviewGrayscale );
    mView->setPreviewModeEnabled( true );
  } );
  connect( mActionPreviewModeMono, &QAction::triggered, this, [ = ]
  {
    mView->setPreviewMode( QgsPreviewEffect::PreviewMono );
    mView->setPreviewModeEnabled( true );
  } );
  connect( mActionPreviewProtanope, &QAction::triggered, this, [ = ]
  {
    mView->setPreviewMode( QgsPreviewEffect::PreviewProtanope );
    mView->setPreviewModeEnabled( true );
  } );
  connect( mActionPreviewDeuteranope, &QAction::triggered, this, [ = ]
  {
    mView->setPreviewMode( QgsPreviewEffect::PreviewDeuteranope );
    mView->setPreviewModeEnabled( true );
  } );
  QActionGroup *previewGroup = new QActionGroup( this );
  previewGroup->setExclusive( true );
  mActionPreviewModeOff->setActionGroup( previewGroup );
  mActionPreviewModeGrayscale->setActionGroup( previewGroup );
  mActionPreviewModeMono->setActionGroup( previewGroup );
  mActionPreviewProtanope->setActionGroup( previewGroup );
  mActionPreviewDeuteranope->setActionGroup( previewGroup );

  connect( mActionSaveAsTemplate, &QAction::triggered, this, &KadasLayoutDesignerDialog::saveAsTemplate );
  connect( mActionLoadFromTemplate, &QAction::triggered, this, &KadasLayoutDesignerDialog::addItemsFromTemplate );
  connect( mActionRenameLayout, &QAction::triggered, this, &KadasLayoutDesignerDialog::renameLayout );

  connect( mActionZoomIn, &QAction::triggered, mView, &QgsLayoutView::zoomIn );
  connect( mActionZoomOut, &QAction::triggered, mView, &QgsLayoutView::zoomOut );
  connect( mActionZoomAll, &QAction::triggered, mView, &QgsLayoutView::zoomFull );
  connect( mActionZoomActual, &QAction::triggered, mView, &QgsLayoutView::zoomActual );
  connect( mActionZoomToWidth, &QAction::triggered, mView, &QgsLayoutView::zoomWidth );

  connect( mActionSelectAll, &QAction::triggered, mView, &QgsLayoutView::selectAll );
  connect( mActionDeselectAll, &QAction::triggered, mView, &QgsLayoutView::deselectAll );
  connect( mActionInvertSelection, &QAction::triggered, mView, &QgsLayoutView::invertSelection );
  connect( mActionSelectNextAbove, &QAction::triggered, mView, &QgsLayoutView::selectNextItemAbove );
  connect( mActionSelectNextBelow, &QAction::triggered, mView, &QgsLayoutView::selectNextItemBelow );

  connect( mActionRaiseItems, &QAction::triggered, mView, &QgsLayoutView::raiseSelectedItems );
  connect( mActionLowerItems, &QAction::triggered, mView, &QgsLayoutView::lowerSelectedItems );
  connect( mActionMoveItemsToTop, &QAction::triggered, mView, &QgsLayoutView::moveSelectedItemsToTop );
  connect( mActionMoveItemsToBottom, &QAction::triggered, mView, &QgsLayoutView::moveSelectedItemsToBottom );
  connect( mActionAlignLeft, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignLeft );
  } );
  connect( mActionAlignHCenter, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignHCenter );
  } );
  connect( mActionAlignRight, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignRight );
  } );
  connect( mActionAlignTop, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignTop );
  } );
  connect( mActionAlignVCenter, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignVCenter );
  } );
  connect( mActionAlignBottom, &QAction::triggered, this, [ = ]
  {
    mView->alignSelectedItems( QgsLayoutAligner::AlignBottom );
  } );
  connect( mActionDistributeLeft, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeLeft );
  } );
  connect( mActionDistributeHCenter, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeHCenter );
  } );
  connect( mActionDistributeHSpace, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeHSpace );
  } );
  connect( mActionDistributeRight, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeRight );
  } );
  connect( mActionDistributeTop, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeTop );
  } );
  connect( mActionDistributeVCenter, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeVCenter );
  } );
  connect( mActionDistributeVSpace, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeVSpace );
  } );
  connect( mActionDistributeBottom, &QAction::triggered, this, [ = ]
  {
    mView->distributeSelectedItems( QgsLayoutAligner::DistributeBottom );
  } );
  connect( mActionResizeNarrowest, &QAction::triggered, this, [ = ]
  {
    mView->resizeSelectedItems( QgsLayoutAligner::ResizeNarrowest );
  } );
  connect( mActionResizeWidest, &QAction::triggered, this, [ = ]
  {
    mView->resizeSelectedItems( QgsLayoutAligner::ResizeWidest );
  } );
  connect( mActionResizeShortest, &QAction::triggered, this, [ = ]
  {
    mView->resizeSelectedItems( QgsLayoutAligner::ResizeShortest );
  } );
  connect( mActionResizeTallest, &QAction::triggered, this, [ = ]
  {
    mView->resizeSelectedItems( QgsLayoutAligner::ResizeTallest );
  } );
  connect( mActionResizeToSquare, &QAction::triggered, this, [ = ]
  {
    mView->resizeSelectedItems( QgsLayoutAligner::ResizeToSquare );
  } );

  connect( mActionAddPages, &QAction::triggered, this, &KadasLayoutDesignerDialog::addPages );

  connect( mActionUnlockAll, &QAction::triggered, mView, &QgsLayoutView::unlockAllItems );
  connect( mActionLockItems, &QAction::triggered, mView, &QgsLayoutView::lockSelectedItems );

  connect( mActionDeleteSelection, &QAction::triggered, this, [ = ]
  {
    if ( mView->tool() == mNodesTool )
      mNodesTool->deleteSelectedNode();
    else
      mView->deleteSelectedItems();
  } );
  connect( mActionGroupItems, &QAction::triggered, this, [ = ]
  {
    mView->groupSelectedItems();
  } );
  connect( mActionUngroupItems, &QAction::triggered, this, [ = ]
  {
    mView->ungroupSelectedItems();
  } );

  //cut/copy/paste actions. Note these are not included in the ui file
  //as ui files have no support for QKeySequence shortcuts
  mActionCut = new QAction( tr( "Cu&t" ), this );
  mActionCut->setShortcuts( QKeySequence::Cut );
  mActionCut->setStatusTip( tr( "Cut" ) );
  mActionCut->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditCut.svg" ) ) );
  connect( mActionCut, &QAction::triggered, this, [ = ]
  {
    mView->copySelectedItems( QgsLayoutView::ClipboardCut );
  } );

  mActionCopy = new QAction( tr( "&Copy" ), this );
  mActionCopy->setShortcuts( QKeySequence::Copy );
  mActionCopy->setStatusTip( tr( "Copy" ) );
  mActionCopy->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditCopy.svg" ) ) );
  connect( mActionCopy, &QAction::triggered, this, [ = ]
  {
    mView->copySelectedItems( QgsLayoutView::ClipboardCopy );
  } );

  mActionPaste = new QAction( tr( "&Paste" ), this );
  mActionPaste->setShortcuts( QKeySequence::Paste );
  mActionPaste->setStatusTip( tr( "Paste" ) );
  mActionPaste->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionEditPaste.svg" ) ) );
  connect( mActionPaste, &QAction::triggered, this, &KadasLayoutDesignerDialog::paste );

  menuEdit->insertAction( mActionPasteInPlace, mActionCut );
  menuEdit->insertAction( mActionPasteInPlace, mActionCopy );
  menuEdit->insertAction( mActionPasteInPlace, mActionPaste );

  // Add a progress bar to the status bar for indicating rendering in progress
  mStatusProgressBar = new QProgressBar( mStatusBar );
  mStatusProgressBar->setObjectName( QStringLiteral( "mProgressBar" ) );
  mStatusProgressBar->setMaximumWidth( 100 );
  mStatusProgressBar->setMaximumHeight( 18 );
  mStatusProgressBar->hide();
  mStatusBar->addPermanentWidget( mStatusProgressBar, 1 );

  //create status bar labels
  mStatusCursorXLabel = new QLabel( mStatusBar );
  mStatusCursorXLabel->setMinimumWidth( 100 );
  mStatusCursorYLabel = new QLabel( mStatusBar );
  mStatusCursorYLabel->setMinimumWidth( 100 );
  mStatusCursorPageLabel = new QLabel( mStatusBar );
  mStatusCursorPageLabel->setMinimumWidth( 100 );

  mStatusBar->addPermanentWidget( mStatusCursorXLabel );
  mStatusBar->addPermanentWidget( mStatusCursorXLabel );
  mStatusBar->addPermanentWidget( mStatusCursorYLabel );
  mStatusBar->addPermanentWidget( mStatusCursorPageLabel );

  mStatusZoomCombo = new QComboBox();
  mStatusZoomCombo->setEditable( true );
  mStatusZoomCombo->setInsertPolicy( QComboBox::NoInsert );
  mStatusZoomCombo->setCompleter( nullptr );
  mStatusZoomCombo->setMinimumWidth( 100 );
  //zoom combo box accepts decimals in the range 1-9999, with an optional decimal point and "%" sign
  QRegularExpression zoomRx( QStringLiteral( "\\s*\\d{1,4}(\\.\\d?)?\\s*%?" ) );
  QValidator *zoomValidator = new QRegularExpressionValidator( zoomRx, mStatusZoomCombo );
  mStatusZoomCombo->lineEdit()->setValidator( zoomValidator );

  const auto constSStatusZoomLevelsList = sStatusZoomLevelsList;
  for ( double level : constSStatusZoomLevelsList )
  {
    mStatusZoomCombo->insertItem( 0, tr( "%1%" ).arg( level * 100.0, 0, 'f', 1 ), level );
  }
  mStatusZoomCombo->insertItem( 0, tr( "Fit Layout" ), FIT_LAYOUT );
  mStatusZoomCombo->insertItem( 0, tr( "Fit Layout Width" ), FIT_LAYOUT_WIDTH );
  connect( mStatusZoomCombo, static_cast<void ( QComboBox::* )( int )>( &QComboBox::activated ), this, &KadasLayoutDesignerDialog::statusZoomCombo_currentIndexChanged );
  connect( mStatusZoomCombo->lineEdit(), &QLineEdit::returnPressed, this, &KadasLayoutDesignerDialog::statusZoomCombo_zoomEntered );

  mStatusZoomSlider = new QSlider();
  mStatusZoomSlider->setFixedWidth( mStatusZoomCombo->width() );
  mStatusZoomSlider->setOrientation( Qt::Horizontal );
  mStatusZoomSlider->setMinimum( 20 );
  mStatusZoomSlider->setMaximum( 800 );
  connect( mStatusZoomSlider, &QSlider::valueChanged, this, &KadasLayoutDesignerDialog::sliderZoomChanged );

  mStatusZoomCombo->setToolTip( tr( "Zoom level" ) );
  mStatusZoomSlider->setToolTip( tr( "Zoom level" ) );

  mStatusBar->addPermanentWidget( mStatusZoomCombo );
  mStatusBar->addPermanentWidget( mStatusZoomSlider );

  //hide borders from child items in status bar under Windows
  mStatusBar->setStyleSheet( QStringLiteral( "QStatusBar::item {border: none;}" ) );

  mView->setTool( mSelectTool );
  mView->setFocus();
  connect( mView, &QgsLayoutView::zoomLevelChanged, this, &KadasLayoutDesignerDialog::updateStatusZoom );
  connect( mView, &QgsLayoutView::cursorPosChanged, this, &KadasLayoutDesignerDialog::updateStatusCursorPos );
  //also listen out for position updates from the horizontal/vertical rulers
  connect( mHorizontalRuler, &QgsLayoutRuler::cursorPosChanged, this, &KadasLayoutDesignerDialog::updateStatusCursorPos );
  connect( mVerticalRuler, &QgsLayoutRuler::cursorPosChanged, this, &KadasLayoutDesignerDialog::updateStatusCursorPos );

  connect( mView, &QgsLayoutView::itemFocused, this, [ = ]( QgsLayoutItem * item )
  {
    showItemOptions( item, false );
  } );

  // Panel and toolbar submenus
  mToolbarMenu->addAction( mLayoutToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mNavigationToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mToolsToolbar->toggleViewAction() );
  mToolbarMenu->addAction( mActionsToolbar->toggleViewAction() );

  connect( mActionToggleFullScreen, &QAction::toggled, this, &KadasLayoutDesignerDialog::toggleFullScreen );

  mMenuProvider = new KadasLayoutAppMenuProvider( this );
  mView->setMenuProvider( mMenuProvider );

  int minDockWidth( fontMetrics().horizontalAdvance( QStringLiteral( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) ) );

  setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
  mGeneralDock = new QgsDockWidget( tr( "Layout" ), this );
  mGeneralDock->setObjectName( QStringLiteral( "LayoutDock" ) );
  mGeneralDock->setMinimumWidth( minDockWidth );
  mGeneralPropertiesStack = new QgsPanelWidgetStack();
  mGeneralDock->setWidget( mGeneralPropertiesStack );
  mPanelsMenu->addAction( mGeneralDock->toggleViewAction() );
  connect( mActionLayoutProperties, &QAction::triggered, this, [ = ]
  {
    mGeneralDock->setUserVisible( true );
  } );

  mItemDock = new QgsDockWidget( tr( "Item Properties" ), this );
  mItemDock->setObjectName( QStringLiteral( "ItemDock" ) );
  mItemDock->setMinimumWidth( minDockWidth );
  mItemPropertiesStack = new QgsPanelWidgetStack();
  mItemDock->setWidget( mItemPropertiesStack );
  mPanelsMenu->addAction( mItemDock->toggleViewAction() );

  mGuideDock = new QgsDockWidget( tr( "Guides" ), this );
  mGuideDock->setObjectName( QStringLiteral( "GuideDock" ) );
  mGuideDock->setMinimumWidth( minDockWidth );
  mGuideStack = new QgsPanelWidgetStack();
  mGuideDock->setWidget( mGuideStack );
  mPanelsMenu->addAction( mGuideDock->toggleViewAction() );
  connect( mActionManageGuides, &QAction::triggered, this, [ = ]
  {
    mGuideDock->setUserVisible( true );
  } );

  mUndoDock = new QgsDockWidget( tr( "Undo History" ), this );
  mUndoDock->setObjectName( QStringLiteral( "UndoDock" ) );
  mPanelsMenu->addAction( mUndoDock->toggleViewAction() );
  mUndoView = new QUndoView( this );
  mUndoDock->setWidget( mUndoView );

  mItemsDock = new QgsDockWidget( tr( "Items" ), this );
  mItemsDock->setObjectName( QStringLiteral( "ItemsDock" ) );
  mPanelsMenu->addAction( mItemsDock->toggleViewAction() );

  //items tree widget
  mItemsTreeView = new QgsLayoutItemsListView( mItemsDock, iface() );
  mItemsDock->setWidget( mItemsTreeView );

  addDockWidget( Qt::RightDockWidgetArea, mItemDock );
  addDockWidget( Qt::RightDockWidgetArea, mGeneralDock );
  addDockWidget( Qt::RightDockWidgetArea, mGuideDock );
  addDockWidget( Qt::RightDockWidgetArea, mUndoDock );
  addDockWidget( Qt::RightDockWidgetArea, mItemsDock );

  createLayoutPropertiesWidget();

  mUndoDock->show();
  mItemDock->show();
  mGeneralDock->show();
  mItemsDock->show();

  tabifyDockWidget( mGeneralDock, mUndoDock );
  tabifyDockWidget( mItemDock, mUndoDock );
  tabifyDockWidget( mGeneralDock, mItemDock );
  tabifyDockWidget( mItemDock, mItemsDock );

  QList<QAction *> actions = mPanelsMenu->actions();
  std::sort( actions.begin(), actions.end(), cmpByText_ );
  mPanelsMenu->insertActions( nullptr, actions );

  actions = mToolbarMenu->actions();
  std::sort( actions.begin(), actions.end(), cmpByText_ );
  mToolbarMenu->insertActions( nullptr, actions );

  restoreWindowState();

  //listen out to status bar updates from the view
  connect( mView, &QgsLayoutView::statusMessage, this, &KadasLayoutDesignerDialog::statusMessageReceived );

  connect( QgsProject::instance(), &QgsProject::isDirtyChanged, this, &KadasLayoutDesignerDialog::updateWindowTitle );
}

KadasAppLayoutDesignerInterface *KadasLayoutDesignerDialog::iface()
{
  return mInterface;
}

QgsLayoutGuideWidget *KadasLayoutDesignerDialog::guideWidget()
{
  return mGuideWidget;
}

void KadasLayoutDesignerDialog::showGuideDock( bool show )
{
  mGuideDock->setUserVisible( show );
}

QgsPrintLayout *KadasLayoutDesignerDialog::currentLayout()
{
  return mLayout;
}

void KadasLayoutDesignerDialog::setCurrentLayout( QgsPrintLayout *layout )
{
  if ( !layout )
  {
    mLayout = nullptr;
  }
  else
  {
    if ( mLayout )
    {
      disconnect( mLayout, &QObject::destroyed, this, &QObject::deleteLater );
      disconnect( mLayout, &QgsLayout::backgroundTaskCountChanged, this, &KadasLayoutDesignerDialog::backgroundTaskCountChanged );
    }

    layout->deselectAll();
    mLayout = layout;

    mView->setCurrentLayout( layout );

    // add undo/redo actions which apply to the correct layout undo stack
    delete mUndoAction;
    delete mRedoAction;
    mUndoAction = layout->undoStack()->stack()->createUndoAction( this );
    mUndoAction->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionUndo.svg" ) ) );
    mUndoAction->setShortcuts( QKeySequence::Undo );
    mRedoAction = layout->undoStack()->stack()->createRedoAction( this );
    mRedoAction->setIcon( QgsApplication::getThemeIcon( QStringLiteral( "/mActionRedo.svg" ) ) );
    mRedoAction->setShortcuts( QKeySequence::Redo );
    menuEdit->insertAction( menuEdit->actions().at( 0 ), mRedoAction );
    menuEdit->insertAction( mRedoAction, mUndoAction );
    mLayoutToolbar->addAction( mUndoAction );
    mLayoutToolbar->addAction( mRedoAction );

    connect( mLayout->undoStack(), &QgsLayoutUndoStack::undoRedoOccurredForItems, this, &KadasLayoutDesignerDialog::undoRedoOccurredForItems );
    connect( mActionClearGuides, &QAction::triggered, &mLayout->guides(), [ = ]
    {
      mLayout->guides().clear();
    } );

    mActionShowGrid->setChecked( mLayout->renderContext().gridVisible() );
    mActionSnapGrid->setChecked( mLayout->snapper().snapToGrid() );
    mActionShowGuides->setChecked( mLayout->guides().visible() );
    mActionSnapGuides->setChecked( mLayout->snapper().snapToGuides() );
    mActionSmartGuides->setChecked( mLayout->snapper().snapToItems() );
    mActionShowBoxes->setChecked( mLayout->renderContext().boundingBoxesVisible() );
    mActionShowPage->setChecked( mLayout->renderContext().pagesVisible() );

    mUndoView->setStack( mLayout->undoStack()->stack() );

    mSelectTool->setLayout( layout );
    mItemsTreeView->setCurrentLayout( mLayout );

    connect( mLayout, &QObject::destroyed, this, &QObject::deleteLater );
    connect( mLayout, &QgsLayout::backgroundTaskCountChanged, this, &KadasLayoutDesignerDialog::backgroundTaskCountChanged );

    createLayoutPropertiesWidget();
  }
}

void KadasLayoutDesignerDialog::showItemOptions( QgsLayoutItem *item, bool bringPanelToFront )
{
  if ( mBlockItemOptions )
    return;

  if ( !item )
  {
    delete mItemPropertiesStack->takeMainPanel();
    return;
  }

  if ( auto widget = qobject_cast< QgsLayoutItemBaseWidget * >( mItemPropertiesStack->mainPanel() ) )
  {
    if ( widget->layoutObject() == item )
    {
      // already showing properties for this item - we don't want to create a new panel
      if ( bringPanelToFront )
        mItemDock->setUserVisible( true );

      return;
    }
    else
    {
      // try to reuse
      if ( widget->setItem( item ) )
      {
        if ( bringPanelToFront )
          mItemDock->setUserVisible( true );

        return;
      }
    }
  }

  std::unique_ptr< QgsLayoutItemBaseWidget > widget( QgsGui::layoutItemGuiRegistry()->createItemWidget( item ) );
  delete mItemPropertiesStack->takeMainPanel();

  if ( ! widget )
    return;

  widget->setDesignerInterface( iface() );
  widget->setMasterLayout( mLayout );

  widget->setDockMode( true );
  connect( item, &QgsLayoutItem::destroyed, widget.get(), [this]
  {
    delete mItemPropertiesStack->takeMainPanel();
  } );

  mItemPropertiesStack->setMainPanel( widget.release() );
  if ( bringPanelToFront )
    mItemDock->setUserVisible( true );

}

void KadasLayoutDesignerDialog::open()
{
  show();
  activate();
  if ( mView )
  {
    mView->zoomFull(); // zoomFull() does not work properly until we have called show()
  }
}

void KadasLayoutDesignerDialog::activate()
{
  // bool shown = isVisible();
  show();
  raise();
  setWindowState( windowState() & ~Qt::WindowMinimized );
  activateWindow();
}

void KadasLayoutDesignerDialog::showRulers( bool visible )
{
  //show or hide rulers
  mHorizontalRuler->setVisible( visible );
  mVerticalRuler->setVisible( visible );
  mRulerLayoutFix->setVisible( visible );

  QgsSettings settings;
  settings.setValue( QStringLiteral( "LayoutDesigner/showRulers" ), visible, QgsSettings::App );
}

void KadasLayoutDesignerDialog::showGrid( bool visible )
{
  mLayout->renderContext().setGridVisible( visible );
  mLayout->pageCollection()->redraw();
}

void KadasLayoutDesignerDialog::showBoxes( bool visible )
{
  mLayout->renderContext().setBoundingBoxesVisible( visible );
  mSelectTool->mouseHandles()->update();
}

void KadasLayoutDesignerDialog::showPages( bool visible )
{
  mLayout->renderContext().setPagesVisible( visible );
  mLayout->pageCollection()->redraw();
}

void KadasLayoutDesignerDialog::snapToGrid( bool enabled )
{
  mLayout->snapper().setSnapToGrid( enabled );
}

void KadasLayoutDesignerDialog::showGuides( bool visible )
{
  mLayout->guides().setVisible( visible );
}

void KadasLayoutDesignerDialog::snapToGuides( bool enabled )
{
  mLayout->snapper().setSnapToGuides( enabled );
}

void KadasLayoutDesignerDialog::snapToItems( bool enabled )
{
  mLayout->snapper().setSnapToItems( enabled );
}

void KadasLayoutDesignerDialog::refreshLayout()
{
  if ( !currentLayout() )
  {
    return;
  }

  currentLayout()->refresh();
}

void KadasLayoutDesignerDialog::closeEvent( QCloseEvent * )
{
  emit aboutToClose();
  saveWindowState();
}

void KadasLayoutDesignerDialog::setTitle( const QString &title )
{
  mTitle = title;
  updateWindowTitle();
}

void KadasLayoutDesignerDialog::itemTypeAdded( int id )
{
  if ( QgsGui::layoutItemGuiRegistry()->itemMetadata( id )->flags() & QgsLayoutItemAbstractGuiMetadata::FlagNoCreationTools )
    return;

  QString name = QgsGui::layoutItemGuiRegistry()->itemMetadata( id )->visibleName();
  QString groupId = QgsGui::layoutItemGuiRegistry()->itemMetadata( id )->groupId();
  bool nodeBased = QgsGui::layoutItemGuiRegistry()->itemMetadata( id )->isNodeBased();
  QToolButton *groupButton = nullptr;
  QMenu *itemSubmenu = nullptr;
  if ( !groupId.isEmpty() )
  {
    // find existing group toolbutton and submenu, or create new ones if this is the first time the group has been encountered
    const QgsLayoutItemGuiGroup &group = QgsGui::layoutItemGuiRegistry()->itemGroup( groupId );
    QIcon groupIcon = group.icon.isNull() ? QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddBasicShape.svg" ) ) : group.icon;
    QString groupText = tr( "Add %1" ).arg( group.name );
    if ( mItemGroupToolButtons.contains( groupId ) )
    {
      groupButton = mItemGroupToolButtons.value( groupId );
    }
    else
    {
      QToolButton *groupToolButton = new QToolButton( mToolsToolbar );
      groupToolButton->setIcon( groupIcon );
      groupToolButton->setCheckable( true );
      groupToolButton->setPopupMode( QToolButton::InstantPopup );
      groupToolButton->setAutoRaise( true );
      groupToolButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
      groupToolButton->setToolTip( groupText );
      mToolsToolbar->addWidget( groupToolButton );
      mItemGroupToolButtons.insert( groupId, groupToolButton );
      groupButton = groupToolButton;
    }

    if ( mItemGroupSubmenus.contains( groupId ) )
    {
      itemSubmenu = mItemGroupSubmenus.value( groupId );
    }
    else
    {
      QMenu *groupSubmenu = mItemMenu->addMenu( groupText );
      groupSubmenu->setIcon( groupIcon );
      mItemMenu->addMenu( groupSubmenu );
      mItemGroupSubmenus.insert( groupId, groupSubmenu );
      itemSubmenu = groupSubmenu;
    }
  }

  // update UI for new item type
  QAction *action = new QAction( tr( "Add %1" ).arg( name ), this );
  action->setToolTip( tr( "Adds a new %1 to the layout" ).arg( name ) );
  action->setCheckable( true );
  action->setData( id );
  action->setIcon( QgsGui::layoutItemGuiRegistry()->itemMetadata( id )->creationIcon() );

  mToolsActionGroup->addAction( action );
  if ( itemSubmenu )
    itemSubmenu->addAction( action );
  else
    mItemMenu->addAction( action );

  if ( groupButton )
    groupButton->addAction( action );
  else
    mToolsToolbar->addAction( action );

  connect( action, &QAction::triggered, this, [this, id, nodeBased]()
  {
    activateNewItemCreationTool( id, nodeBased );
  } );
}

void KadasLayoutDesignerDialog::statusZoomCombo_currentIndexChanged( int index )
{
  QVariant data = mStatusZoomCombo->itemData( index );
  if ( data.toInt() == FIT_LAYOUT )
  {
    mView->zoomFull();
  }
  else if ( data.toInt() == FIT_LAYOUT_WIDTH )
  {
    mView->zoomWidth();
  }
  else
  {
    double selectedZoom = data.toDouble();
    if ( mView )
    {
      mView->setZoomLevel( selectedZoom );
      //update zoom combobox text for correct format (one decimal place, trailing % sign)
      whileBlocking( mStatusZoomCombo )->lineEdit()->setText( tr( "%1%" ).arg( selectedZoom * 100.0, 0, 'f', 1 ) );
    }
  }
}

void KadasLayoutDesignerDialog::statusZoomCombo_zoomEntered()
{
  if ( !mView )
  {
    return;
  }

  //need to remove spaces and "%" characters from input text
  QString zoom = mStatusZoomCombo->currentText().remove( QChar( '%' ) ).trimmed();
  mView->setZoomLevel( zoom.toDouble() / 100 );
}

void KadasLayoutDesignerDialog::sliderZoomChanged( int value )
{
  mView->setZoomLevel( value / 100.0 );
}

void KadasLayoutDesignerDialog::updateStatusZoom()
{
  if ( !currentLayout() )
    return;

  double zoomLevel = 0;
  if ( currentLayout()->units() == Qgis::LayoutUnit::Pixels )
  {
    zoomLevel = mView->transform().m11() * 100;
  }
  else
  {
    double dpi = QgsApplication::desktop()->logicalDpiX();
    //monitor dpi is not always correct - so make sure the value is sane
    if ( ( dpi < 60 ) || ( dpi > 1200 ) )
      dpi = 72;

    //pixel width for 1mm on screen
    double scale100 = dpi / 25.4;
    scale100 = currentLayout()->convertFromLayoutUnits( scale100, Qgis::LayoutUnit::Millimeters ).length();
    //current zoomLevel
    zoomLevel = mView->transform().m11() * 100 / scale100;
  }
  whileBlocking( mStatusZoomCombo )->lineEdit()->setText( tr( "%1%" ).arg( zoomLevel, 0, 'f', 1 ) );
  whileBlocking( mStatusZoomSlider )->setValue( static_cast< int >( zoomLevel ) );
}

void KadasLayoutDesignerDialog::updateStatusCursorPos( QPointF position )
{
  if ( !mView->currentLayout() )
  {
    return;
  }

  //convert cursor position to position on current page
  QPointF pagePosition = mLayout->pageCollection()->positionOnPage( position );
  int currentPage = mLayout->pageCollection()->pageNumberForPoint( position );

  QString unit = QgsUnitTypes::toAbbreviatedString( mLayout->units() );
  mStatusCursorXLabel->setText( tr( "x: %1 %2" ).arg( pagePosition.x() ).arg( unit ) );
  mStatusCursorYLabel->setText( tr( "y: %1 %2" ).arg( pagePosition.y() ).arg( unit ) );
  mStatusCursorPageLabel->setText( tr( "page: %1" ).arg( currentPage + 1 ) );
}

void KadasLayoutDesignerDialog::toggleFullScreen( bool enabled )
{
  if ( enabled )
  {
    showFullScreen();
  }
  else
  {
    showNormal();
  }
}

void KadasLayoutDesignerDialog::addPages()
{
  QgsLayoutAddPagesDialog dlg( this );
  dlg.setLayout( mLayout );

  if ( dlg.exec() )
  {
    int firstPagePosition = dlg.beforePage() - 1;
    switch ( dlg.pagePosition() )
    {
      case QgsLayoutAddPagesDialog::BeforePage:
        break;

      case QgsLayoutAddPagesDialog::AfterPage:
        firstPagePosition = firstPagePosition + 1;
        break;

      case QgsLayoutAddPagesDialog::AtEnd:
        firstPagePosition = mLayout->pageCollection()->pageCount();
        break;

    }

    if ( dlg.numberPages() > 1 )
      mLayout->undoStack()->beginMacro( tr( "Add Pages" ) );
    for ( int i = 0; i < dlg.numberPages(); ++i )
    {
      QgsLayoutItemPage *page = new QgsLayoutItemPage( mLayout );
      page->setPageSize( dlg.pageSize() );
      mLayout->pageCollection()->insertPage( page, firstPagePosition + i );
    }
    if ( dlg.numberPages() > 1 )
      mLayout->undoStack()->endMacro();

  }
}

void KadasLayoutDesignerDialog::statusMessageReceived( const QString &message )
{
  mStatusBar->showMessage( message );
}

void KadasLayoutDesignerDialog::undoRedoOccurredForItems( const QSet<QString> &itemUuids )
{
  mBlockItemOptions = true;

  mLayout->deselectAll();
  QgsLayoutItem *focusItem = nullptr;
  for ( const QString &uuid : itemUuids )
  {
    QgsLayoutItem *item = mLayout->itemByUuid( uuid );
    if ( !item )
      continue;

    item->setSelected( true );
    focusItem = item;
  }
  mBlockItemOptions = false;

  if ( focusItem )
    showItemOptions( focusItem, false );
}

void KadasLayoutDesignerDialog::saveAsTemplate()
{
  //show file dialog
  QgsSettings settings;
  QString lastSaveDir = settings.value( QStringLiteral( "lastComposerTemplateDir" ), QDir::homePath(), QgsSettings::App ).toString();
  QString saveFileName = QFileDialog::getSaveFileName(
                           this,
                           tr( "Save template" ),
                           lastSaveDir,
                           tr( "Layout templates" ) + " (*.qpt *.QPT)" );
  if ( saveFileName.isEmpty() )
    return;

  QFileInfo saveFileInfo( saveFileName );
  //check if suffix has been added
  if ( saveFileInfo.suffix().isEmpty() )
  {
    QString saveFileNameWithSuffix = saveFileName.append( ".qpt" );
    saveFileInfo = QFileInfo( saveFileNameWithSuffix );
  }
  settings.setValue( QStringLiteral( "lastComposerTemplateDir" ), saveFileInfo.absolutePath(), QgsSettings::App );

  QgsReadWriteContext context;
  context.setPathResolver( QgsProject::instance()->pathResolver() );
  if ( !currentLayout()->saveAsTemplate( saveFileName, context ) )
  {
    QMessageBox::warning( this, tr( "Save Template" ), tr( "Error creating template file." ) );
  }
}

void KadasLayoutDesignerDialog::addItemsFromTemplate()
{
  if ( !currentLayout() )
    return;

  QgsSettings settings;
  QString openFileDir = settings.value( QStringLiteral( "lastComposerTemplateDir" ), QDir::homePath(), QgsSettings::App ).toString();
  QString openFileString = QFileDialog::getOpenFileName( nullptr, tr( "Load template" ), openFileDir, tr( "Layout templates" ) + " (*.qpt *.QPT)" );

  if ( openFileString.isEmpty() )
  {
    return; //canceled by the user
  }

  QFileInfo openFileInfo( openFileString );
  settings.setValue( QStringLiteral( "LastComposerTemplateDir" ), openFileInfo.absolutePath(), QgsSettings::App );

  QFile templateFile( openFileString );
  if ( !templateFile.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::warning( this, tr( "Load from Template" ), tr( "Could not read template file." ) );
    return;
  }

  QDomDocument templateDoc;
  QgsReadWriteContext context;
  context.setPathResolver( QgsProject::instance()->pathResolver() );
  if ( templateDoc.setContent( &templateFile ) )
  {
    bool ok = false;
    QList< QgsLayoutItem * > items = currentLayout()->loadFromTemplate( templateDoc, context, false, &ok );
    if ( !ok )
    {
      QMessageBox::warning( this, tr( "Load from Template" ), tr( "Could not read template file." ) );
      return;
    }
    else
    {
      whileBlocking( currentLayout() )->deselectAll();
      selectItems( items );
    }
  }
}

void KadasLayoutDesignerDialog::newLayout()
{
  QgsPrintLayout *layout = kApp->createNewPrintLayout();
  if ( layout )
  {
    kApp->showLayoutDesigner( layout );
  }
}


void KadasLayoutDesignerDialog::renameLayout()
{
  QString t = QgsProject::instance()->layoutManager()->generateUniqueTitle( QgsMasterLayoutInterface::PrintLayout );
  QString currentTitle = currentLayout()->name();

  QSet<QString> names;
  for ( QgsMasterLayoutInterface *l : QgsProject::instance()->layoutManager()->layouts() )
  {
    names.insert( l->name() );
  }

  QString newTitle;
  QString chooseMsg = tr( "Enter a unique print layout name:" );
  QString titleMsg = chooseMsg;

  bool valid = false;
  while ( !valid )
  {
    newTitle = QInputDialog::getText( kApp->mainWindow(), tr( "Print Layout Name" ), titleMsg, QLineEdit::Normal, newTitle, &valid );
    if ( !valid )
      return;

    if ( newTitle.isEmpty() )
    {
      titleMsg = chooseMsg + "\n\n" + tr( "Title can not be empty!" );
    }
    else if ( names.contains( newTitle ) )
    {
      titleMsg = chooseMsg + "\n\n" + tr( "Title already exists!" );
    }
    else
    {
      valid = true;
    }
  }

  currentLayout()->setName( newTitle );
}

void KadasLayoutDesignerDialog::deleteLayout()
{
  if ( QMessageBox::question( this, tr( "Delete Layout" ), tr( "Are you sure you want to delete the layout “%1”?" ).arg( currentLayout()->name() ),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
    return;

  kApp->deletePrintLayout( currentLayout() );
  close();
}

void KadasLayoutDesignerDialog::print()
{
  if ( !currentLayout() || currentLayout()->pageCollection()->pageCount() == 0 )
    return;

  // get orientation from first page
  printer()->setPageLayout( currentLayout()->pageCollection()->page( 0 )->pageLayout() );

  QPrintDialog printDialog( printer(), nullptr );
  if ( printDialog.exec() != QDialog::Accepted )
  {
    return;
  }


  mView->setPaintingEnabled( false );
  QgsTemporaryCursorOverride cursorOverride( Qt::BusyCursor );

  QgsLayoutExporter::PrintExportSettings printSettings;
  printSettings.rasterizeWholeImage = mLayout->customProperty( QStringLiteral( "rasterize" ), false ).toBool();
  printSettings.predefinedMapScales = predefinedScales();

  // force a refresh, to e.g. update data defined properties, tables, etc
  mLayout->refresh();

  QgsLayoutExporter exporter( mLayout );
  QString printerName = printer()->printerName();
  QPrinter *p = printer();
  p->setDocName( mLayout->name() );
  QgsLayoutExporter::ExportResult result = exporter.print( *p, printSettings );

  switch ( result )
  {
    case QgsLayoutExporter::Success:
    {
      QString message;
      if ( !printerName.isEmpty() )
      {
        message = tr( "Successfully printed layout to %1." ).arg( printerName );
      }
      else
      {
        message = tr( "Successfully printed layout." );
      }
      mMessageBar->pushMessage( tr( "Print layout" ),
                                message,
                                Qgis::Success, 0 );
      break;
    }

    case QgsLayoutExporter::PrintError:
    {
      QString message;
      if ( !printerName.isEmpty() )
      {
        message =   tr( "Could not create print device for %1." ).arg( printerName );
      }
      else
      {
        message = tr( "Could not create print device." );
      }
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Print Layout" ),
                            message,
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      break;
    }

    case QgsLayoutExporter::MemoryError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Memory Allocation Error" ),
                            tr( "Printing the layout "
                                "resulted in a memory overflow.\n\n"
                                "Please try a lower resolution or a smaller paper size." ),
                            QMessageBox::Ok, QMessageBox::Ok );
      break;

    case QgsLayoutExporter::FileError:
    case QgsLayoutExporter::SvgLayerError:
    case QgsLayoutExporter::IteratorError:
    case QgsLayoutExporter::Canceled:
      // no meaning for PDF exports, will not be encountered
      break;
  }

  mView->setPaintingEnabled( true );
}

void KadasLayoutDesignerDialog::exportToRaster()
{
  QString lastUsedDir = defaultExportPath();
  QString outputFileName = QDir( lastUsedDir ).filePath( QgsFileUtils::stringToSafeFilename( mLayout->name() ) );

  QPair<QString, QString> fileNExt = QgsGuiUtils::getSaveAsImageName( this, tr( "Save Layout As" ), outputFileName );
  this->activateWindow();

  if ( fileNExt.first.isEmpty() )
  {
    return;
  }

  setLastExportPath( fileNExt.first );

  QgsLayoutExporter::ImageExportSettings settings;
  QSize imageSize;
  if ( !getRasterExportSettings( settings, imageSize ) )
    return;

  mView->setPaintingEnabled( false );
  QgsTemporaryCursorOverride cursorOverride( Qt::BusyCursor );

  // force a refresh, to e.g. update data defined properties, tables, etc
  mLayout->refresh();

  QgsLayoutExporter exporter( mLayout );

  QgsLayoutExporter::ExportResult result = exporter.exportToImage( fileNExt.first, settings );

  switch ( result )
  {
    case QgsLayoutExporter::Success:
      mMessageBar->pushMessage( tr( "Export layout" ),
                                tr( "Successfully exported layout to <a href=\"%1\">%2</a>" ).arg( QUrl::fromLocalFile( fileNExt.first ).toString(), QDir::toNativeSeparators( fileNExt.first ) ),
                                Qgis::Success, 0 );
      break;

    case QgsLayoutExporter::PrintError:
    case QgsLayoutExporter::SvgLayerError:
    case QgsLayoutExporter::IteratorError:
    case QgsLayoutExporter::Canceled:
      // no meaning for raster exports, will not be encountered
      break;

    case QgsLayoutExporter::FileError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Image Export Error" ),
                            tr( "Cannot write to %1.\n\nThis file may be open in another application." ).arg( QDir::toNativeSeparators( exporter.errorFile() ) ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      break;

    case QgsLayoutExporter::MemoryError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Image Export Error" ),
                            tr( "Trying to create image %1 (%2×%3 @ %4dpi ) "
                                "resulted in a memory overflow.\n\n"
                                "Please try a lower resolution or a smaller paper size." )
                            .arg( QDir::toNativeSeparators( exporter.errorFile() ) ).arg( imageSize.width() ).arg( imageSize.height() ).arg( settings.dpi ),
                            QMessageBox::Ok, QMessageBox::Ok );
      break;


  }
  mView->setPaintingEnabled( true );
}

void KadasLayoutDesignerDialog::exportToPdf()
{
  const QString exportPath = defaultExportPath();
  QString outputFileName = exportPath + '/' + QgsFileUtils::stringToSafeFilename( mLayout->name() ) + QStringLiteral( ".pdf" );

  outputFileName = QFileDialog::getSaveFileName(
                     this,
                     tr( "Export to PDF" ),
                     outputFileName,
                     tr( "PDF Format" ) + " (*.pdf *.PDF)" );
  this->activateWindow();
  if ( outputFileName.isEmpty() )
  {
    return;
  }

  if ( !outputFileName.endsWith( QLatin1String( ".pdf" ), Qt::CaseInsensitive ) )
  {
    outputFileName += QLatin1String( ".pdf" );
  }

  setLastExportPath( outputFileName );

  QgsLayoutExporter::PdfExportSettings pdfSettings;
  if ( !getPdfExportSettings( pdfSettings ) )
    return;

  mView->setPaintingEnabled( false );
  QgsTemporaryCursorOverride cursorOverride( Qt::BusyCursor );

  pdfSettings.rasterizeWholeImage = mLayout->customProperty( QStringLiteral( "rasterize" ), false ).toBool();

  // force a refresh, to e.g. update data defined properties, tables, etc
  mLayout->refresh();

  QgsLayoutExporter exporter( mLayout );
  QgsLayoutExporter::ExportResult result = exporter.exportToPdf( outputFileName, pdfSettings );

  switch ( result )
  {
    case QgsLayoutExporter::Success:
    {
      mMessageBar->pushMessage( tr( "Export layout" ),
                                tr( "Successfully exported layout to <a href=\"%1\">%2</a>" ).arg( QUrl::fromLocalFile( outputFileName ).toString(), QDir::toNativeSeparators( outputFileName ) ),
                                Qgis::Success, 0 );
      break;
    }

    case QgsLayoutExporter::FileError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Export to PDF" ),
                            tr( "Cannot write to %1.\n\nThis file may be open in another application." ).arg( outputFileName ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      break;

    case QgsLayoutExporter::PrintError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Export to PDF" ),
                            tr( "Could not create print device." ),
                            QMessageBox::Ok,
                            QMessageBox::Ok );
      break;


    case QgsLayoutExporter::MemoryError:
      cursorOverride.release();
      QMessageBox::warning( this, tr( "Export to PDF" ),
                            tr( "Exporting the PDF "
                                "resulted in a memory overflow.\n\n"
                                "Please try a lower resolution or a smaller paper size." ),
                            QMessageBox::Ok, QMessageBox::Ok );
      break;

    case QgsLayoutExporter::SvgLayerError:
    case QgsLayoutExporter::IteratorError:
    case QgsLayoutExporter::Canceled:
      // no meaning for PDF exports, will not be encountered
      break;
  }

  mView->setPaintingEnabled( true );
}

void KadasLayoutDesignerDialog::pageSetup()
{
  if ( currentLayout() && currentLayout()->pageCollection()->pageCount() > 0 )
  {
    printer()->setPageLayout( currentLayout()->pageCollection()->page( 0 )->pageLayout() );
  }

  QPageSetupDialog pageSetupDialog( printer(), this );
  pageSetupDialog.exec();
}

void KadasLayoutDesignerDialog::paste()
{
  QPointF pt = mView->mapFromGlobal( QCursor::pos() );
  //TODO - use a better way of determining whether paste was triggered by keystroke
  //or menu item
  QList< QgsLayoutItem * > items;
  if ( ( pt.x() < 0 ) || ( pt.y() < 0 ) )
  {
    //action likely triggered by menu, paste items in center of screen
    items = mView->pasteItems( QgsLayoutView::PasteModeCenter );
  }
  else
  {
    //action likely triggered by keystroke, paste items at cursor position
    items = mView->pasteItems( QgsLayoutView::PasteModeCursor );
  }

  whileBlocking( currentLayout() )->deselectAll();
  selectItems( items );

  //switch back to select tool so that pasted items can be moved/resized (#8958)
  mView->setTool( mSelectTool );
}

void KadasLayoutDesignerDialog::pasteInPlace()
{
  QList< QgsLayoutItem * > items = mView->pasteItems( QgsLayoutView::PasteModeInPlace );

  whileBlocking( currentLayout() )->deselectAll();
  selectItems( items );

  //switch back to select tool so that pasted items can be moved/resized (#8958)
  mView->setTool( mSelectTool );
}

QgsLayoutView *KadasLayoutDesignerDialog::view()
{
  return mView;
}

void KadasLayoutDesignerDialog::saveWindowState()
{
  QgsSettings settings;
  settings.setValue( QStringLiteral( "LayoutDesigner/geometry" ), saveGeometry(), QgsSettings::App );
  // store the toolbar/dock widget settings using Qt settings API
  settings.setValue( QStringLiteral( "LayoutDesigner/state" ), saveState(), QgsSettings::App );
}

void KadasLayoutDesignerDialog::restoreWindowState()
{
  // restore the toolbar and dock widgets positions using Qt settings API
  QgsSettings settings;

  if ( !restoreState( settings.value( QStringLiteral( "LayoutDesigner/state" ), QByteArray() ).toByteArray() ) )
  {
    QgsDebugMsgLevel( QStringLiteral( "restore of layout UI state failed" ) , 2 );
  }
  // restore window geometry
  if ( !restoreGeometry( settings.value( QStringLiteral( "LayoutDesigner/geometry" ), QgsSettings::App ).toByteArray() ) )
  {
    QgsDebugMsgLevel( QStringLiteral( "restore of layout UI geometry failed" ) , 2 );
    // default to 80% of screen size, at 10% from top left corner
    resize( QDesktopWidget().availableGeometry( this ).size() * 0.8 );
    QSize pos = QDesktopWidget().availableGeometry( this ).size() * 0.1;
    move( pos.width(), pos.height() );
  }
}

void KadasLayoutDesignerDialog::activateNewItemCreationTool( int id, bool nodeBasedItem )
{
  if ( !nodeBasedItem )
  {
    mAddItemTool->setItemMetadataId( id );
    if ( mView )
      mView->setTool( mAddItemTool );
  }
  else
  {
    mAddNodeItemTool->setItemMetadataId( id );
    if ( mView )
      mView->setTool( mAddNodeItemTool );
  }
}

void KadasLayoutDesignerDialog::createLayoutPropertiesWidget()
{
  if ( !mLayout )
  {
    return;
  }

  // update layout based widgets
  QgsLayoutPropertiesWidget *oldLayoutWidget = qobject_cast<QgsLayoutPropertiesWidget *>( mGeneralPropertiesStack->takeMainPanel() );
  delete oldLayoutWidget;
  QgsLayoutGuideWidget *oldGuideWidget = qobject_cast<QgsLayoutGuideWidget *>( mGuideStack->takeMainPanel() );
  delete oldGuideWidget;

  mLayoutPropertiesWidget = new QgsLayoutPropertiesWidget( mGeneralDock, mLayout );
  mLayoutPropertiesWidget->setDockMode( true );
  mLayoutPropertiesWidget->setMasterLayout( mLayout );
  mGeneralPropertiesStack->setMainPanel( mLayoutPropertiesWidget );

  mGuideWidget = new QgsLayoutGuideWidget( mGuideDock, mLayout, mView );
  mGuideWidget->setDockMode( true );
  mGuideStack->setMainPanel( mGuideWidget );
}

void KadasLayoutDesignerDialog::initializeRegistry()
{
  sInitializedRegistry = true;
  auto createPageWidget = ( []( QgsLayoutItem * item )->QgsLayoutItemBaseWidget *
  {
    std::unique_ptr< QgsLayoutPagePropertiesWidget > newWidget = std::make_unique< QgsLayoutPagePropertiesWidget >( nullptr, item );
    return newWidget.release();
  } );

  QgsGui::layoutItemGuiRegistry()->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutPage, QObject::tr( "Page" ), QIcon(), createPageWidget, nullptr, QString(), false, QgsLayoutItemAbstractGuiMetadata::FlagNoCreationTools ) );

}

bool KadasLayoutDesignerDialog::getRasterExportSettings( QgsLayoutExporter::ImageExportSettings &settings, QSize &imageSize )
{
  QSizeF maxPageSize;
  bool hasUniformPageSizes = false;
  double dpi = 300;
  bool cropToContents = false;
  int marginTop = 0;
  int marginRight = 0;
  int marginBottom = 0;
  int marginLeft = 0;
  bool antialias = true;

  // Image size
  if ( mLayout )
  {
    settings.flags = mLayout->renderContext().flags();

    maxPageSize = mLayout->pageCollection()->maximumPageSize();
    hasUniformPageSizes = mLayout->pageCollection()->hasUniformPageSizes();
    dpi = mLayout->renderContext().dpi();

    //get some defaults from the composition
    cropToContents = mLayout->customProperty( QStringLiteral( "imageCropToContents" ), false ).toBool();
    marginTop = mLayout->customProperty( QStringLiteral( "imageCropMarginTop" ), 0 ).toInt();
    marginRight = mLayout->customProperty( QStringLiteral( "imageCropMarginRight" ), 0 ).toInt();
    marginBottom = mLayout->customProperty( QStringLiteral( "imageCropMarginBottom" ), 0 ).toInt();
    marginLeft = mLayout->customProperty( QStringLiteral( "imageCropMarginLeft" ), 0 ).toInt();
    antialias = mLayout->customProperty( QStringLiteral( "imageAntialias" ), true ).toBool();
  }

  QgsLayoutImageExportOptionsDialog imageDlg( this );
  imageDlg.setImageSize( maxPageSize );
  imageDlg.setResolution( dpi );
  imageDlg.setCropToContents( cropToContents );
  imageDlg.setCropMargins( marginTop, marginRight, marginBottom, marginLeft );
  if ( mLayout )
    imageDlg.setGenerateWorldFile( mLayout->customProperty( QStringLiteral( "exportWorldFile" ), false ).toBool() );
  imageDlg.setAntialiasing( antialias );

  if ( !imageDlg.exec() )
    return false;

  imageSize = QSize( imageDlg.imageWidth(), imageDlg.imageHeight() );
  cropToContents = imageDlg.cropToContents();
  imageDlg.getCropMargins( marginTop, marginRight, marginBottom, marginLeft );
  if ( mLayout )
  {
    mLayout->setCustomProperty( QStringLiteral( "imageCropToContents" ), cropToContents );
    mLayout->setCustomProperty( QStringLiteral( "imageCropMarginTop" ), marginTop );
    mLayout->setCustomProperty( QStringLiteral( "imageCropMarginRight" ), marginRight );
    mLayout->setCustomProperty( QStringLiteral( "imageCropMarginBottom" ), marginBottom );
    mLayout->setCustomProperty( QStringLiteral( "imageCropMarginLeft" ), marginLeft );
    mLayout->setCustomProperty( QStringLiteral( "imageAntialias" ), imageDlg.antialiasing() );
  }

  settings.cropToContents = cropToContents;
  settings.cropMargins = QgsMargins( marginLeft, marginTop, marginRight, marginBottom );
  settings.dpi = imageDlg.resolution();
  if ( hasUniformPageSizes )
  {
    settings.imageSize = imageSize;
  }
  settings.generateWorldFile = imageDlg.generateWorldFile();
  settings.predefinedMapScales = predefinedScales();
  settings.flags |= QgsLayoutRenderContext::FlagUseAdvancedEffects;
  if ( imageDlg.antialiasing() )
    settings.flags |= QgsLayoutRenderContext::FlagAntialiasing;
  else
    settings.flags &= ~QgsLayoutRenderContext::FlagAntialiasing;

  return true;
}

bool KadasLayoutDesignerDialog::getPdfExportSettings( QgsLayoutExporter::PdfExportSettings &settings )
{
  Qgis::TextRenderFormat prevTextRenderFormat = mLayout->layoutProject()->labelingEngineSettings().defaultTextRenderFormat();
  bool forceVector = false;
  bool appendGeoreference = true;
  bool includeMetadata = true;
  bool disableRasterTiles = false;
  bool simplify = true;
  bool geoPdf = false;
  bool useOgcBestPracticeFormat = false;
  bool exportGeoPdfFeatures = true;
  QStringList exportThemes;
  if ( mLayout )
  {
    settings.flags = mLayout->renderContext().flags();
    forceVector = mLayout->customProperty( QStringLiteral( "forceVector" ), 0 ).toBool();
    appendGeoreference = mLayout->customProperty( QStringLiteral( "pdfAppendGeoreference" ), 1 ).toBool();
    includeMetadata = mLayout->customProperty( QStringLiteral( "pdfIncludeMetadata" ), 1 ).toBool();
    disableRasterTiles = mLayout->customProperty( QStringLiteral( "pdfDisableRasterTiles" ), 0 ).toBool();
    simplify = mLayout->customProperty( QStringLiteral( "pdfSimplify" ), 1 ).toBool();
    geoPdf = mLayout->customProperty( QStringLiteral( "pdfCreateGeoPdf" ), 0 ).toBool();
    useOgcBestPracticeFormat = mLayout->customProperty( QStringLiteral( "pdfOgcBestPracticeFormat" ), 0 ).toBool();
    exportGeoPdfFeatures = mLayout->customProperty( QStringLiteral( "pdfExportGeoPdfFeatures" ), 1 ).toBool();
    const QString themes = mLayout->customProperty( QStringLiteral( "pdfExportThemes" ) ).toString();
    if ( !themes.isEmpty() )
      exportThemes = themes.split( QStringLiteral( "~~~" ) );
    const int prevLayoutSettingLabelsAsOutlines = mLayout->customProperty( QStringLiteral( "pdfTextFormat" ), -1 ).toInt();
    if ( prevLayoutSettingLabelsAsOutlines >= 0 )
    {
      // previous layout setting takes default over project setting
      prevTextRenderFormat = static_cast< Qgis::TextRenderFormat >( prevLayoutSettingLabelsAsOutlines );
    }
  }

  // open options dialog
  QgsLayoutPdfExportOptionsDialog dialog( this );

  dialog.setTextRenderFormat( prevTextRenderFormat );
  dialog.setForceVector( forceVector );
  dialog.enableGeoreferencingOptions( mLayout && mLayout->referenceMap() && mLayout->referenceMap()->page() == 0 );
  dialog.setGeoreferencingEnabled( appendGeoreference );
  dialog.setMetadataEnabled( includeMetadata );
  dialog.setRasterTilingDisabled( disableRasterTiles );
  dialog.setGeometriesSimplified( simplify );
  dialog.setExportGeoPdf( geoPdf );
  dialog.setUseOgcBestPracticeFormat( useOgcBestPracticeFormat );
  dialog.setExportGeoPdf( exportGeoPdfFeatures );
  dialog.setExportThemes( exportThemes );

  if ( dialog.exec() != QDialog::Accepted )
    return false;

  appendGeoreference = dialog.georeferencingEnabled();
  includeMetadata = dialog.metadataEnabled();
  forceVector = dialog.forceVector();
  disableRasterTiles = dialog.rasterTilingDisabled();
  simplify = dialog.geometriesSimplified();
  Qgis::TextRenderFormat textRenderFormat = dialog.textRenderFormat();
  geoPdf = dialog.exportGeoPdf();
  useOgcBestPracticeFormat = dialog.useOgcBestPracticeFormat();
  exportGeoPdfFeatures = dialog.exportGeoPdf();
  exportThemes = dialog.exportThemes();

  if ( mLayout )
  {
    //save dialog settings
    mLayout->setCustomProperty( QStringLiteral( "forceVector" ), forceVector ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfAppendGeoreference" ), appendGeoreference ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfIncludeMetadata" ), includeMetadata ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfDisableRasterTiles" ), disableRasterTiles ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfTextFormat" ), static_cast< int >( textRenderFormat ) );
    mLayout->setCustomProperty( QStringLiteral( "pdfSimplify" ), simplify ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfCreateGeoPdf" ), geoPdf ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfOgcBestPracticeFormat" ), useOgcBestPracticeFormat ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfExportGeoPdfFeatures" ), exportGeoPdfFeatures ? 1 : 0 );
    mLayout->setCustomProperty( QStringLiteral( "pdfExportThemes" ), exportThemes.join( QStringLiteral( "~~~" ) ) );
  }

  settings.forceVectorOutput = forceVector;
  settings.appendGeoreference = appendGeoreference;
  settings.exportMetadata = includeMetadata;
  settings.textRenderFormat = textRenderFormat;
  settings.simplifyGeometries = simplify;
  settings.writeGeoPdf = geoPdf;
  settings.useOgcBestPracticeFormatGeoreferencing = useOgcBestPracticeFormat;
  settings.useIso32000ExtensionFormatGeoreferencing = !useOgcBestPracticeFormat;
  settings.includeGeoPdfFeatures = exportGeoPdfFeatures;
  settings.exportThemes = exportThemes;
  settings.predefinedMapScales = predefinedScales();

  if ( disableRasterTiles )
    settings.flags = settings.flags | QgsLayoutRenderContext::FlagDisableTiledRasterLayerRenders;
  else
    settings.flags = settings.flags & ~QgsLayoutRenderContext::FlagDisableTiledRasterLayerRenders;

  return true;
}

void KadasLayoutDesignerDialog::loadPredefinedScalesFromProject()
{
  if ( mLayout )
    mLayout->renderContext().setPredefinedScales( predefinedScales() );
}

QVector<double> KadasLayoutDesignerDialog::predefinedScales() const
{
  QgsProject *project = mLayout->layoutProject();
  // first look at project's scales
  QVector< double > projectScales = project->viewSettings()->mapScales();
  bool hasProjectScales( project->viewSettings()->useProjectScales() );
  if ( !hasProjectScales || projectScales.isEmpty() )
  {
    // default to global map tool scales
    QgsSettings settings;
    QString scalesStr( settings.value( QStringLiteral( "Map/scales" ), Qgis::defaultProjectScales() ).toString() );
    QStringList scales = scalesStr.split( ',' );

    for ( auto scaleIt = scales.constBegin(); scaleIt != scales.constEnd(); ++scaleIt )
    {
      QStringList parts( scaleIt->split( ':' ) );
      if ( parts.size() == 2 )
      {
        projectScales.push_back( parts[1].toDouble() );
      }
    }
  }
  return projectScales;
}

QPrinter *KadasLayoutDesignerDialog::printer()
{
  //only create the printer on demand - creating a printer object can be very slow
  //due to QTBUG-3033
  if ( !mPrinter )
    mPrinter = std::make_unique< QPrinter >();

  return mPrinter.get();
}

QString KadasLayoutDesignerDialog::defaultExportPath() const
{
  // first priority - last export folder saved in project
  const QString projectLastExportPath = QgsFileUtils::findClosestExistingPath( QgsProject::instance()->readEntry( QStringLiteral( "Layouts" ), QStringLiteral( "/lastLayoutExportDir" ), QString() ) );
  if ( !projectLastExportPath.isEmpty() )
    return projectLastExportPath;

  // second priority - project home path
  const QString projectHome = QgsFileUtils::findClosestExistingPath( QgsProject::instance()->homePath() );
  if ( !projectHome.isEmpty() )
    return projectHome;

  // last priority - app setting last export folder, with homepath as backup
  QgsSettings s;
  return QgsFileUtils::findClosestExistingPath( s.value( QStringLiteral( "lastLayoutExportDir" ), QDir::homePath(), QgsSettings::App ).toString() );
}

void KadasLayoutDesignerDialog::setLastExportPath( const QString &path ) const
{
  QFileInfo fi( path );
  QString savePath;
  if ( fi.isFile() )
    savePath = fi.path();
  else
    savePath = path;

  QgsProject::instance()->writeEntry( QStringLiteral( "Layouts" ), QStringLiteral( "/lastLayoutExportDir" ), savePath );
  QgsSettings().setValue( QStringLiteral( "lastLayoutExportDir" ), savePath, QgsSettings::App );
}

void KadasLayoutDesignerDialog::updateWindowTitle()
{
  QString title;
  if ( mSectionTitle.isEmpty() )
    title = mTitle;
  else
    title = QStringLiteral( "%1 - %2" ).arg( mTitle, mSectionTitle );

  if ( QgsProject::instance()->isDirty() )
    title.prepend( '*' );

  setWindowTitle( title );
}

void KadasLayoutDesignerDialog::backgroundTaskCountChanged( int total )
{
  if ( total > 1 )
    mStatusBar->showMessage( tr( "Redrawing %1 maps" ).arg( total ) );
  else if ( total == 1 )
    mStatusBar->showMessage( tr( "Redrawing map" ) );
  else
    mStatusBar->clearMessage();

  if ( total == 0 )
  {
    mStatusProgressBar->reset();
    mStatusProgressBar->hide();
  }
  else
  {
    //only call show if not already hidden to reduce flicker
    mStatusProgressBar->setMinimum( 0 );
    mStatusProgressBar->setMaximum( 0 );
    if ( !mStatusProgressBar->isVisible() )
    {
      mStatusProgressBar->show();
    }
  }
}

void KadasLayoutDesignerDialog::selectItems( const QList<QgsLayoutItem *> &items )
{
  for ( QGraphicsItem *item : items )
  {
    if ( item )
    {
      item->setSelected( true );
    }
  }

  //update item panel
  const QList<QgsLayoutItem *> selectedItemList = currentLayout()->selectedLayoutItems();
  if ( !selectedItemList.isEmpty() )
  {
    showItemOptions( selectedItemList.at( 0 ) );
  }
  else
  {
    showItemOptions( nullptr );
  }
}

QgsMessageBar *KadasLayoutDesignerDialog::messageBar()
{
  return mMessageBar;
}

void KadasLayoutDesignerDialog::setSectionTitle( const QString &title )
{
  mSectionTitle = title;
  updateWindowTitle();
  mView->setSectionLabel( title );
}


KadasAppLayoutDesignerInterface::KadasAppLayoutDesignerInterface( KadasLayoutDesignerDialog *dialog )
  : QgsLayoutDesignerInterface( dialog )
  , mDesigner( dialog )
{}


QgsLayout *KadasAppLayoutDesignerInterface::layout()
{
  return mDesigner->currentLayout();
}

QgsMasterLayoutInterface *KadasAppLayoutDesignerInterface::masterLayout()
{
  return mDesigner->currentLayout();
}

void KadasAppLayoutDesignerInterface::activateTool( QgsLayoutDesignerInterface::StandardTool tool )
{
  switch ( tool )
  {
    case QgsLayoutDesignerInterface::ToolMoveItemContent:
      if ( !mDesigner->mActionMoveItemContent->isChecked() )
        mDesigner->mActionMoveItemContent->trigger();
      break;

    case QgsLayoutDesignerInterface::ToolMoveItemNodes:
      if ( !mDesigner->mActionEditNodesItem->isChecked() )
        mDesigner->mActionEditNodesItem->trigger();
      break;
  }
}

void KadasAppLayoutDesignerInterface::setAtlasFeature( const QgsFeature &feature )
{
  // TODO ?
}

QgsLayoutDesignerInterface::ExportResults *KadasAppLayoutDesignerInterface::lastExportResults() const
{
  // TODO ?
  return nullptr;
}
