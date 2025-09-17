/***************************************************************************
    kadasitemcontextmenuactions.cpp
    -------------------------------
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

#include <QClipboard>
#include <QIcon>
#include <QInputDialog>
#include <QMenu>

#include <qgis/qgsapplication.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include "kadas/core/kadascoordinateformat.h"
#include "kadas/gui/kadasclipboard.h"
#include "kadas/gui/kadasitemcontextmenuactions.h"
#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"


KadasItemContextMenuActions::KadasItemContextMenuActions( QgsMapCanvas *canvas, QMenu *menu, KadasMapItem *item, KadasItemLayer *layer, KadasItemLayer::ItemId layerItemId, QObject *parent )
  : QObject( parent ), mCanvas( canvas ), mItem( item ), mLayer( layer ), mLayerItemId( layerItemId )
{
  if ( dynamic_cast<KadasPinItem *>( mItem ) )
  {
    menu->addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy position" ), this, &KadasItemContextMenuActions::copyItemPosition );
    menu->addAction( QgsApplication::getThemeIcon( "/mIconPointLayer.svg" ), tr( "Convert to waypoint" ), this, &KadasItemContextMenuActions::createWaypointFromPin );
  }
  else if ( dynamic_cast<KadasPointItem *>( mItem ) )
  {
    menu->addAction( QIcon( ":/kadas/icons/pin_red" ), tr( "Convert to pin" ), this, &KadasItemContextMenuActions::createPinFromPoint );
  }
  else if ( dynamic_cast<KadasCircleItem *>( mItem ) )
  {
    menu->addAction( QIcon( ":/kadas/icons/polygon" ), tr( "Convert to polygon" ), this, &KadasItemContextMenuActions::createPolygonFromCircle );
  }
  menu->addAction( QgsApplication::getThemeIcon( "/mActionEditCut.svg" ), tr( "Cut" ), this, &KadasItemContextMenuActions::cutItem );
  menu->addAction( QgsApplication::getThemeIcon( "/mActionEditCopy.svg" ), tr( "Copy" ), this, &KadasItemContextMenuActions::copyItem );
  menu->addAction( QgsApplication::getThemeIcon( "/mActionDeleteSelected.svg" ), tr( "Delete" ), this, [this] { deleteItem(); } );
}

void KadasItemContextMenuActions::copyItem()
{
  KadasClipboard::instance()->setStoredMapItems( QList<KadasMapItem *>() << mItem );
}

void KadasItemContextMenuActions::cutItem()
{
  copyItem();
  deleteItem( true );
}

void KadasItemContextMenuActions::deleteItem( bool preventAttachmentCleanup )
{
  if ( mLayerItemId != KadasItemLayer::ITEM_ID_NULL )
  {
    KadasMapItem *item = mLayer->takeItem( mLayerItemId );
    if ( preventAttachmentCleanup )
    {
      item->preventAttachmentCleanup();
    }
    delete item;
  }
  else
  {
    if ( preventAttachmentCleanup )
    {
      mItem->preventAttachmentCleanup();
    }
    delete mItem;
  }
  mItem = nullptr;
  mLayer->triggerRepaint();
}

void KadasItemContextMenuActions::copyItemPosition()
{
  QgsCoordinateReferenceSystem mapCrs = mCanvas->mapSettings().destinationCrs();
  QgsCoordinateTransform crst( mItem->crs(), mapCrs, QgsProject::instance() );
  QgsPointXY mapPos = crst.transform( mItem->position() );

  QString posStr = KadasCoordinateFormat::instance()->getDisplayString( mapPos, mapCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mapPos.toString() ).arg( mapCrs.authid() );
  }
  QString text = QString( "%1\n%2" )
                   .arg( posStr )
                   .arg( KadasCoordinateFormat::instance()->getHeightAtPos( mapPos, mapCrs ) );
  QApplication::clipboard()->setText( text );
}

void KadasItemContextMenuActions::createWaypointFromPin()
{
  KadasPinItem *pin = dynamic_cast<KadasPinItem *>( mItem );
  if ( !pin )
  {
    return;
  }

  KadasGpxWaypointItem *waypoint = new KadasGpxWaypointItem();
  waypoint->setName( pin->name() );
  QgsCoordinateTransform crst( pin->crs(), waypoint->crs(), QgsProject::instance()->transformContext() );
  waypoint->setPosition( KadasItemPos::fromPoint( crst.transform( pin->position() ) ) );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::RoutesLayer )->addItem( waypoint );

  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::RoutesLayer )->triggerRepaint();
  deleteItem();
}

void KadasItemContextMenuActions::createPinFromPoint()
{
  KadasPointItem *pointItem = dynamic_cast<KadasPointItem *>( mItem );
  if ( !pointItem )
  {
    return;
  }

  KadasPinItem *pin = new KadasPinItem();
  pin->setCrs( QgsCoordinateReferenceSystem( "EPSG:3857" ) );
  pin->setEditor( "KadasSymbolAttributesEditor" );
  if ( dynamic_cast<KadasGpxWaypointItem *>( pointItem ) )
  {
    pin->setName( static_cast<KadasGpxWaypointItem *>( pointItem )->name() );
  }
  QgsCoordinateTransform crst( pointItem->crs(), pin->crs(), QgsProject::instance()->transformContext() );
  pin->setPosition( KadasItemPos::fromPoint( crst.transform( pointItem->position() ) ) );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::PinsLayer )->addItem( pin );

  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::PinsLayer )->triggerRepaint();
  deleteItem();
}

void KadasItemContextMenuActions::createPolygonFromCircle()
{
  KadasCircleItem *circleItem = dynamic_cast<KadasCircleItem *>( mItem );
  if ( !circleItem || circleItem->constState()->centers.isEmpty() )
  {
    return;
  }
  bool ok = false;
  int num = QInputDialog::getInt( mCanvas, tr( "Vertex Count" ), tr( "Number of polygon vertices:" ), 10, 3, 10000, 1, &ok );
  if ( !ok )
  {
    return;
  }
  KadasPolygonItem *polygonitem = new KadasPolygonItem();
  polygonitem->setCrs( circleItem->crs() );
  polygonitem->setEditor( "KadasRedliningItemEditor" );
  KadasItemPos pos = circleItem->constState()->centers.front();
  double r = std::sqrt( circleItem->constState()->ringpos.front().sqrDist( pos ) );

  QgsLineString *ring = new QgsLineString();
  for ( int i = 0; i < num; ++i )
  {
    ring->addVertex( QgsPoint( pos.x() + r * std::cos( ( 2. * i ) / num * M_PI ), pos.y() + r * std::sin( ( 2. * i ) / num * M_PI ) ) );
  }
  ring->addVertex( QgsPoint( pos.x() + r, pos.y() ) );
  QgsPolygon poly;
  poly.setExteriorRing( ring );

  polygonitem->addPartFromGeometry( poly );
  polygonitem->setOutline( circleItem->outline() );
  polygonitem->setFill( circleItem->fill() );

  mLayer->addItem( polygonitem );
  deleteItem();
}
