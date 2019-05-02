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

//#include <kadas/gui/kadaspinannotationitem.h> // TODO

#include <kadas/gui/search/kadaspinsearchprovider.h>

const QString KadasPinSearchProvider::sCategoryName = KadasPinSearchProvider::tr( "Pins" );

void KadasPinSearchProvider::startSearch( const QString &searchtext, const SearchRegion &/*searchRegion*/ )
{
  // TODO
#if 0
  for ( QGraphicsItem* item : mMapCanvas->scene()->items() )
  {
    if ( dynamic_cast<QgsPinAnnotationItem*>( item ) )
    {
      QgsPinAnnotationItem* pin = static_cast<QgsPinAnnotationItem*>( item );
      if ( pin->getName().contains( searchtext, Qt::CaseInsensitive ) ||
           pin->getRemarks().contains( searchtext, Qt::CaseInsensitive ) )
      {
        SearchResult searchResult;
        searchResult.zoomScale = 1000;
        searchResult.category = sCategoryName;
        searchResult.categoryPrecedence = 2;
        searchResult.text = tr( "Pin %1" ).arg( pin->getName() );
        searchResult.pos = pin->mapGeoPos();
        searchResult.crs = pin->mapGeoPosCrs().authid();
        searchResult.showPin = false;
        emit searchResultFound( searchResult );
      }
    }
  }
#endif
  emit searchFinished();
}
