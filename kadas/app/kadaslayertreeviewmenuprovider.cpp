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

#include <QDesktopServices>
#include <QMenu>
#include <QWidgetAction>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreeviewdefaultactions.h>
#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasterrenderer.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadaspluginlayertype.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/app/kadasapplication.h>
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

  QModelIndex idx = mView->currentIndex();
  if ( !idx.isValid() )
  {
    menu->addAction( actions->actionAddGroup( menu ) );
  }
  else if ( QgsLayerTreeNode *node = mView->layerTreeModel()->index2node( idx ) )
  {
    if ( QgsLayerTree::isGroup( node ) )
    {
      menu->addAction( actions->actionRenameGroupOrLayer( menu ) );
      menu->addAction( actions->actionMutuallyExclusiveGroup( menu ) );
    }
    else if ( QgsLayerTree::isLayer( node ) && QgsLayerTree::toLayer( node )->layer() )
    {
      QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();

      if ( layer->type() == QgsMapLayerType::PluginLayer )
      {
        QgsPluginLayer *pluginLayer = static_cast<QgsPluginLayer *>( layer );
        KadasPluginLayerType *plt = dynamic_cast<KadasPluginLayerType *>( QgsApplication::pluginLayerRegistry()->pluginLayerType( pluginLayer->pluginLayerType() ) );
        if ( plt )
        {
          plt->addLayerTreeMenuActions( menu, pluginLayer );
        }
      }

      if ( qobject_cast<QgsVectorLayer *>( layer ) || qobject_cast<QgsRasterLayer *>( layer ) || qobject_cast<KadasItemLayer *>( layer ) )
      {
        menu->addAction( actionLayerTransparency( menu ) );
      }
      menu->addAction( actions->actionZoomToLayer( kApp->mainWindow()->mapCanvas(), menu ) );
      menu->addAction( actions->actionRenameGroupOrLayer( menu ) );
      menu->addAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), this, &KadasLayerTreeViewMenuProvider::removeLayer );


      if ( layer->type() == QgsMapLayerType::RasterLayer )
      {
        menu->addAction( actionLayerUseAsHeightmap( menu ) );
      }
      else if ( layer->type() == QgsMapLayerType::VectorLayer )
      {
        menu->addAction( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ), tr( "&Open Attribute Table" ),
                         this, &KadasLayerTreeViewMenuProvider::showLayerAttributeTable );
      }
      if ( !layer->metadataUrl().isEmpty() )
      {
        menu->addAction( QgsApplication::getThemeIcon( "/mActionInfo.png" ), tr( "Show layer info" ), this, &KadasLayerTreeViewMenuProvider::showLayerInfo );
      }
      if ( layer->type() == QgsMapLayerType::PluginLayer )
      {
        QgsPluginLayer *pluginLayer = static_cast<QgsPluginLayer *>( layer );
        QgsPluginLayerType *plt = QgsApplication::pluginLayerRegistry()->pluginLayerType( pluginLayer->pluginLayerType() );
        if ( plt && plt->showLayerProperties( pluginLayer ) )
        {
          menu->addAction( tr( "&Properties" ), this, &KadasLayerTreeViewMenuProvider::showLayerProperties );
        }
      }
      else
      {
        menu->addAction( tr( "&Properties" ), this, &KadasLayerTreeViewMenuProvider::showLayerProperties );
      }
    }
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
    opacity = static_cast<QgsVectorLayer *>( layer )->opacity();
  }
  else if ( qobject_cast<KadasItemLayer *>( layer ) )
  {
    opacity = static_cast<KadasItemLayer *>( layer )->opacity();
  }
  else if ( qobject_cast<QgsRasterLayer *>( layer ) )
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

void KadasLayerTreeViewMenuProvider::removeLayer()
{
  QgsMapLayer *layer = mView->currentLayer();
  QgsProject::instance()->removeMapLayer( layer );
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
    static_cast<QgsVectorLayer *>( layer )->setOpacity( 100 - value );
  }
  else if ( qobject_cast<KadasItemLayer *>( layer ) )
  {
    static_cast<KadasItemLayer *>( layer )->setOpacity( 100 - value );
  }
  else if ( qobject_cast<QgsRasterLayer *>( layer ) )
  {
    static_cast<QgsRasterLayer *>( layer )->renderer()->setOpacity( 1. - value / 100. );
  }
  mView->refreshLayerSymbology( layer->id() );
  layer->triggerRepaint();
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
//  mView->currentLayer();
  // TODO
}

void KadasLayerTreeViewMenuProvider::showLayerInfo()
{
  QgsMapLayer *layer = mView->currentLayer();
  if ( layer && !layer->metadataUrl().isEmpty() )
  {
    QDesktopServices::openUrl( layer->metadataUrl() );
  }
}

void KadasLayerTreeViewMenuProvider::showLayerProperties()
{
  kApp->showLayerProperties( mView->currentLayer() );
}
