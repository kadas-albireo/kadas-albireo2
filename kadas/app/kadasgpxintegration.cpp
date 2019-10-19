/***************************************************************************
    kadasgpxintegration.cpp
    -----------------------
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

#include <QAction>
#include <QShortcut>

#include <qgis/qgscoordinatereferencesystem.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitemeditors/kadasgpxrouteeditor.h>
#include <kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h>
#include <kadas/gui/mapitems/kadasgpxrouteitem.h>
#include <kadas/gui/mapitems/kadasgpxwaypointitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasgpxintegration.h>
#include <kadas/app/kadasmainwindow.h>

KadasGpxIntegration::KadasGpxIntegration( QAction *actionWaypoint, QAction *actionRoute, KadasMainWindow *parent )
  : QObject( parent ), mActionWaypoint( actionWaypoint ), mActionRoute( actionRoute )
{

  KadasMapToolCreateItem::ItemFactory waypointFactory = [ = ]
  {
    KadasGpxWaypointItem *waypoint = new KadasGpxWaypointItem();
    waypoint->setEditorFactory( KadasGpxWaypointEditor::factory );
    return waypoint;
  };
  KadasMapToolCreateItem::ItemFactory routeFactory = [ = ]
  {
    KadasGpxRouteItem *route = new KadasGpxRouteItem();
    route->setEditorFactory( KadasGpxRouteEditor::factory );
    return route;
  };

  connect( mActionWaypoint, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, waypointFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_W ), parent ), &QShortcut::activated, mActionWaypoint, &QAction::trigger );

  connect( mActionRoute, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, routeFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_G, Qt::CTRL + Qt::Key_R ), parent ), &QShortcut::activated, mActionRoute, &QAction::trigger );
}

KadasItemLayer *KadasGpxIntegration::getOrCreateLayer()
{
  return kApp->getOrCreateItemLayer( tr( "GPS Routes" ) );
}

void KadasGpxIntegration::toggleCreateItem( bool active, const std::function<KadasMapItem*() > &itemFactory )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *> ( QObject::sender() );
  if ( active )
  {
    KadasMapToolCreateItem *tool = new KadasMapToolCreateItem( canvas, itemFactory, getOrCreateLayer() );
    tool->setAction( action );
    kApp->mainWindow()->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    kApp->mainWindow()->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    canvas->setMapTool( tool );
  }
  else if ( !active && canvas->mapTool() && canvas->mapTool()->action() == action )
  {
    canvas->unsetMapTool( canvas->mapTool() );
  }
}
