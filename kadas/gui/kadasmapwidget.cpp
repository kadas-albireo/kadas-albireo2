/***************************************************************************
    kadasmapwidget.cpp
    ------------------
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


#include <algorithm>

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPalette>
#include <QStackedWidget>
#include <QToolButton>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreelayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayerelevationproperties.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>
#include <qgis/qgselevationcontrollerwidget.h>
#include <qgis/qgselevationutils.h>
#include <qgis/qgsmathutils.h>
#include <qgis/qgsrangeslider.h>
#include <qgis/qgsrasterlayer.h>


#include "kadas/gui/kadasmapwidget.h"
#include "kadas/gui/maptools/kadasmaptoolpan.h"

KadasMapWidget::KadasMapWidget( int number, const QString &id, const QString &title, QgsMapCanvas *masterCanvas, QWidget *parent )
  : QDockWidget( parent )
  , mNumber( number )
  , mId( id )
  , mMasterCanvas( masterCanvas )
{
  mLayerSelectionButton = new QToolButton( this );
  mLayerSelectionButton->setAutoRaise( true );
  mLayerSelectionButton->setText( tr( "Layers" ) );
  mLayerSelectionButton->setPopupMode( QToolButton::InstantPopup );
  mLayerSelectionMenu = new QMenu( mLayerSelectionButton );
  mLayerSelectionButton->setMenu( mLayerSelectionMenu );

  mLockViewButton = new QToolButton( this );
  mLockViewButton->setAutoRaise( true );
  mLockViewButton->setToolTip( tr( "Lock with main view" ) );
  mLockViewButton->setCheckable( true );
  mLockViewButton->setIcon( QIcon( ":/kadas/icons/unlocked" ) );
  mLockViewButton->setIconSize( QSize( 12, 12 ) );
  connect( mLockViewButton, &QToolButton::toggled, this, &KadasMapWidget::setCanvasLocked );

  mTitleStackedWidget = new QStackedWidget( this );
  mTitleLabel = new QLabel( title );
  mTitleLabel->setCursor( Qt::IBeamCursor );
  mTitleStackedWidget->addWidget( mTitleLabel );

  mTitleLineEdit = new QLineEdit( title, this );
  mTitleStackedWidget->addWidget( mTitleLineEdit );

  mTitleLabel->installEventFilter( this );
  mTitleLineEdit->installEventFilter( this );
  mLayerSelectionMenu->installEventFilter( this );

  mCloseButton = new QToolButton( this );
  mCloseButton->setAutoRaise( true );
  mCloseButton->setIcon( QgsApplication::getThemeIcon( "/mActionRemove.svg" ) );
  mCloseButton->setIconSize( QSize( 12, 12 ) );
  mCloseButton->setToolTip( tr( "Close" ) );
  connect( mCloseButton, &QToolButton::clicked, this, &KadasMapWidget::closeMapWidget );

  QWidget *titleWidget = new QWidget( this );
  titleWidget->setObjectName( "mapWidgetTitleWidget" );
  titleWidget->setLayout( new QHBoxLayout() );
  titleWidget->layout()->addWidget( mLayerSelectionButton );
  titleWidget->layout()->addWidget( mLockViewButton );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( mTitleStackedWidget );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( mCloseButton );
  titleWidget->layout()->setContentsMargins( 0, 0, 0, 0 );

  setWindowTitle( mTitleLineEdit->text() );
  setTitleBarWidget( titleWidget );

  mMapCanvas = new QgsMapCanvas( this );
  mMapCanvas->setFlags( Qgis::MapCanvasFlag::ShowMainAnnotationLayer );
  mMapCanvas->setCanvasColor( Qt::transparent );
  mMapCanvas->enableAntiAliasing( mMasterCanvas->antiAliasingEnabled() );
  mMapCanvas->enableMapTileRendering( mMasterCanvas->mapSettings().flags() & Qgis::MapSettingsFlag::RenderMapTile );
  mMapCanvas->setMapUpdateInterval( mMasterCanvas->mapUpdateInterval() );
  mMapCanvas->setCachingEnabled( mMasterCanvas->isCachingEnabled() );
  mMapCanvas->setParallelRenderingEnabled( mMasterCanvas->isParallelRenderingEnabled() );
  mMapCanvas->setPreviewJobsEnabled( mMasterCanvas->previewJobsEnabled() );
  setWidget( mMapCanvas );

  KadasMapToolPan *mapTool = new KadasMapToolPan( mMapCanvas, false );
  mapTool->setParent( mMapCanvas );
  mMapCanvas->setMapTool( mapTool );

  connect( mMasterCanvas, &QgsMapCanvas::extentsChanged, this, &KadasMapWidget::syncCanvasExtents );
  connect( mMasterCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasMapWidget::updateMapProjection );
  connect( QgsProject::instance()->layerTreeRoot(), &QgsLayerTree::layerOrderChanged, this, &KadasMapWidget::updateLayerSelectionMenu );
  connect( mMapCanvas, &QgsMapCanvas::xyCoordinates, mMasterCanvas, &QgsMapCanvas::xyCoordinates );
  connect( mMapCanvas, &QgsMapCanvas::layersChanged, this, &KadasMapWidget::updateElevationControllerVisibility );

  const QList<QgsMapLayer *> layers = mMasterCanvas->layers();
  for ( QgsMapLayer *layer : layers )
  {
    mInitialLayers.append( layer->id() );
  }
  updateLayerSelectionMenu();
  mMapCanvas->setRenderFlag( false );
  updateMapProjection();
  mMapCanvas->setExtent( mMasterCanvas->extent() );
  mMapCanvas->setRenderFlag( true );
}

KadasMapWidget::~KadasMapWidget()
{
  mMapCanvas->cancelJobs();
  emit aboutToBeDestroyed();
}

void KadasMapWidget::setInitialLayers( const QStringList &initialLayers )
{
  mInitialLayers = initialLayers;
  updateLayerSelectionMenu();
}

QStringList KadasMapWidget::getLayers() const
{
  QStringList layers;
  const QList<QAction *> actions = mLayerSelectionMenu->actions();
  for ( QAction *layerAction : actions )
  {
    if ( layerAction->isChecked() )
    {
      layers.append( layerAction->data().toString() );
    }
  }
  return layers;
}

QgsRectangle KadasMapWidget::getMapExtent() const
{
  return mMapCanvas->extent();
}

void KadasMapWidget::setMapExtent( const QgsRectangle &extent )
{
  mMapCanvas->setExtent( extent );
  mMapCanvas->refresh();
}

namespace
{
  //! Drops every spacer from \a layout, so the remaining widgets keep no slack between them.
  void removeSpacers( QBoxLayout *layout )
  {
    for ( int i = layout->count() - 1; i >= 0; --i )
    {
      if ( layout->itemAt( i )->spacerItem() )
        delete layout->takeAt( i );
    }
  }

  //! Returns the first widget held by \a layout, skipping \a ignore.
  QWidget *firstWidget( QBoxLayout *layout, QWidget *ignore = nullptr )
  {
    for ( int i = 0; i < layout->count(); ++i )
    {
      QWidget *widget = layout->itemAt( i )->widget();
      if ( widget && widget != ignore )
        return widget;
    }
    return nullptr;
  }
} // namespace

void KadasMapWidget::adjustElevationControllerLayout( QgsElevationControllerWidget *controller )
{
  if ( !controller )
    return;

  // By default the elevation controller lays out the slider on the left with its labels on the
  // right, and the settings button above the slider. In kadas the controller sits on the right
  // edge of the canvas, so the whole thing reads better mirrored: labels first, then the slider
  // flush against the edge, with the settings button following the slider rather than the labels.
  // Instead of patching QGIS, rearrange the layouts at runtime.
  QgsRangeSlider *slider = controller->slider();
  QLayout *rootLayout = controller->layout();
  if ( !slider || !rootLayout )
    return;

  // The slider fills the span between its two handles with QPalette::Highlight at a fixed alpha,
  // and the controller starts with the whole range selected, so a saturated accent reads as a
  // coloured slab painted across the map rather than as a selection. The palette is the only
  // handle we have on this: the groove, the handles and the band are all drawn with a null
  // widget pointer, which leaves style sheet rules inert. So pick a muted, low saturation tone -
  // kadas' own slate - which sits over map imagery as a shaded track instead of a highlight.
  QPalette sliderPalette = slider->palette();
  sliderPalette.setColor( QPalette::Highlight, QColor( "#5D7081" ) );
  slider->setPalette( sliderPalette );

  QBoxLayout *sliderLayout = nullptr;
  QBoxLayout *buttonLayout = nullptr;
  for ( int i = 0; i < rootLayout->count(); ++i )
  {
    QBoxLayout *childLayout = qobject_cast<QBoxLayout *>( rootLayout->itemAt( i )->layout() );
    if ( !childLayout )
      continue;

    if ( childLayout->indexOf( slider ) >= 0 )
      sliderLayout = childLayout;
    else
      buttonLayout = childLayout;
  }

  if ( sliderLayout )
  {
    // the labels are the other widget sharing the layout with the slider
    QWidget *labels = firstWidget( sliderLayout, slider );
    if ( labels && sliderLayout->indexOf( labels ) > sliderLayout->indexOf( slider ) )
    {
      sliderLayout->removeWidget( labels );
      sliderLayout->removeWidget( slider );
      sliderLayout->insertWidget( 0, labels, 1 );
      sliderLayout->insertWidget( 1, slider );
    }
    // the labels absorb the slack, so dropping the trailing spacer leaves the slider flush right
    removeSpacers( sliderLayout );
  }

  QWidget *button = buttonLayout ? firstWidget( buttonLayout ) : nullptr;
  if ( button )
  {
    // move the button over the slider column instead of over the labels
    removeSpacers( buttonLayout );
    buttonLayout->removeWidget( button );
    buttonLayout->addStretch();
    buttonLayout->addWidget( button );
  }

  QToolButton *settingsButton = qobject_cast<QToolButton *>( button );
  QMenu *menu = controller->menu();
  if ( settingsButton && menu )
  {
    // The controller hugs the right edge of the canvas, so the settings menu has to open towards
    // the map. QToolButton cannot be talked into this on its own: it only mirrors the popup when
    // it knows the menu size hint, which Qt deliberately skips for menus having aboutToShow()
    // receivers - and the controller connects to that signal. So drive the popup ourselves.
    settingsButton->setMenu( nullptr );
    settingsButton->setPopupMode( QToolButton::DelayedPopup );
    QObject::connect( settingsButton, &QToolButton::clicked, menu, [settingsButton, menu] {
      const QPoint topLeft( settingsButton->width() - menu->sizeHint().width(), settingsButton->height() );
      menu->popup( settingsButton->mapToGlobal( topLeft ) );
    } );
  }
}

bool KadasMapWidget::hasVisibleElevationLayer( QgsMapCanvas *canvas )
{
  if ( !canvas )
    return false;

  // expand group layers, so an elevation layer nested in a group is still matched
  const QList<QgsMapLayer *> layers = canvas->layers( true );
  return std::any_of( layers.begin(), layers.end(), []( QgsMapLayer *layer ) { return layer && layer->elevationProperties() && layer->elevationProperties()->hasElevation(); } );
}

void KadasMapWidget::updateElevationControllerVisibility()
{
  if ( !mElevationController )
    return;

  // hiding the heightmap alone does not retire the controller, it still filters any
  // other elevation layer left on the map
  const bool hasElevationLayer = hasVisibleElevationLayer( mMapCanvas );
  mElevationController->setVisible( hasElevationLayer );

  // a hidden controller must not keep filtering the map, the user has no way to undo it.
  // the range itself is kept, so showing the controller again restores the filter as it was
  mMapCanvas->setZRange( hasElevationLayer ? mElevationController->range() : QgsDoubleRange() );
}

void KadasMapWidget::setElevationController()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    removeElevationController();
    return;
  }

  if ( !mElevationController )
  {
    mElevationController = new QgsElevationControllerWidget( this );
    mElevationController->setMapCanvas( mMapCanvas );
    adjustElevationControllerLayout( mElevationController );
    connect( mElevationController, &QgsElevationControllerWidget::rangeChanged, mMapCanvas, &QgsMapCanvas::setZRange );
    mMapCanvas->addOverlayWidget( mElevationController, Qt::Edge::RightEdge );
  }

  // Use QGIS' elevation utils as the single source of truth for the range. For
  // a raster elevation surface this samples the band statistics rather than
  // reading every pixel.
  const QgsDoubleRange range = QgsElevationUtils::calculateZRangeForLayers( { layer } );
  if ( !range.isInfinite() && !range.isEmpty() )
  {
    // expand to round values, matching what the controller's own limits menu does
    const QgsDoubleRange rounded = QgsMathUtils::roundedRange( range );
    mElevationController->setRangeLimits( rounded );
    mElevationController->setRange( rounded );
  }

  updateElevationControllerVisibility();
}

void KadasMapWidget::removeElevationController()
{
  if ( mElevationController )
  {
    delete mElevationController;
    mElevationController = nullptr;
    // dropping the controller must not leave its filter behind
    mMapCanvas->setZRange( QgsDoubleRange() );
    return;
  }
}

bool KadasMapWidget::getLocked() const
{
  return mLockViewButton->isChecked();
}

void KadasMapWidget::setLocked( bool locked )
{
  mLockViewButton->setChecked( locked );
}

void KadasMapWidget::setCanvasLocked( bool locked )
{
  if ( locked )
  {
    mLockViewButton->setIcon( QIcon( ":/kadas/icons/locked" ) );
  }
  else
  {
    mLockViewButton->setIcon( QIcon( ":/kadas/icons/unlocked" ) );
  }
  if ( locked )
  {
    mMapCanvas->setEnabled( false );
    syncCanvasExtents();
  }
  else
  {
    mMapCanvas->setEnabled( true );
  }
}

void KadasMapWidget::syncCanvasExtents()
{
  if ( mLockViewButton->isChecked() )
  {
    QgsPointXY center = mMasterCanvas->extent().center();
    double w = width() * mMasterCanvas->mapUnitsPerPixel();
    double h = height() * mMasterCanvas->mapUnitsPerPixel();
    setMapExtent( QgsRectangle( center.x() - .5 * w, center.y() - .5 * h, center.x() + .5 * w, center.y() + .5 * h ) );
  }
}

void KadasMapWidget::updateLayerSelectionMenu()
{
  QStringList prevDisabledLayers;
  QStringList prevLayers;
  const QList<QAction *> actions = mLayerSelectionMenu->actions();
  for ( QAction *action : actions )
  {
    prevLayers.append( action->data().toString() );
    if ( !action->isChecked() )
    {
      prevDisabledLayers.append( action->data().toString() );
    }
  }
  mLayerSelectionMenu->clear();
  mLayerSelectionMenu->addAction( tr( "Sync with main view" ), this, [this] {
    const QList<QgsMapLayer *> layers = mMasterCanvas->layers();
    for ( QgsMapLayer *layer : layers )
    {
      mInitialLayers.append( layer->id() );
    }
    updateLayerSelectionMenu();
  } );
  mLayerSelectionMenu->addSeparator();
  // Use layerTreeRoot to get layers ordered as in the layer tree
  for ( QgsLayerTreeLayer *layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer *layer = layerTreeLayer->layer();
    if ( !layer )
    {
      continue;
    }
    connect( layer, &QgsMapLayer::nameChanged, this, &KadasMapWidget::updateLayerSelectionMenu, Qt::UniqueConnection );
    QAction *layerAction = new QAction( layer->name(), mLayerSelectionMenu );
    layerAction->setData( layer->id() );
    layerAction->setCheckable( true );
    if ( !mInitialLayers.isEmpty() )
    {
      layerAction->setChecked( mInitialLayers.contains( layer->id() ) );
    }
    else
    {
      bool wasDisabled = prevDisabledLayers.contains( layer->id() );
      bool isNewEnabledLayer = !prevLayers.contains( layer->id() );
      layerAction->setChecked( ( prevLayers.contains( layer->id() ) && !wasDisabled ) || isNewEnabledLayer );
    }
    connect( layerAction, &QAction::toggled, this, &KadasMapWidget::updateLayerSet );
    mLayerSelectionMenu->addAction( layerAction );
  }
  updateLayerSet();
  mInitialLayers.clear();
}

void KadasMapWidget::updateLayerSet()
{
  QList<QgsMapLayer *> layerSet;
  const QList<QAction *> actions = mLayerSelectionMenu->actions();
  for ( QAction *layerAction : actions )
  {
    if ( layerAction->isChecked() )
    {
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerAction->data().toString() );
      connect( layer, &QgsMapLayer::repaintRequested, mMapCanvas, &QgsMapCanvas::refresh );
      layerSet.append( layer );
    }
  }
  const QList<QgsMapLayer *> layers = mMapCanvas->layers();
  for ( QgsMapLayer *layer : layers )
  {
    disconnect( layer, &QgsMapLayer::repaintRequested, mMapCanvas, &QgsMapCanvas::refresh );
  }

  mMapCanvas->setLayers( layerSet );
}

void KadasMapWidget::updateMapProjection()
{
  mMapCanvas->setDestinationCrs( mMasterCanvas->mapSettings().destinationCrs() );
}

void KadasMapWidget::showEvent( QShowEvent * )
{
  if ( mUnsetFixedSize )
  {
    // Clear previously set fixed size - which was just used to enforce the initial dimensions...
    mUnsetFixedSize = false;
    widget()->setMinimumSize( 0, 0 );
    widget()->setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
  }
}

bool KadasMapWidget::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mTitleLabel && ev->type() == QEvent::MouseButtonPress )
  {
    mTitleStackedWidget->setCurrentWidget( mTitleLineEdit );
    mTitleLineEdit->setText( mTitleLabel->text() );
    mTitleLineEdit->setFocus();
    mTitleLineEdit->selectAll();
    return true;
  }
  else if ( obj == mTitleLineEdit && ev->type() == QEvent::FocusOut )
  {
    setWindowTitle( mTitleLineEdit->text() );
    mTitleLabel->setText( mTitleLineEdit->text() );
    mTitleStackedWidget->setCurrentWidget( mTitleLabel );
    return true;
  }
  else if (
    obj == mTitleLineEdit
    && ev->type() == QEvent::KeyPress
    && ( static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Enter || static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Return || static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Escape )
  )
  {
    setWindowTitle( mTitleLineEdit->text() );
    mTitleLabel->setText( mTitleLineEdit->text() );
    mTitleStackedWidget->setCurrentWidget( mTitleLabel );
    return true;
  }
  else if ( obj == mLayerSelectionMenu && ( ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::MouseButtonRelease ) )
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( ev );
    QAction *action = mLayerSelectionMenu->actionAt( mouseEvent->pos() );
    if ( action )
    {
      if ( ev->type() == QEvent::MouseButtonRelease )
      {
        action->trigger();
      }
      return true;
    }
  }

  return QObject::eventFilter( obj, ev );
}

void KadasMapWidget::contextMenuEvent( QContextMenuEvent *e )
{
  e->accept();
}

void KadasMapWidget::closeMapWidget()
{
  close();
  if ( mMapCanvas->isDrawing() )
  {
    connect( mMapCanvas, &QgsMapCanvas::renderComplete, this, &KadasMapWidget::deleteLater );
  }
  else
  {
    deleteLater();
  }
}
