/***************************************************************************
    kadastextitem.cpp
    -----------------
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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadastextitem.h>


KadasTextItem::KadasTextItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : KadasAnchoredItem( crs, parent )
{
  clear();
}

void KadasTextItem::setText( const QString &text )
{
  mText = text;
  QFontMetrics metrics( mFont );
  state()->size.setWidth( metrics.width( mText ) );
  state()->size.setHeight( metrics.height() );
  emit changed();
}

void KadasTextItem::setFillColor( const QColor &c )
{
  mFillColor = c;
  emit changed();
}

void KadasTextItem::setOutlineColor( const QColor &c )
{
  mOutlineColor = c;
  emit changed();
}

void KadasTextItem::setFont( const QFont &font )
{
  mFont = font;
  QFontMetrics metrics( mFont );
  state()->size.setWidth( metrics.width( mText ) );
  state()->size.setHeight( metrics.height() );
  emit changed();
}

void KadasTextItem::render( QgsRenderContext &context ) const
{
  QgsPointXY mapPos = context.coordinateTransform().transform( constState()->pos );
  QPointF pos = context.mapToPixel().transform( mapPos ).toQPointF();
  QFontMetrics metrics( mFont );
  QRect bbox = metrics.boundingRect( mText );
  int baselineOffset = metrics.ascent() - 0.5 * metrics.height();

  context.painter()->setBrush( QBrush( mFillColor ) );
  context.painter()->setPen( QPen( mOutlineColor, mFont.pointSizeF() / 15., Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin ) );
  context.painter()->setFont( mFont );
  QPainterPath path;
  path.addText( -0.5 * bbox.width(), baselineOffset, mFont, mText );
  context.painter()->translate( pos );
  context.painter()->rotate( -constState()->angle );
  context.painter()->drawPath( path );
}
