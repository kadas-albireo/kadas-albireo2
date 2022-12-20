/***************************************************************************
    kadaslayertreeviewmenuprovider.cpp
    ----------------------------------
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

#include <QMenu>
#include <QWidgetAction>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreeviewdefaultactions.h>
#include <qgis/qgspluginlayerregistry.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasterrenderer.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadaspluginlayer.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadaslayerrefreshmanager.h>
#include <kadas/app/kadaslayertreeviewmenuprovider.h>
#include <kadas/app/kadasmainwindow.h>

KadasLayerTreeViewMenuProvider::KadasLayerTreeViewMenuProvider( QgsLayerTreeView *view ) :
  mView( view )
{
}

QMenu *KadasLayerTreeViewMenuProvider::createContextMenu()
{
  if ( !mView )
  {
    return nullptr;
  }
  QMenu *menu = new QMenu;

  QgsLayerTreeViewDefaultActions *actions = mView->defaultActions();

  QList<QgsLayerTreeNode *> selected = mView->selectedNodes();
  if ( selected.isEmpty() )
  {
    menu->addAction( actions->actionAddGroup( menu ) );
  }
  else if ( selected.size() == 1 )
  {
    QgsLayerTreeNode *node = selected[0];
    if ( QgsLayerTree::isGroup( node ) )
    {
      QAction *renameAction = actions->actionRenameGroupOrLayer( menu );
      renameAction->setIcon( QIcon( ":/kadas/icons/rename" ) );
      menu->addAction( actions->actionZoomToGroup( kApp->mainWindow()->mapCanvas(), menu ) );
      menu->addAction( renameAction );
      menu->addAction( actions->actionMutuallyExclusiveGroup( menu ) );
      menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), this, &KadasLayerTreeViewMenuProvider::removeLayerTreeItems );
    }
    else if ( QgsLayerTree::isLayer( node ) && QgsLayerTree::toLayer( node )->layer() )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();


      if ( qobject_cast<QgsVectorLayer *>( layer ) || qobject_cast<QgsRasterLayer *>( layer ) || qobject_cast<KadasPluginLayer *>( layer ) )
      {
        menu->addAction( actionLayerTransparency( menu ) );
      }
      if ( qobject_cast<QgsVectorLayer *>( layer ) || qobject_cast<QgsRasterLayer *>( layer ) )
      {
        menu->addAction( actionLayerRefreshRate( menu ) );
      }

      if ( layer->type() == QgsMapLayerType::PluginLayer )
      {
        QgsPluginLayer *pluginLayer = static_cast<QgsPluginLayer *>( layer );
        KadasPluginLayerType *plt = dynamic_cast<KadasPluginLayerType *>( QgsApplication::pluginLayerRegistry()->pluginLayerType( pluginLayer->pluginLayerType() ) );
        if ( plt )
        {
          plt->addLayerTreeMenuActions( menu, pluginLayer );
        }
      }
      menu->addAction( actions->actionZoomToLayers( kApp->mainWindow()->mapCanvas(), menu ) );
      QAction *renameAction = actions->actionRenameGroupOrLayer( menu );
      renameAction->setIcon( QIcon( ":/kadas/icons/rename" ) );
      menu->addAction( renameAction );
      menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), this, &KadasLayerTreeViewMenuProvider::removeLayerTreeItems );


      if ( layer->type() == QgsMapLayerType::RasterLayer && ( layer->providerType() == "gdal" || layer->providerType() == "wcs" ) )
      {
        menu->addAction( actionLayerUseAsHeightmap( menu ) );
      }
      else if ( layer->type() == QgsMapLayerType::VectorLayer )
      {
        menu->addAction( QgsApplication::getThemeIcon( "/mActionOpenTable.svg" ), tr( "&Open Attribute Table" ),
                         this, &KadasLayerTreeViewMenuProvider::showLayerAttributeTable );
      }
      if ( !layer->metadataUrl().isEmpty() )
      {
        menu->addAction( QIcon( ":/kadas/icons/info" ), tr( "Show layer info" ), this, &KadasLayerTreeViewMenuProvider::showLayerInfo );
      }
      if ( qobject_cast<KadasPluginLayer *>( layer ) || layer->type() == QgsMapLayerType::RasterLayer || layer->type() == QgsMapLayerType::VectorLayer || layer->type() == QgsMapLayerType::VectorTileLayer )
      {
        menu->addAction( QgsApplication::getThemeIcon( "/mIconProperties.svg" ), tr( "&Properties" ), this, &KadasLayerTreeViewMenuProvider::showLayerProperties );
      }
    }
  }
  else
  {
    menu->addAction( actions->actionGroupSelected( menu ) );
    menu->addAction( actions->actionZoomToLayers( kApp->mainWindow()->mapCanvas(), menu ) );
    menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), this, &KadasLayerTreeViewMenuProvider::removeLayerTreeItems );
  }

  return menu;
}

QAction *KadasLayerTreeViewMenuProvider::actionLayerTransparency( QMenu *parent )
{
  QgsMapLayer *layer = mView->currentLayer();
  if ( !layer )
  {
    return nullptr;
  }

  int opacity = 0;
  if ( qobject_cast<QgsVectorLayer *>( layer ) )
  {
    opacity = static_cast<QgsVectorLayer *>( layer )->opacity() * 100;
  }
  else if ( qobject_cast<KadasPluginLayer *>( layer ) )
  {
    opacity = static_cast<KadasPluginLayer *>( layer )->opacity() * 100;
  }
  else if ( qobject_cast<QgsRasterLayer *>( layer ) && static_cast<QgsRasterLayer *>( layer )->renderer() )
  {
    opacity = static_cast<QgsRasterLayer *>( layer )->renderer()->opacity() * 100;
  }

  QWidget *transpWidget = new QWidget();
  QHBoxLayout *transpLayout = new QHBoxLayout( transpWidget );

  QLabel *transpLabel = new QLabel( tr( "Transparency:" ) );
  transpLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
  transpLayout->addWidget( transpLabel );

  QSlider *transpSlider = new QSlider( Qt::Horizontal );
  transpSlider->setRange( 0, 100 );
  transpSlider->setValue( 100 - opacity );
  transpSlider->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  transpSlider->setTracking( false );
  connect( transpSlider, &QSlider::valueChanged, this, &KadasLayerTreeViewMenuProvider::setLayerTransparency );
  transpLayout->addWidget( transpSlider );

  QWidgetAction *transpAction = new QWidgetAction( parent );
  transpAction->setDefaultWidget( transpWidget );
  return transpAction;
}

QAction *KadasLayerTreeViewMenuProvider::actionLayerRefreshRate( QMenu *parent )
{
  QWidget *refreshRateWidget = new QWidget();
  QHBoxLayout *refreshRateLayout = new QHBoxLayout( refreshRateWidget );

  QLabel *refreshRateLabel = new QLabel( tr( "Data refresh rate:" ) );
  refreshRateLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
  refreshRateLayout->addWidget( refreshRateLabel );

  QSpinBox *refreshRateSpin = new QSpinBox( );
  refreshRateSpin->setRange( 0, 1000000 );
  refreshRateSpin->setSuffix( " s" );
  refreshRateSpin->setValue( kApp->layerRefreshManager()->layerRefreshInterval( mView->currentLayer()->id() ) );
  refreshRateSpin->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  connect( refreshRateSpin, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasLayerTreeViewMenuProvider::setLayerRefreshRate );
  refreshRateLayout->addWidget( refreshRateSpin );

  QWidgetAction *transpAction = new QWidgetAction( parent );
  transpAction->setDefaultWidget( refreshRateWidget );
  return transpAction;
}

QAction *KadasLayerTreeViewMenuProvider::actionLayerUseAsHeightmap( QMenu *parent )
{
  QgsMapLayer *layer = mView->currentLayer();
  if ( !layer )
  {
    return nullptr;
  }

  QAction *heightmapAction = new QAction( tr( "Use as heightmap" ), parent );
  heightmapAction->setCheckable( true );
  QString currentHeightmap = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  heightmapAction->setChecked( currentHeightmap == layer->id() );
  connect( heightmapAction, &QAction::toggled, this, &KadasLayerTreeViewMenuProvider::setLayerUseAsHeightmap );
  return heightmapAction;
}

void KadasLayerTreeViewMenuProvider::removeLayerTreeItems()
{
  // look for layers recursively so we catch also those that are within selected groups
  const QList<QgsMapLayer *> selectedLayers = mView->selectedLayersRecursive();
  const QList<QgsLayerTreeNode *> selectedNodes = mView->selectedNodes( true );

  if ( selectedNodes.isEmpty() )
  {
    return;
  }

  for ( QgsLayerTreeNode *node : selectedNodes )
  {
    QgsLayerTreeGroup *parentGroup = qobject_cast<QgsLayerTreeGroup *>( node->parent() );
    if ( parentGroup )
      parentGroup->removeChildNode( node );
  }
}

void KadasLayerTreeViewMenuProvider::setLayerTransparency( int value )
{
  QgsMapLayer *layer = mView->currentLayer();
  if ( !layer )
  {
    return;
  }

  if ( qobject_cast<QgsVectorLayer *>( layer ) )
  {
    static_cast<QgsVectorLayer *>( layer )->setOpacity( ( 100 - value ) / 100. );
  }
  else if ( qobject_cast<KadasPluginLayer *>( layer ) )
  {
    static_cast<KadasPluginLayer *>( layer )->setOpacity( ( 100 - value ) / 100. );
  }
  else if ( qobject_cast<QgsRasterLayer *>( layer ) && static_cast<QgsRasterLayer *>( layer )->renderer() )
  {
    static_cast<QgsRasterLayer *>( layer )->renderer()->setOpacity( 1. - value / 100. );
  }
  mView->refreshLayerSymbology( layer->id() );
  layer->triggerRepaint();
}

void KadasLayerTreeViewMenuProvider::setLayerRefreshRate( int value )
{
  kApp->layerRefreshManager()->setLayerRefreshInterval( mView->currentLayer()->id(), value );
}

void KadasLayerTreeViewMenuProvider::setLayerUseAsHeightmap( bool enabled )
{
  QgsMapLayer *layer = mView->currentLayer();
  if ( layer )
  {
    QgsProject::instance()->writeEntry( "Heightmap", "layer", enabled ? layer->id() : "" );
  }
}

void KadasLayerTreeViewMenuProvider::showLayerAttributeTable()
{
  kApp->showLayerAttributeTable( mView->currentLayer() );
}

void KadasLayerTreeViewMenuProvider::showLayerInfo()
{
  kApp->showLayerInfo( mView->currentLayer() );
}

void KadasLayerTreeViewMenuProvider::showLayerProperties()
{
  kApp->showLayerProperties( mView->currentLayer() );
}
