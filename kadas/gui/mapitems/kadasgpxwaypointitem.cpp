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

#include <kadas/gui/mapitems/kadasgpxwaypointitem.h>

KadasGpxWaypointItem::KadasGpxWaypointItem( QObject *parent )
  : KadasPointItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), ICON_CIRCLE, parent )
{
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );
}

void KadasGpxWaypointItem::setName( const QString &name )
{
  mName = name;
  QFontMetrics fm( mLabelFont );
  mLabelSize = fm.size( 0, name );
  update();
}

KadasMapItem::Margin KadasGpxWaypointItem::margin() const
{
  Margin m = KadasPointItem::margin();
  if ( !mName.isEmpty() )
  {
    m.right = qMax( m.right, mLabelSize.width() + mIconSize / 2 + 1 );
    m.top = qMax( m.top, mLabelSize.height() + mIconSize / 2 + 1 );
  }
  return m;
}

void KadasGpxWaypointItem::render( QgsRenderContext &context ) const
{
  KadasPointItem::render( context );

  // Draw name label
  if ( !mName.isEmpty() && !constState()->points.isEmpty() )
  {
    QColor bufferColor = ( 0.2126 * mIconBrush.color().red() + 0.7152 * mIconBrush.color().green() + 0.0722 * mIconBrush.color().blue() ) > 128 ? Qt::black : Qt::white;
    QPointF pos = context.mapToPixel().transform( context.coordinateTransform().transform( constState()->points.front() ) ).toQPointF();
    QPointF offset( 0.5 * mIconSize, -0.5 * mIconSize );
    QPainterPath path;
    path.addText( pos + offset, mLabelFont, mName );
    context.painter()->setPen( QPen( bufferColor, 2 ) );
    context.painter()->setBrush( mIconBrush );
    context.painter()->drawPath( path );
    context.painter()->setPen( Qt::transparent );
    context.painter()->drawPath( path );
  }
}
