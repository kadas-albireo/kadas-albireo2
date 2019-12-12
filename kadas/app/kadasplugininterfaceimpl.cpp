/***************************************************************************
    kadasplugininterfaceimpl.cpp
    ----------------------------
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
#include <QFont>
#include <QMenu>
#include <QToolBar>

#include <qgis/qgsgui.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmaptool.h>
#include <qgis/qgsprintlayout.h>
#include <qgis/qgsshortcutsmanager.h>

#include <kadas/gui/kadasmapwidget.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmapwidgetmanager.h>
#include <kadas/app/kadasplugininterfaceimpl.h>

KadasPluginInterfaceImpl::KadasPluginInterfaceImpl( KadasApplication *app )
{
  connect( kApp, &KadasApplication::printLayoutAdded, this, &KadasPluginInterface::printLayoutAdded );
  connect( kApp, &KadasApplication::printLayoutWillBeRemoved, this, &KadasPluginInterface::printLayoutWillBeRemoved );
  // TODO
}

QgsPluginManagerInterface *KadasPluginInterfaceImpl::pluginManagerInterface()
{
  // TODO
  return nullptr;
}

QgsLayerTreeView *KadasPluginInterfaceImpl::layerTreeView()
{
  return kApp->mainWindow()->layerTreeView();
}

void KadasPluginInterfaceImpl::addCustomActionForLayerType( QAction *action, QString menu, QgsMapLayerType type, bool allLayers )
{
  // TODO
}

void KadasPluginInterfaceImpl::addCustomActionForLayer( QAction *action, QgsMapLayer *layer )
{
  // TODO
}

bool KadasPluginInterfaceImpl::removeCustomActionForLayerType( QAction *action )
{
  // TODO
  return false;
}

QList< QgsMapCanvas * > KadasPluginInterfaceImpl::mapCanvases()
{
  QList<QgsMapCanvas *> canvases;
  canvases.append( kApp->mainWindow()->mapCanvas() );
  for ( const KadasMapWidget *widget : kApp->mainWindow()->mapWidgetManager()->mapWidgets() )
  {
    canvases.append( widget->mapCanvas() );
  }
  return canvases;
}

QgsMapCanvas *KadasPluginInterfaceImpl::createNewMapCanvas( const QString &name )
{
  return kApp->mainWindow()->mapWidgetManager()->addMapWidget( name )->mapCanvas();
}

void KadasPluginInterfaceImpl::closeMapCanvas( const QString &name )
{
  return kApp->mainWindow()->mapWidgetManager()->removeMapWidget( name );
}

QSize KadasPluginInterfaceImpl::iconSize( bool dockedToolbar ) const
{
  return QSize();
}

QList<QgsMapLayer *> KadasPluginInterfaceImpl::editableLayers( bool modified ) const
{
  return QList<QgsMapLayer *>();
}

QgsMapLayer *KadasPluginInterfaceImpl::activeLayer()
{
  return kApp->mainWindow()->layerTreeView()->currentLayer();
}

QgsMapCanvas *KadasPluginInterfaceImpl::mapCanvas()
{
  return kApp->mainWindow()->mapCanvas();
}

QgsLayerTreeMapCanvasBridge *KadasPluginInterfaceImpl::layerTreeCanvasBridge()
{
  return kApp->mainWindow()->layerTreeMapCanvasBridge();
}

QWidget *KadasPluginInterfaceImpl::mainWindow()
{
  return kApp->mainWindow();
}

QgsMessageBar *KadasPluginInterfaceImpl::messageBar()
{
  return kApp->mainWindow()->messageBar();
}

QList<QgsLayoutDesignerInterface *> KadasPluginInterfaceImpl::openLayoutDesigners()
{
  // TODO
  return QList<QgsLayoutDesignerInterface *>();

}

QgsVectorLayerTools *KadasPluginInterfaceImpl::vectorLayerTools()
{
  // TODO
  return nullptr;
}

void KadasPluginInterfaceImpl::addPluginToMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( PLUGIN_MENU ), name )->addAction( action );
}

void KadasPluginInterfaceImpl::removePluginMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( PLUGIN_MENU ), name )->removeAction( action );
}

void KadasPluginInterfaceImpl::insertAddLayerAction( QAction *action )
{
  getClassicMenu( ADDLAYER_MENU )->addAction( action );
}

void KadasPluginInterfaceImpl::removeAddLayerAction( QAction *action )
{
  getClassicMenu( ADDLAYER_MENU )->removeAction( action );
}

void KadasPluginInterfaceImpl::addPluginToDatabaseMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( DATABASE_MENU ), name )->addAction( action );
}

void KadasPluginInterfaceImpl::removePluginDatabaseMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( DATABASE_MENU ), name )->removeAction( action );
}

void KadasPluginInterfaceImpl::addPluginToRasterMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( RASTER_MENU ), name )->addAction( action );
}

void KadasPluginInterfaceImpl::removePluginRasterMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( RASTER_MENU ), name )->removeAction( action );
}

void KadasPluginInterfaceImpl::addPluginToVectorMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( VECTOR_MENU ), name )->addAction( action );
}

void KadasPluginInterfaceImpl::removePluginVectorMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( VECTOR_MENU ), name )->removeAction( action );
}

void KadasPluginInterfaceImpl::addPluginToWebMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( WEB_MENU ), name )->addAction( action );
}

void KadasPluginInterfaceImpl::removePluginWebMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( WEB_MENU ), name )->removeAction( action );
}

int KadasPluginInterfaceImpl::messageTimeout()
{
  return kApp->mainWindow()->messageTimeout();
}

void KadasPluginInterfaceImpl::zoomFull()
{
  kApp->mainWindow()->zoomFull();
}

void KadasPluginInterfaceImpl::zoomToPrevious()
{
  kApp->mainWindow()->zoomPrev();
}

void KadasPluginInterfaceImpl::zoomToNext()
{
  kApp->mainWindow()->zoomNext();
}

void KadasPluginInterfaceImpl::zoomToActiveLayer()
{
  kApp->mainWindow()->zoomToLayerExtent();
}

QgsVectorLayer *KadasPluginInterfaceImpl::addVectorLayer( const QString &vectorLayerPath, const QString &baseName, const QString &providerKey )
{
  return kApp->addVectorLayer( vectorLayerPath, baseName, providerKey );
}

QgsRasterLayer *KadasPluginInterfaceImpl::addRasterLayer( const QString &rasterLayerPath, const QString &baseName )
{
  return kApp->addRasterLayer( rasterLayerPath, baseName, QString() );
}

QgsRasterLayer *KadasPluginInterfaceImpl::addRasterLayer( const QString &url, const QString &layerName, const QString &providerKey )
{
  return kApp->addRasterLayer( url, layerName, providerKey );
}

QgsMeshLayer *KadasPluginInterfaceImpl::addMeshLayer( const QString &url, const QString &baseName, const QString &providerKey )
{
  return nullptr;
}

bool KadasPluginInterfaceImpl::addProject( const QString &project )
{
  return kApp->projectOpen( project );
}

void KadasPluginInterfaceImpl::newProject( bool promptToSaveFlag )
{
  kApp->projectNew( promptToSaveFlag );
}

void KadasPluginInterfaceImpl::reloadConnections( )
{
  // TODO ?
}

bool KadasPluginInterfaceImpl::setActiveLayer( QgsMapLayer *layer )
{
  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  return true;
}

void KadasPluginInterfaceImpl::copySelectionToClipboard( QgsMapLayer * )
{
  // TODO
}

void KadasPluginInterfaceImpl::pasteFromClipboard( QgsMapLayer * )
{
  // TODO
}

void KadasPluginInterfaceImpl::openURL( const QString &url, bool useQgisDocDirectory )
{
  if ( useQgisDocDirectory )
  {
    QDesktopServices::openUrl( "file://" + QgsApplication::pkgDataPath() + "/doc/" + url );
  }
  else
  {
    QDesktopServices::openUrl( url );
  }
}

void KadasPluginInterfaceImpl::openMessageLog()
{
  kApp->showMessageLog();
}

void KadasPluginInterfaceImpl::showLayoutManager()
{
  // TODO
}

bool KadasPluginInterfaceImpl::saveProject()
{
  return kApp->projectSave();
}

QgsLayoutDesignerInterface *KadasPluginInterfaceImpl::openLayoutDesigner( QgsMasterLayoutInterface *layout )
{
  // TODO
  return nullptr;
}

void KadasPluginInterfaceImpl::registerCustomDropHandler( QgsCustomDropHandler *handler )
{
  kApp->mainWindow()->addCustomDropHandler( handler );
}

void KadasPluginInterfaceImpl::unregisterCustomDropHandler( QgsCustomDropHandler *handler )
{
  kApp->mainWindow()->removeCustomDropHandler( handler );
}

void KadasPluginInterfaceImpl::registerCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler )
{
  // TODO
}

void KadasPluginInterfaceImpl::unregisterCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler )
{
  // TODO
}

void KadasPluginInterfaceImpl::showOptionsDialog( QWidget *parent, const QString &currentPage )
{
  // TODO ?
}


QgsLayerTreeRegistryBridge::InsertionPoint KadasPluginInterfaceImpl::layerTreeInsertionPoint()
{
  // TODO ?
  return QgsLayerTreeRegistryBridge::InsertionPoint( nullptr, 0 );
}

void KadasPluginInterfaceImpl::showLayerProperties( QgsMapLayer *l )
{
  // TODO
}

QDialog *KadasPluginInterfaceImpl::showAttributeTable( QgsVectorLayer *l, const QString &filterExpression )
{
  // TODO
  return nullptr;
}

bool KadasPluginInterfaceImpl::openFeatureForm( QgsVectorLayer *l, QgsFeature &f, bool updateFeatureOnly, bool showModal )
{
  // TODO
  return false;
}

QgsAttributeDialog *KadasPluginInterfaceImpl::getFeatureForm( QgsVectorLayer *l, QgsFeature &f )
{
  // TODO
  return nullptr;
}

bool KadasPluginInterfaceImpl::registerMainWindowAction( QAction *action, const QString &defaultShortcut )
{
  return QgsGui::shortcutsManager()->registerAction( action, defaultShortcut );
}

bool KadasPluginInterfaceImpl::unregisterMainWindowAction( QAction *action )
{
  return QgsGui::shortcutsManager()->unregisterAction( action );
}

QToolBar *KadasPluginInterfaceImpl::dummyToolbar()
{
  static QToolBar toolbar;
  toolbar.hide();
  return &toolbar;
}

QMenu *KadasPluginInterfaceImpl::getClassicMenu( ActionClassicMenuLocation classicMenuLocation, const QString &customName )
{
  QString menuName;
  switch ( classicMenuLocation )
  {
    case NO_MENU:
      break;
    case PROJECT_MENU:
      menuName = tr( "Project" ); break;
    case EDIT_MENU:
      menuName = tr( "Edit" ); break;
    case VIEW_MENU:
      menuName = tr( "View" ); break;
    case LAYER_MENU:
      menuName = tr( "Layer" ); break;
    case NEWLAYER_MENU:
      menuName = tr( "New layer" ); break;
    case ADDLAYER_MENU:
      menuName = tr( "Add layer" ); break;
    case SETTINGS_MENU:
      menuName = tr( "Settings" ); break;
    case PLUGIN_MENU:
      return kApp->mainWindow()->pluginsMenu();
    case RASTER_MENU:
      menuName = tr( "Raster" ); break;
    case DATABASE_MENU:
      menuName = tr( "Database" ); break;
    case VECTOR_MENU:
      menuName = tr( "Vector" ); break;
    case WEB_MENU:
      menuName = tr( "Web" ); break;
    case WINDOW_MENU:
      menuName = tr( "Window" ); break;
    case HELP_MENU:
    case FIRST_RIGHT_STANDARD_MENU:
      menuName = tr( "Help" ); break;
    case CUSTOM_MENU:
      menuName = customName;
      break;
  };
  return getSubMenu( kApp->mainWindow()->pluginsMenu(), menuName );
}

QMenu *KadasPluginInterfaceImpl::getSubMenu( QMenu *menu, const QString &submenuName )
{
  for ( const QAction *action : menu->actions() )
  {
    if ( action->menu() && action->menu()->title() == submenuName )
    {
      return action->menu();
    }
  }
  QMenu *submenu = new QMenu( menu );
  submenu->setTitle( submenuName );
  menu->addAction( submenu->menuAction() );
  return submenu;
}

QWidget *KadasPluginInterfaceImpl::getRibbonTabWidget( ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
{
  QWidget *targetTabWidget = nullptr;
  switch ( ribbonTabLocation )
  {
    case NO_TAB:
      break;
    case MAPS_TAB:
      targetTabWidget = kApp->mainWindow()->mapsTab(); break;
    case VIEW_TAB:
      targetTabWidget = kApp->mainWindow()->viewTab(); break;
    case ANALYSIS_TAB:
      targetTabWidget = kApp->mainWindow()->analysisTab(); break;
    case DRAW_TAB:
      targetTabWidget = kApp->mainWindow()->drawTab(); break;
    case GPS_TAB:
      targetTabWidget = kApp->mainWindow()->gpsTab(); break;
    case MSS_TAB:
      targetTabWidget = kApp->mainWindow()->mssTab(); break;
    case SETTINGS_TAB:
      targetTabWidget = kApp->mainWindow()->settingsTab(); break;
    case HELP_TAB:
      targetTabWidget = kApp->mainWindow()->helpTab(); break;
    case CUSTOM_TAB:
      for ( int i = 0, n = kApp->mainWindow()->ribbonTabWidget()->count(); i < n; ++i )
      {
        if ( kApp->mainWindow()->ribbonTabWidget()->tabText( i ) == customName )
        {
          targetTabWidget = kApp->mainWindow()->ribbonTabWidget()->widget( i );
          break;
        }
      }
      if ( !targetTabWidget )
      {
        targetTabWidget = kApp->mainWindow()->addRibbonTab( customName );
      }
      break;
  }
  return targetTabWidget;
}

void KadasPluginInterfaceImpl::addAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName, QgsMapTool *associatedMapTool )
{
  if ( ribbonTabLocation != NO_TAB )
  {
    // Add action to ribbon tabs
    QWidget *targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      kApp->mainWindow()->addActionToTab( action, targetTabWidget );
    }
    if ( associatedMapTool )
    {
      associatedMapTool->setAction( action );
    }
  }
  else
  {
    // Add action to classic app menu
    QMenu *targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      targetMenu->addAction( action );
    }
  }
}

void KadasPluginInterfaceImpl::addActionMenu( const QString &text, const QIcon &icon, QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
{
  if ( ribbonTabLocation != NO_TAB )
  {
    // Add action to ribbon tabs
    QWidget *targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      kApp->mainWindow()->addMenuButtonToTab( text, icon, menu, targetTabWidget );
    }
  }
  else
  {
    // Add action to classic app menu
    QMenu *targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      QAction *menuAction = new QAction( icon, text );
      menuAction->setMenu( menu );
      targetMenu->addAction( menuAction );
    }
  }
}

void KadasPluginInterfaceImpl::removeAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName, QgsMapTool *associatedMapTool )
{
  if ( ribbonTabLocation != NO_TAB )
  {
    QWidget *targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      kApp->mainWindow()->removeActionFromTab( action, targetTabWidget );
    }
  }
  else
  {
    //remove action from classic app menu
    QMenu *targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      targetMenu->removeAction( action );
      delete action;
    }
  }
}

void KadasPluginInterfaceImpl::removeActionMenu( QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
{
  if ( ribbonTabLocation != NO_TAB )
  {
    // Add action to ribbon tabs
    QWidget *targetTabWidget = getRibbonTabWidget( ribbonTabLocation, customName );
    if ( targetTabWidget )
    {
      kApp->mainWindow()->removeMenuButtonFromTab( menu, targetTabWidget );
    }
  }
  else
  {
    // Add action to classic app menu
    QMenu *targetMenu = getClassicMenu( classicMenuLocation, customName );
    if ( targetMenu )
    {
      for ( QAction *action : targetMenu->actions() )
      {
        if ( action->menu() == menu )
        {
          targetMenu->removeAction( action );
          break;
        }
      }
    }
  }
}

QAction *KadasPluginInterfaceImpl::findAction( const QString &name )
{
  return kApp->mainWindow()->findChild<QAction *>( name );
}

QObject *KadasPluginInterfaceImpl::findObject( const QString &name )
{
  return kApp->mainWindow()->findChild<QObject *>( name );
}

QgsPrintLayout *KadasPluginInterfaceImpl::createNewPrintLayout( const QString &title )
{
  return kApp->createNewPrintLayout( title );
}

bool KadasPluginInterfaceImpl::deletePrintLayout( QgsPrintLayout *layout )
{
  return kApp->deletePrintLayout( layout );
}

QList<QgsPrintLayout *> KadasPluginInterfaceImpl::printLayouts() const
{
  return kApp->printLayouts();
}

void KadasPluginInterfaceImpl::showLayoutDesigner( QgsPrintLayout *layout )
{
  kApp->showLayoutDesigner( layout );
}
