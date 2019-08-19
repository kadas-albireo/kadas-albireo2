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

#include <kadas/core/mapitems/kadasmapitem.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>

KadasMapCanvasItemManager* KadasMapCanvasItemManager::instance()
{
  static KadasMapCanvasItemManager manager;
  return &manager;
}

void KadasMapCanvasItemManager::addItem ( const KadasMapItem* item )
{
  instance()->mMapItems.append ( item );
  connect ( item, &KadasMapItem::aboutToBeDestroyed, instance(), &KadasMapCanvasItemManager::itemAboutToBeDestroyed );
  emit instance()->itemAdded ( item );
}

void KadasMapCanvasItemManager::removeItem ( const KadasMapItem* item )
{
  emit instance()->itemWillBeRemoved ( item );
  instance()->mMapItems.removeAll ( item );
}

const QList<const KadasMapItem*>& KadasMapCanvasItemManager::items()
{
  return instance()->mMapItems;
}

void KadasMapCanvasItemManager::itemAboutToBeDestroyed()
{
  const KadasMapItem* item = qobject_cast<const KadasMapItem*> ( QObject::sender() );
  if ( item ) {
    removeItem ( item );
  }
}
