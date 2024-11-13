/***************************************************************************
    kadasgpxwaypointitem.cpp
    ------------------------
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

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include <qgis/qgsrendercontext.h>
#include <qgis/qgstextrenderer.h>

#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"


KadasGpxWaypointItem::KadasGpxWaypointItem()
  : KadasPointItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), IconType::ICON_CIRCLE )
{
  setEditor( KadasMapItemEditor::GPX_WAYPOINT );
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );

  // Default style
  int size = 2;
  QColor color = Qt::yellow;

  setOutline( QPen( color, size ) );
  setFill( QBrush( color ) );

  setIconSize( 10 + 2 * size );
  setIconOutline( QPen( Qt::black, size / 2 ) );
  setIconFill( QBrush( color ) );
}

QString KadasGpxWaypointItem::exportName() const
{
  if ( name().isEmpty() )
    return itemName();

  return name();
}

void KadasGpxWaypointItem::setName( const QString &name )
{
  mName = name;
  QFontMetrics fm( mLabelFont );
  mLabelSize = fm.size( 0, name );
  update();
  emit propertyChanged();
}

void KadasGpxWaypointItem::setLabelFont(const QFont &labelFont)
{
  mLabelFont = labelFont;
  emit propertyChanged();
}

void KadasGpxWaypointItem::setLabelColor(const QColor &labelColor)
{
  mLabelColor = labelColor;
  emit propertyChanged();
}

KadasMapItem::Margin KadasGpxWaypointItem::margin() const
{
  Margin m = KadasPointItem::margin();
  if ( !mName.isEmpty() )
  {
    m.right = std::max( m.right, mLabelSize.width() + mIconSize / 2 + 1 );
    m.top = std::max( m.top, mLabelSize.height() + mIconSize / 2 + 1 );
  }
  return m;
}

void KadasGpxWaypointItem::render( QgsRenderContext &context ) const
{
  KadasPointItem::render( context );

  // Draw name label
  if ( !mName.isEmpty() && !constState()->points.isEmpty() )
  {
    QPointF pos = context.mapToPixel().transform( context.coordinateTransform().transform( constState()->points.front() ) ).toQPointF();
    pos += QPointF( 0.5 * mIconSize, -0.5 * mIconSize );
    QColor bufferColor = ( 0.2126 * mIconBrush.color().red() + 0.7152 * mIconBrush.color().green() + 0.0722 * mIconBrush.color().blue() ) > 128 ? Qt::black : Qt::white;

    QFont font = mLabelFont;
    font.setPointSizeF( font.pointSizeF() * outputDpiScale( context ) );
    QFontMetrics metrics( font );
    pos += QPointF( metrics.width( mName ) / 2.0, 0.0 );

    // no idea why this works, otherwise text scales up when edited
    // the rendex context is coming from KadasMapItem when edited while it comes from the QgsMapLayerRenderer otherwise
    double scale = 1.0;
    if ( context.painter()->device()->physicalDpiX() )
      scale = 1.0 / context.painter()->device()->physicalDpiX() * context.painter()->device()->logicalDpiX();

    QgsTextFormat format;
    format.setFont( font );
    format.setSize( font.pointSize() * scale );
    if( mLabelColor.isValid() )
      format.setColor( mLabelColor );
    else
      format.setColor( mIconBrush.color() );
    QgsTextBufferSettings bs;
    bs.setColor( bufferColor );
    bs.setSize( 1 );
    bs.setEnabled( true );
    format.setBuffer( bs );

    QgsTextRenderer::drawText( pos, 0.0, Qgis::TextHorizontalAlignment::Center, {mName}, context, format, false );

    // QPainterPath path;
    // path.addText( pos + offset, mLabelFont, mName );
    // context.painter()->setPen( QPen( bufferColor, 2 ) );
    // context.painter()->drawPath( path );
    // context.painter()->setPen( Qt::NoPen );
    // if( mLabelColor.isValid() )
    //   context.painter()->setBrush( QBrush( mLabelColor ) );
    // else
    //   context.painter()->setBrush( QBrush( mIconBrush ) );
    // context.painter()->drawPath( path );
  }
}
