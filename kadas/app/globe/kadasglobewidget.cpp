/***************************************************************************
    kadasglobewidget.cpp
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

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgslayertree.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/globe/kadasglobewidget.h>


KadasGlobeWidget::KadasGlobeWidget( QAction *action3D, QWidget *parent )
  : QDockWidget( tr( "Globe" ), parent )
{
  setWindowTitle( tr( "Globe" ) );

  QToolButton *layerSelectionButton = new QToolButton( this );
  layerSelectionButton->setAutoRaise( true );
  layerSelectionButton->setText( tr( "Layers" ) );
  layerSelectionButton->setPopupMode( QToolButton::InstantPopup );
  mLayerSelectionMenu = new QMenu( layerSelectionButton );
  layerSelectionButton->setMenu( mLayerSelectionMenu );
  mLayerSelectionMenu->installEventFilter( this );

  QToolButton *syncButton = new QToolButton( this );
  syncButton->setAutoRaise( true );
  syncButton->setToolTip( tr( "Sync extent" ) );
  syncButton->setIcon( QgsApplication::getThemeIcon( "/sync_views.svg" ) );
  syncButton->setIconSize( QSize( 16, 16 ) );
  connect( syncButton, &QToolButton::clicked, this, &KadasGlobeWidget::syncExtent );

  QToolButton *refreshButton = new QToolButton( this );
  refreshButton->setAutoRaise( true );
  refreshButton->setToolTip( tr( "Reload scene" ) );
  refreshButton->setIcon( QgsApplication::getThemeIcon( "/mActionReload.svg" ) );
  refreshButton->setIconSize( QSize( 16, 16 ) );
  connect( refreshButton, &QToolButton::clicked, this, &KadasGlobeWidget::refresh );

  QToolButton *screenshotButton = new QToolButton( this );
  screenshotButton->setAutoRaise( true );
  screenshotButton->setToolTip( tr( "Screenshot" ) );
  screenshotButton->setIcon( QIcon( ":/kadas/icons/camera" ) );
  screenshotButton->setIconSize( QSize( 16, 16 ) );
  connect( screenshotButton, &QToolButton::clicked, this, &KadasGlobeWidget::takeScreenshot );

  QToolButton *settingsButton = new QToolButton( this );
  settingsButton->setAutoRaise( true );
  settingsButton->setToolTip( tr( "Globe settings" ) );
  settingsButton->setIcon( QgsApplication::getThemeIcon( "/mActionOptions.svg" ) );
  settingsButton->setIconSize( QSize( 16, 16 ) );
  connect( settingsButton, &QToolButton::clicked, this, &KadasGlobeWidget::showSettings );

  QToolButton *closeButton = new QToolButton( this );
  closeButton->setAutoRaise( true );
  closeButton->setIcon( QgsApplication::getThemeIcon( "/mActionRemove.svg" ) );
  closeButton->setIconSize( QSize( 12, 12 ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QToolButton::clicked, this, [action3D] { action3D->trigger(); } );

  QWidget *titleWidget = new QWidget( this );
  titleWidget->setObjectName( "globeTitleWidget" );
  titleWidget->setLayout( new QHBoxLayout() );
  titleWidget->layout()->addWidget( layerSelectionButton );
  titleWidget->layout()->addWidget( syncButton );
  titleWidget->layout()->addWidget( refreshButton );
  titleWidget->layout()->addWidget( screenshotButton );
  titleWidget->layout()->addWidget( settingsButton );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( new QLabel( tr( "Globe" ) ) );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 ); // spacer
  titleWidget->layout()->addWidget( closeButton );
  titleWidget->layout()->setContentsMargins( 0, 0, 0, 0 );

  setTitleBarWidget( titleWidget );
  setMinimumSize( 128, 128 );

  connect( kApp->mainWindow()->mapCanvas(), &QgsMapCanvas::layersChanged, this, &KadasGlobeWidget::updateLayerSelectionMenu );
  connect( QgsProject::instance(), &QgsProject::layersAdded, this, &KadasGlobeWidget::updateLayerSelectionMenu );
  connect( QgsProject::instance(), &QgsProject::layerRemoved, this, &KadasGlobeWidget::updateLayerSelectionMenu );

  buildLayerSelectionMenu( true );
}

void KadasGlobeWidget::buildLayerSelectionMenu( bool syncMainLayers )
{
  QgsMapCanvas *mainCanvas = kApp->mainWindow()->mapCanvas();

  QStringList prevLayers;
  QStringList prevDisabledLayerIds;
  QStringList prevEnabledLayerIds;
  for ( QAction *action : mLayerSelectionMenu->actions() )
  {
    prevLayers.append( action->data().toString() );
    if ( !action->isChecked() )
    {
      prevDisabledLayerIds.append( action->data().toString() );
    }
    else
    {
      prevEnabledLayerIds.append( action->data().toString() );
    }
  }

  mLayerSelectionMenu->clear();
  mLayerSelectionMenu->addAction( tr( "Sync with main view" ), this, [this] { buildLayerSelectionMenu( true ); } );
  mLayerSelectionMenu->addSeparator();

  QString heightmap = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  // Use layerTreeRoot to get layers ordered as in the layer tree
  for ( QgsLayerTreeLayer *layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer *layer = layerTreeLayer->layer();
    if ( !layer )
    {
      continue;
    }
    connect( layer, &QgsMapLayer::nameChanged, this, &KadasGlobeWidget::updateLayerSelectionMenu, Qt::UniqueConnection );
    QAction *layerAction = new QAction( layer->name(), mLayerSelectionMenu );
    layerAction->setData( layer->id() );
    // Check if was not previously unchecked, unless it is a new layer with url=http in datasource
    layerAction->setCheckable( true );
    bool wasUnchecked = prevDisabledLayerIds.contains( layer->id() );
    bool isNew = !prevLayers.contains( layer->id() );
    bool isRemote = layer->source().contains( "url=http" );
    QgsLayerTreeNode *parentGroup = QgsProject::instance()->layerTreeRoot()->findLayer( layer )->parent();
    bool isBasemap = parentGroup && parentGroup->name() == "Basemaps";
    bool isHeightmap = layer->id() == heightmap;
    bool isVisibleInMainCanvas = mainCanvas->layers().contains( layer );
    if ( syncMainLayers )
    {
      layerAction->setChecked( isVisibleInMainCanvas && !isBasemap );
    }
    else
    {
      layerAction->setChecked( !wasUnchecked && !( isNew && ( isRemote || isHeightmap || isBasemap ) ) );
    }
    connect( layerAction, &QAction::toggled, this, &KadasGlobeWidget::layersChanged );
    mLayerSelectionMenu->addAction( layerAction );
  }
  if ( prevEnabledLayerIds != getSelectedLayerIds() )
    emit layersChanged();
}

QStringList KadasGlobeWidget::getSelectedLayerIds() const
{
  QStringList selectedLayerIds;
  for ( QAction *layerAction : mLayerSelectionMenu->actions() )
  {
    if ( layerAction->isChecked() )
    {
      selectedLayerIds.append( layerAction->data().toString() );
    }
  }
  return selectedLayerIds;
}

void KadasGlobeWidget::contextMenuEvent( QContextMenuEvent *e )
{
  e->accept();
}

bool KadasGlobeWidget::eventFilter( QObject *obj, QEvent *ev )
{
  if ( obj == mLayerSelectionMenu && ( ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::MouseButtonRelease ) )
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
