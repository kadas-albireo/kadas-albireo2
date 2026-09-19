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

#include <memory>

#include <QDomDocument>
#include <QDomElement>
#include <QTextDocument>
#include <QTextDocumentFragment>

#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationshadow.h"
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
  return QStringLiteral( ":/kadas/icons/pin_red" );
}

void KadasPinAnnotationItem::installDefaultSymbol()
{
  auto *layer = new QgsSvgMarkerSymbolLayer( defaultIconPath(), 12.0 );
  layer->setFillColor( QColor( 255, 0, 0 ) );
  layer->setVerticalAnchorPoint( Qgis::VerticalAnchorPoint::Bottom );
  layer->setHorizontalAnchorPoint( Qgis::HorizontalAnchorPoint::Center );
  setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << layer ) );
}

void KadasPinAnnotationItem::makeFillOpaque()
{
  const QgsMarkerSymbol *sym = symbol();
  if ( !sym )
    return;
  std::unique_ptr<QgsMarkerSymbol> opaque;
  for ( int i = 0; i < sym->symbolLayerCount(); ++i )
  {
    const auto *svg = dynamic_cast<const QgsSvgMarkerSymbolLayer *>( sym->symbolLayer( i ) );
    if ( !svg || svg->fillColor().alpha() == 255 )
      continue;
    if ( !opaque )
      opaque.reset( sym->clone() );
    auto *layer = static_cast<QgsSvgMarkerSymbolLayer *>( opaque->symbolLayer( i ) );
    QColor fill = layer->fillColor();
    fill.setAlpha( 255 );
    layer->setFillColor( fill );
  }
  if ( opaque )
    setSymbol( opaque.release() );
}

bool KadasPinAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QgsAnnotationMarkerItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "kadasName" ), mName );
  element.setAttribute( QStringLiteral( "kadasRemarks" ), mRemarks );
  mShadow.writeXml( element );
  return true;
}

bool KadasPinAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationMarkerItem::readXml( element, context );
  makeFillOpaque();
  mName = element.attribute( QStringLiteral( "kadasName" ) );
  mRemarks = element.attribute( QStringLiteral( "kadasRemarks" ) );
  mShadow.readXml( element );
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

QString KadasPinAnnotationItem::remarksAsHtml( const QString &remarks )
{
  if ( remarks.isEmpty() )
    return QString();
  // Rich text brings its own anchors, images and block structure; the editor has
  // already linked any bare URLs in it. Plain text — a description set from
  // outside the editor — still needs escaping and a block of its own.
  if ( Qt::mightBeRichText( remarks ) )
    return remarks;
  return QStringLiteral( "<p>%1</p>" ).arg( remarks.toHtmlEscaped().replace( QLatin1Char( '\n' ), QStringLiteral( "<br>" ) ) );
}

QString KadasPinAnnotationItem::remarksAsPlainText( const QString &remarks )
{
  if ( remarks.isEmpty() )
    return QString();
  // Searching the markup both misses words split by formatting and hits tag names.
  return Qt::mightBeRichText( remarks ) ? QTextDocumentFragment::fromHtml( remarks ).toPlainText() : remarks;
}
