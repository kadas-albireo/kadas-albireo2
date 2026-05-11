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
#include <QMenu>

#include <qgis/qgsapplication.h>

#include "kadas/gui/kadasclipboard.h"
#include "kadas/gui/kadasitemcontextmenuactions.h"
#include "kadas/gui/mapitems/kadasmapitem.h"


KadasItemContextMenuActions::KadasItemContextMenuActions( QgsMapCanvas *canvas, QMenu *menu, KadasMapItem *item, KadasItemLayer *layer, KadasItemLayer::ItemId layerItemId, QObject *parent )
  : QObject( parent )
  , mCanvas( canvas )
  , mItem( item )
  , mLayer( layer )
  , mLayerItemId( layerItemId )
{
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
