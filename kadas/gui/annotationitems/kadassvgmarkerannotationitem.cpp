/***************************************************************************
    kadassvgmarkerannotationitem.cpp
    --------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadassvgmarkerannotationitem.h"


KadasSvgMarkerAnnotationItem::KadasSvgMarkerAnnotationItem( const QgsPoint &point )
  : QgsAnnotationMarkerItem( point )
{
  setZIndex( KadasAnnotationZIndex::Marker );
  installDefaultSymbol();
}

QString KadasSvgMarkerAnnotationItem::type() const
{
  return itemTypeId();
}

QString KadasSvgMarkerAnnotationItem::placeholderIconPath()
{
  return QStringLiteral( ":/kadas/icons/draw_svgmarker" );
}

void KadasSvgMarkerAnnotationItem::installDefaultSymbol()
{
  auto *layer = new QgsSvgMarkerSymbolLayer( placeholderIconPath(), 24.0 );
  layer->setFillColor( QColor( 255, 0, 0 ) );
  setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << layer ) );
}

KadasSvgMarkerAnnotationItem *KadasSvgMarkerAnnotationItem::clone() const
{
  auto *item = new KadasSvgMarkerAnnotationItem( QgsPoint( geometry().x(), geometry().y() ) );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->copyCommonProperties( this );
  return item;
}

KadasSvgMarkerAnnotationItem *KadasSvgMarkerAnnotationItem::create()
{
  return new KadasSvgMarkerAnnotationItem();
}
