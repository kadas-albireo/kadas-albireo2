/***************************************************************************
    kadaspythoninterface.cpp
    ------------------------
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
#include <qgis/qgsshortcutsmanager.h>

#include <kadas/gui/kadasmapwidget.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmapwidgetmanager.h>
#include <kadas/app/kadaspythoninterface.h>

KadasPythonInterface::KadasPythonInterface( KadasApplication *app )
{
  // TODO
}

QgsPluginManagerInterface *KadasPythonInterface::pluginManagerInterface()
{
  // TODO
  return nullptr;
}

QgsLayerTreeView *KadasPythonInterface::layerTreeView()
{
  return kApp->mainWindow()->layerTreeView();
}

void KadasPythonInterface::addCustomActionForLayerType( QAction *action, QString menu, QgsMapLayerType type, bool allLayers )
{
  // TODO
}

void KadasPythonInterface::addCustomActionForLayer( QAction *action, QgsMapLayer *layer )
{
  // TODO
}

bool KadasPythonInterface::removeCustomActionForLayerType( QAction *action )
{
  // TODO
  return false;
}

QList< QgsMapCanvas * > KadasPythonInterface::mapCanvases()
{
  QList<QgsMapCanvas *> canvases;
  canvases.append( kApp->mainWindow()->mapCanvas() );
  for ( const KadasMapWidget *widget : kApp->mainWindow()->mapWidgetManager()->mapWidgets() )
  {
    canvases.append( widget->mapCanvas() );
  }
  return canvases;
}

QgsMapCanvas *KadasPythonInterface::createNewMapCanvas( const QString &name )
{
  return kApp->mainWindow()->mapWidgetManager()->addMapWidget( name )->mapCanvas();
}

void KadasPythonInterface::closeMapCanvas( const QString &name )
{
  return kApp->mainWindow()->mapWidgetManager()->removeMapWidget( name );
}

QSize KadasPythonInterface::iconSize( bool dockedToolbar ) const
{
  return QSize();
}

QList<QgsMapLayer *> KadasPythonInterface::editableLayers( bool modified ) const
{
  return QList<QgsMapLayer *>();
}

QgsMapLayer *KadasPythonInterface::activeLayer()
{
  return kApp->mainWindow()->layerTreeView()->currentLayer();
}

QgsMapCanvas *KadasPythonInterface::mapCanvas()
{
  return kApp->mainWindow()->mapCanvas();
}

QgsLayerTreeMapCanvasBridge *KadasPythonInterface::layerTreeCanvasBridge()
{
  return kApp->mainWindow()->layerTreeMapCanvasBridge();
}

QWidget *KadasPythonInterface::mainWindow()
{
  return kApp->mainWindow();
}

QgsMessageBar *KadasPythonInterface::messageBar()
{
  return kApp->mainWindow()->messageBar();
}

QList<QgsLayoutDesignerInterface *> KadasPythonInterface::openLayoutDesigners()
{
  // TODO
  return QList<QgsLayoutDesignerInterface *>();

}

QgsVectorLayerTools *KadasPythonInterface::vectorLayerTools()
{
  // TODO
  return nullptr;
}

void KadasPythonInterface::addPluginToMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( PLUGIN_MENU ), name )->addAction( action );
}

void KadasPythonInterface::removePluginMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( PLUGIN_MENU ), name )->removeAction( action );
}

void KadasPythonInterface::insertAddLayerAction( QAction *action )
{
  getClassicMenu( ADDLAYER_MENU )->addAction( action );
}

void KadasPythonInterface::removeAddLayerAction( QAction *action )
{
  getClassicMenu( ADDLAYER_MENU )->removeAction( action );
}

void KadasPythonInterface::addPluginToDatabaseMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( DATABASE_MENU ), name )->addAction( action );
}

void KadasPythonInterface::removePluginDatabaseMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( DATABASE_MENU ), name )->removeAction( action );
}

void KadasPythonInterface::addPluginToRasterMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( RASTER_MENU ), name )->addAction( action );
}

void KadasPythonInterface::removePluginRasterMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( RASTER_MENU ), name )->removeAction( action );
}

void KadasPythonInterface::addPluginToVectorMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( VECTOR_MENU ), name )->addAction( action );
}

void KadasPythonInterface::removePluginVectorMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( VECTOR_MENU ), name )->removeAction( action );
}

void KadasPythonInterface::addPluginToWebMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( WEB_MENU ), name )->addAction( action );
}

void KadasPythonInterface::removePluginWebMenu( const QString &name, QAction *action )
{
  getSubMenu( getClassicMenu( WEB_MENU ), name )->removeAction( action );
}

int KadasPythonInterface::messageTimeout()
{
  return kApp->mainWindow()->messageTimeout();
}

void KadasPythonInterface::zoomFull()
{
  kApp->mainWindow()->zoomFull();
}

void KadasPythonInterface::zoomToPrevious()
{
  kApp->mainWindow()->zoomPrev();
}

void KadasPythonInterface::zoomToNext()
{
  kApp->mainWindow()->zoomNext();
}

void KadasPythonInterface::zoomToActiveLayer()
{
  kApp->mainWindow()->zoomToLayerExtent();
}

QgsVectorLayer *KadasPythonInterface::addVectorLayer( const QString &vectorLayerPath, const QString &baseName, const QString &providerKey )
{
  return kApp->addVectorLayer( vectorLayerPath, baseName, providerKey );
}

QgsRasterLayer *KadasPythonInterface::addRasterLayer( const QString &rasterLayerPath, const QString &baseName )
{
  return kApp->addRasterLayer( rasterLayerPath, baseName, QString() );
}

QgsRasterLayer *KadasPythonInterface::addRasterLayer( const QString &url, const QString &layerName, const QString &providerKey )
{
  return kApp->addRasterLayer( url, layerName, providerKey );
}

QgsMeshLayer *KadasPythonInterface::addMeshLayer( const QString &url, const QString &baseName, const QString &providerKey )
{
  return nullptr;
}

bool KadasPythonInterface::addProject( const QString &project )
{
  return kApp->projectOpen( project );
}

void KadasPythonInterface::newProject( bool promptToSaveFlag )
{
  kApp->projectNew( promptToSaveFlag );
}

void KadasPythonInterface::reloadConnections( )
{
  // TODO ?
}

bool KadasPythonInterface::setActiveLayer( QgsMapLayer *layer )
{
  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  return true;
}

void KadasPythonInterface::copySelectionToClipboard( QgsMapLayer * )
{
  // TODO
}

void KadasPythonInterface::pasteFromClipboard( QgsMapLayer * )
{
  // TODO
}

void KadasPythonInterface::openURL( const QString &url, bool useQgisDocDirectory )
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

void KadasPythonInterface::openMessageLog()
{
  // TODO
}

void KadasPythonInterface::showLayoutManager()
{
  // TODO
}

QgsLayoutDesignerInterface *KadasPythonInterface::openLayoutDesigner( QgsMasterLayoutInterface *layout )
{
  // TODO
  return nullptr;
}

void KadasPythonInterface::registerCustomDropHandler( QgsCustomDropHandler *handler )
{
  // TODO
}

void KadasPythonInterface::unregisterCustomDropHandler( QgsCustomDropHandler *handler )
{
  // TODO
}

void KadasPythonInterface::registerCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler )
{
  // TODO
}

void KadasPythonInterface::unregisterCustomLayoutDropHandler( QgsLayoutCustomDropHandler *handler )
{
  // TODO
}

void KadasPythonInterface::showOptionsDialog( QWidget *parent, const QString &currentPage )
{
  // TODO ?
}

void KadasPythonInterface::showLayerProperties( QgsMapLayer *l )
{
  // TODO
}

QDialog *KadasPythonInterface::showAttributeTable( QgsVectorLayer *l, const QString &filterExpression )
{
  // TODO
  return nullptr;
}

bool KadasPythonInterface::openFeatureForm( QgsVectorLayer *l, QgsFeature &f, bool updateFeatureOnly, bool showModal )
{
  // TODO
  return false;
}

QgsAttributeDialog *KadasPythonInterface::getFeatureForm( QgsVectorLayer *l, QgsFeature &f )
{
  // TODO
  return nullptr;
}

bool KadasPythonInterface::registerMainWindowAction( QAction *action, const QString &defaultShortcut )
{
  return QgsGui::shortcutsManager()->registerAction( action, defaultShortcut );
}

bool KadasPythonInterface::unregisterMainWindowAction( QAction *action )
{
  return QgsGui::shortcutsManager()->unregisterAction( action );
}

QToolBar *KadasPythonInterface::dummyToolbar()
{
  static QToolBar toolbar;
  toolbar.hide();
  return &toolbar;
}

QMenu *KadasPythonInterface::getClassicMenu( ActionClassicMenuLocation classicMenuLocation, const QString &customName )
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

QMenu *KadasPythonInterface::getSubMenu( QMenu *menu, const QString &submenuName )
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

QWidget *KadasPythonInterface::getRibbonTabWidget( ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
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

void KadasPythonInterface::addAction( QAction *action, ActionClassicMenuLocation classicMenuLocation, ActionClassicToolbarLocation classicToolbarLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName, QgsMapTool *associatedMapTool )
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

void KadasPythonInterface::addActionMenu( const QString &text, const QIcon &icon, QMenu *menu, ActionClassicMenuLocation classicMenuLocation, ActionClassicToolbarLocation classicToolbarLocation, ActionRibbonTabLocation ribbonTabLocation, const QString &customName )
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

QAction *KadasPythonInterface::findAction( const QString &name )
{
  return kApp->mainWindow()->findChild<QAction *>( name );
}

QObject *KadasPythonInterface::findObject( const QString &name )
{
  return kApp->mainWindow()->findChild<QObject *>( name );
}

