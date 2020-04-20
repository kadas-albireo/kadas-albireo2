/***************************************************************************
    kadasmapcanvasitemmanager.cpp
    -----------------------------
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

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

KadasMapCanvasItemManager::KadasMapCanvasItemManager()
{
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasMapCanvasItemManager::readFromProject );
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasMapCanvasItemManager::writeToProject );
}

KadasMapCanvasItemManager *KadasMapCanvasItemManager::instance()
{
  static KadasMapCanvasItemManager manager;
  return &manager;
}

void KadasMapCanvasItemManager::addItem( KadasMapItem *item )
{
  instance()->mMapItems.append( item );
  connect( item, &KadasMapItem::aboutToBeDestroyed, instance(), &KadasMapCanvasItemManager::itemAboutToBeDestroyed );
  emit instance()->itemAdded( item );
}

void KadasMapCanvasItemManager::removeItem( KadasMapItem *item )
{
  emit instance()->itemWillBeRemoved( item );
  instance()->mMapItems.removeAll( item );
}

void KadasMapCanvasItemManager::removeItemAfterRefresh( KadasMapItem *item, QgsMapCanvas *canvas )
{
  if ( !canvas->mapSettings().hasValidSettings() || canvas->isFrozen() )
  {
    // Canvas does not refresh if settings are invalid...
    removeItem( item );
  }
  else
  {
    QObject *scope = new QObject;
    connect( canvas, &QgsMapCanvas::mapCanvasRefreshed, scope, [item, scope] { removeItem( item ); scope->deleteLater(); } );
  }
}

const QList<KadasMapItem *> &KadasMapCanvasItemManager::items()
{
  return instance()->mMapItems;
}

void KadasMapCanvasItemManager::clear()
{
  qDeleteAll( instance()->mMapItems );
  instance()->mMapItems.clear();
}

void KadasMapCanvasItemManager::itemAboutToBeDestroyed()
{
  KadasMapItem *item = qobject_cast<KadasMapItem *> ( QObject::sender() );
  if ( item )
  {
    removeItem( item );
  }
}

void KadasMapCanvasItemManager::readFromProject( const QDomDocument &doc )
{
  QDomElement itemsEl = doc.firstChildElement( "qgis" ).firstChildElement( "MapCanvasItems" );
  if ( !itemsEl.isNull() )
  {
    QDomNodeList items = itemsEl.elementsByTagName( "MapItem" );
    for ( int i = 0, n = items.size(); i < n; ++i )
    {
      KadasMapItem *item = KadasMapItem::fromXml( items.at( i ).toElement() );
      if ( item )
      {
        addItem( item );
      }
    }
  }
}

void KadasMapCanvasItemManager::writeToProject( QDomDocument &doc )
{
  QDomElement root = doc.firstChildElement( "qgis" );
  // Write all items associated to a layer
  QList<const KadasMapItem *> items;
  for ( const KadasMapItem *item : mMapItems )
  {
    if ( item->associatedLayer() )
    {
      items.append( item );
    }
  }
  if ( !items.isEmpty() )
  {
    QDomElement itemsEl = doc.createElement( "MapCanvasItems" );
    for ( const KadasMapItem *item : items )
    {
      itemsEl.appendChild( item->writeXml( doc ) );
    }
    root.appendChild( itemsEl );
  }
}
