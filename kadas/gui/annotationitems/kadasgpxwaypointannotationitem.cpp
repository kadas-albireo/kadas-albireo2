/***************************************************************************
    kadasgpxwaypointannotationitem.cpp
    ----------------------------------
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
#include <QFontMetrics>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgstextformat.h>
#include <qgis/qgstextrenderer.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasgpxwaypointannotationitem.h"


KadasGpxWaypointAnnotationItem::KadasGpxWaypointAnnotationItem( const QgsPoint &point )
  : QgsAnnotationMarkerItem( point )
{
  setZIndex( KadasAnnotationZIndex::GpxWaypoint );
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );
  installDefaultSymbol();
}

QString KadasGpxWaypointAnnotationItem::type() const
{
  return itemTypeId();
}

void KadasGpxWaypointAnnotationItem::installDefaultSymbol()
{
  // Legacy KadasGpxWaypointItem default: yellow circle, stroke == fill.
  auto *layer = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle );
  layer->setColor( Qt::yellow );
  layer->setStrokeColor( Qt::yellow );
  layer->setStrokeWidth( 0.4 );
  layer->setSize( 3 );
  setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << layer ) );
}

void KadasGpxWaypointAnnotationItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  QgsAnnotationMarkerItem::render( context, feedback );

  if ( mName.isEmpty() )
    return;

  // Project layer-CRS point to screen coordinates.
  QgsPointXY p = geometry();
  try
  {
    p = context.coordinateTransform().transform( p );
  }
  catch ( const QgsCsException & )
  {
    return;
  }
  const QPointF screenPos = context.mapToPixel().transform( QgsPointXY( p ) ).toQPointF();

  // Pick label color: configured override, otherwise the marker fill color.
  QColor labelColor = mLabelColor;
  if ( !labelColor.isValid() )
  {
    if ( const QgsMarkerSymbol *s = symbol() )
      labelColor = s->color();
    if ( !labelColor.isValid() )
      labelColor = Qt::yellow;
  }
  const QColor bufferColor = ( 0.2126 * labelColor.red() + 0.7152 * labelColor.green() + 0.0722 * labelColor.blue() ) > 128 ? Qt::black : Qt::white;

  // Offset the label up-and-right of the marker by roughly the marker size.
  const double markerSize = symbol() ? context.convertToPainterUnits( symbol()->size(), Qgis::RenderUnit::Millimeters ) : 10.0;
  QFont font = mLabelFont;
  font.setPointSizeF( font.pointSizeF() * context.scaleFactor() );
  QFontMetrics metrics( font );
  const QPointF labelPos = screenPos + QPointF( 0.5 * markerSize + metrics.horizontalAdvance( mName ) / 2.0, -0.5 * markerSize );

  QgsTextFormat format;
  format.setFont( font );
  format.setSize( font.pointSizeF() );
  format.setSizeUnit( Qgis::RenderUnit::Points );
  format.setColor( labelColor );
  QgsTextBufferSettings bs;
  bs.setColor( bufferColor );
  bs.setSize( 1 );
  bs.setEnabled( true );
  format.setBuffer( bs );

  QgsTextRenderer::drawText( labelPos, 0.0, Qgis::TextHorizontalAlignment::Center, { mName }, context, format, false );
}

bool KadasGpxWaypointAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QgsAnnotationMarkerItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "kadasName" ), mName );
  element.setAttribute( QStringLiteral( "kadasLabelFont" ), mLabelFont.toString() );
  element.setAttribute( QStringLiteral( "kadasLabelColor" ), mLabelColor.isValid() ? QgsSymbolLayerUtils::encodeColor( mLabelColor ) : QString() );
  return true;
}

bool KadasGpxWaypointAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationMarkerItem::readXml( element, context );
  mName = element.attribute( QStringLiteral( "kadasName" ) );
  const QString fontStr = element.attribute( QStringLiteral( "kadasLabelFont" ) );
  if ( !fontStr.isEmpty() )
    mLabelFont.fromString( fontStr );
  const QString colorStr = element.attribute( QStringLiteral( "kadasLabelColor" ) );
  mLabelColor = colorStr.isEmpty() ? QColor() : QgsSymbolLayerUtils::decodeColor( colorStr );
  return true;
}

KadasGpxWaypointAnnotationItem *KadasGpxWaypointAnnotationItem::clone() const
{
  auto *item = new KadasGpxWaypointAnnotationItem( QgsPoint( geometry().x(), geometry().y() ) );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->setName( mName );
  item->setLabelFont( mLabelFont );
  item->setLabelColor( mLabelColor );
  item->copyCommonProperties( this );
  return item;
}

KadasGpxWaypointAnnotationItem *KadasGpxWaypointAnnotationItem::create()
{
  return new KadasGpxWaypointAnnotationItem();
}
