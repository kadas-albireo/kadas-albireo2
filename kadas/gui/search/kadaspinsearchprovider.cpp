/***************************************************************************
    kadaspinsearchprovider.cpp
    --------------------------
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

#include <QGraphicsItem>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/search/kadaspinsearchprovider.h>

const QString KadasPinSearchProvider::sCategoryName = KadasPinSearchProvider::tr ( "Pins" );

void KadasPinSearchProvider::startSearch ( const QString& searchtext, const SearchRegion& /*searchRegion*/ )
{
  for ( QgsMapLayer* layer : mMapCanvas->layers() ) {
    KadasItemLayer* itemLayer = dynamic_cast<KadasItemLayer*> ( layer );
    if ( !itemLayer ) {
      return;
    }
    for ( KadasMapItem* item : itemLayer->items() ) {
      const KadasSymbolItem* symbolItem = dynamic_cast<KadasSymbolItem*> ( item );
      if ( !symbolItem ) {
        continue;
      }
      if ( symbolItem->name().contains ( searchtext, Qt::CaseInsensitive ) ||
           symbolItem->remarks().contains ( searchtext, Qt::CaseInsensitive ) ) {
        SearchResult searchResult;
        searchResult.zoomScale = 1000;
        searchResult.category = sCategoryName;
        searchResult.categoryPrecedence = 2;
        searchResult.text = tr ( "Pin %1" ).arg ( symbolItem->name() );
        searchResult.pos = symbolItem->constState()->pos;
        searchResult.crs = symbolItem->crs().authid();
        searchResult.showPin = false;
        emit searchResultFound ( searchResult );
      }
    }
  }
  emit searchFinished();
}
