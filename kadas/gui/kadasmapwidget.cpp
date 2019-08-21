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


#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreelayer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadasmapcanvasitem.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/kadasmapwidget.h>
#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/gui/maptools/kadasmaptoolpan.h>

KadasMapWidget::KadasMapWidget( int number, const QString &id, const QString &title, QgsMapCanvas *masterCanvas, QWidget *parent )
  : QDockWidget( parent ), mId( id ), mNumber( number ), mMasterCanvas( masterCanvas ), mUnsetFixedSize( true )
{
  QgsSettings settings;

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
  mLockViewButton->setIcon( QIcon( ":/images/themes/default/unlocked.svg" ) );
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

  mCloseButton = new QToolButton( this );
  mCloseButton->setAutoRaise( true );
  mCloseButton->setIcon( QIcon( ":/images/themes/default/mActionRemove.svg" ) );
  mCloseButton->setIconSize( QSize( 12, 12 ) );
  mCloseButton->setToolTip( tr( "Close" ) );
  connect( mCloseButton, &QToolButton::clicked, this, &KadasMapWidget::closeMapWidget );

  QWidget *titleWidget = new QWidget( this );
  titleWidget->setObjectName( "mapWidgetTitleWidget" );
  titleWidget->setLayout( new QHBoxLayout() );
  titleWidget->layout()->addWidget( mLayerSelectionButton );
  titleWidget->layout()->addWidget( mLockViewButton );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 );   // spacer
  titleWidget->layout()->addWidget( mTitleStackedWidget );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 );   // spacer
  titleWidget->layout()->addWidget( mCloseButton );
  titleWidget->layout()->setContentsMargins( 0, 0, 0, 0 );

  setWindowTitle( mTitleLineEdit->text() );
  setTitleBarWidget( titleWidget );

  mMapCanvas = new QgsMapCanvas( this );
  mMapCanvas->setCanvasColor( Qt::transparent );
  mMapCanvas->setMapUpdateInterval( 1000 );
//  mMapCanvas->enableAntiAliasing( mMasterCanvas->antiAliasingEnabled() );
//  QgsMapCanvas::WheelAction wheelAction = static_cast<QgsMapCanvas::WheelAction>( settings.value( "/Qgis/wheel_action", "0" ).toInt() );
//  double zoomFactor = settings.value( "/Qgis/zoom_factor", "2.0" ).toDouble();
//  mMapCanvas->setWheelAction( wheelAction, zoomFactor );
  setWidget( mMapCanvas );

  KadasMapToolPan *mapTool = new KadasMapToolPan( mMapCanvas, false );
  mapTool->setParent( mMapCanvas );
  mMapCanvas->setMapTool( mapTool );

  for ( const KadasMapItem *item : KadasMapCanvasItemManager::items() )
  {
    addMapCanvasItem( item );
  }

  connect( mMasterCanvas, &QgsMapCanvas::extentsChanged, this, &KadasMapWidget::syncCanvasExtents );
  connect( mMasterCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasMapWidget::updateMapProjection );
//  connect( mMasterCanvas, &QgsMapCanvas::layersChanged, this, &KadasMapWidget::updateLayerSelectionMenu ); // TODO, neccessary?
  connect( QgsProject::instance(), &QgsProject::layersAdded, this, &KadasMapWidget::updateLayerSelectionMenu );
  connect( QgsProject::instance(), &QgsProject::layerRemoved, this, &KadasMapWidget::updateLayerSelectionMenu );
  connect( mMapCanvas, &QgsMapCanvas::xyCoordinates, mMasterCanvas, &QgsMapCanvas::xyCoordinates );
  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemAdded, this, &KadasMapWidget::addMapCanvasItem );
  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemWillBeRemoved, this, &KadasMapWidget::removeMapCanvasItem );

  updateLayerSelectionMenu();
  mMapCanvas->setRenderFlag( false );
  updateMapProjection();
  mMapCanvas->setExtent( mMasterCanvas->extent() );
  mMapCanvas->setRenderFlag( true );
}

KadasMapWidget::~KadasMapWidget()
{
  emit aboutToBeDestroyed();
}

void KadasMapWidget::setInitialLayers( const QStringList &initialLayers, bool updateMenu )
{
  mInitialLayers = initialLayers;
  if ( updateMenu )
  {
    updateLayerSelectionMenu();
  }
}

QStringList KadasMapWidget::getLayers() const
{
  QStringList layers;
  for ( QAction *layerAction : mLayerSelectionMenu->actions() )
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
    mLockViewButton->setIcon( QIcon( ":/images/themes/default/locked.svg" ) );
  }
  else
  {
    mLockViewButton->setIcon( QIcon( ":/images/themes/default/unlocked.svg" ) );
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
  for ( QAction *action : mLayerSelectionMenu->actions() )
  {
    prevLayers.append( action->data().toString() );
    if ( !action->isChecked() )
    {
      prevDisabledLayers.append( action->data().toString() );
    }
  }
  QStringList masterLayers;
  for ( const QgsMapLayer *layer : mMasterCanvas->layers() )
  {
    masterLayers.append( layer->id() );
  }

  mLayerSelectionMenu->clear();
  // Use layerTreeRoot to get layers ordered as in the layer tree
  for ( QgsLayerTreeLayer *layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer *layer = layerTreeLayer->layer();
    if ( !layer )
    {
      continue;
    }
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
      bool isNewEnabledLayer = !prevLayers.contains( layer->id() ) && masterLayers.contains( layer->id() );
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
  for ( QAction *layerAction : mLayerSelectionMenu->actions() )
  {
    if ( layerAction->isChecked() )
    {
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerAction->data().toString() );
      connect( layer, &QgsMapLayer::repaintRequested, mMapCanvas, &QgsMapCanvas::refresh );
      layerSet.append( layer );
    }
  }
  for ( QgsMapLayer *layer : mMapCanvas->layers() )
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
    setFixedSize( QSize() );
    setMinimumSize( 200, 200 );
    setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
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
  else if ( obj == mTitleLineEdit && ev->type() == QEvent::KeyPress && (
              static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Enter ||
              static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Return ||
              static_cast<QKeyEvent *>( ev )->key() == Qt::Key_Escape ) )
  {
    setWindowTitle( mTitleLineEdit->text() );
    mTitleLabel->setText( mTitleLineEdit->text() );
    mTitleStackedWidget->setCurrentWidget( mTitleLabel );
    return true;
  }
  else
  {
    return QObject::eventFilter( obj, ev );
  }
}

void KadasMapWidget::addMapCanvasItem( const KadasMapItem *item )
{
  KadasMapCanvasItem *canvasItem = new KadasMapCanvasItem( item, mMapCanvas );
  Q_UNUSED( canvasItem );  //item is already added automatically to canvas scene
}

void KadasMapWidget::removeMapCanvasItem( const KadasMapItem *item )
{
  for ( QGraphicsItem *canvasItem : mMapCanvas->items() )
  {
    if ( dynamic_cast<KadasMapCanvasItem *>( canvasItem ) && static_cast<KadasMapCanvasItem *>( canvasItem )->mapItem() == item )
    {
      delete canvasItem;
    }
  }
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
