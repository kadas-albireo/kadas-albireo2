/***************************************************************************
    kadaspinannotationitem.cpp
    --------------------------
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

#include <QDomDocument>
#include <QDomElement>

#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"


KadasPinAnnotationItem::KadasPinAnnotationItem( const QgsPoint &point )
  : QgsAnnotationMarkerItem( point )
{
  setZIndex( KadasAnnotationZIndex::Pin );
  installDefaultSymbol();
}

QString KadasPinAnnotationItem::type() const
{
  return itemTypeId();
}

QString KadasPinAnnotationItem::defaultIconPath()
{
  return QStringLiteral( ":/kadas/icons/pin_red.svg" );
}

void KadasPinAnnotationItem::installDefaultSymbol()
{
  // Standard Kadas pin: SVG anchored at its bottom tip so the geographic
  // anchor matches the visible point of the pin. 24px is the default
  // visible size matching the legacy KadasPinItem icon.
  auto *layer = new QgsSvgMarkerSymbolLayer( defaultIconPath(), 24.0 );
  layer->setVerticalAnchorPoint( Qgis::VerticalAnchorPoint::Bottom );
  layer->setHorizontalAnchorPoint( Qgis::HorizontalAnchorPoint::Center );
  setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << layer ) );
}

bool KadasPinAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QgsAnnotationMarkerItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "kadasName" ), mName );
  element.setAttribute( QStringLiteral( "kadasRemarks" ), mRemarks );
  return true;
}

bool KadasPinAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationMarkerItem::readXml( element, context );
  mName = element.attribute( QStringLiteral( "kadasName" ) );
  mRemarks = element.attribute( QStringLiteral( "kadasRemarks" ) );
  return true;
}

KadasPinAnnotationItem *KadasPinAnnotationItem::clone() const
{
  auto *item = new KadasPinAnnotationItem( QgsPoint( geometry().x(), geometry().y() ) );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->setName( mName );
  item->setRemarks( mRemarks );
  item->copyCommonProperties( this );
  return item;
}

KadasPinAnnotationItem *KadasPinAnnotationItem::create()
{
  return new KadasPinAnnotationItem();
}
