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

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadasimageitem.h>

#include <kadas/gui/search/kadaspinsearchprovider.h>

const QString KadasPinSearchProvider::sCategoryName = KadasPinSearchProvider::tr( "Pins" );

void KadasPinSearchProvider::startSearch( const QString &searchtext, const SearchRegion &/*searchRegion*/ )
{
  for(QgsMapLayer* layer : mMapCanvas->layers()) {
    KadasItemLayer* itemLayer = dynamic_cast<KadasItemLayer*>(layer);
    if(!itemLayer) {
      return;
    }
    for(KadasMapItem* item : itemLayer->items()) {
      const KadasImageItem* imageItem = dynamic_cast<KadasImageItem*>(item);
      if(!imageItem) {
        continue;
      }
      if ( imageItem->name().contains( searchtext, Qt::CaseInsensitive ) ||
           imageItem->remarks().contains( searchtext, Qt::CaseInsensitive ) )
      {
        SearchResult searchResult;
        searchResult.zoomScale = 1000;
        searchResult.category = sCategoryName;
        searchResult.categoryPrecedence = 2;
        searchResult.text = tr( "Pin %1" ).arg( imageItem->name() );
        searchResult.pos = imageItem->state()->pos;
        searchResult.crs = imageItem->crs().authid();
        searchResult.showPin = false;
        emit searchResultFound( searchResult );
      }
    }
  }
  emit searchFinished();
}
