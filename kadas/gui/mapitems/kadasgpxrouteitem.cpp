/***************************************************************************
    kadasgpxrouteitem.cpp
    ---------------------
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

#include "kadas/gui/mapitems/kadasgpxrouteitem.h"


KadasGpxRouteItem::KadasGpxRouteItem( QObject *parent )
  : KadasLineItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), parent )
{
  setEditor( KadasMapItemEditor::GPX_ROUTE );
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );

  // Default style
  int outlineWidth = 2;
  QColor color = Qt::yellow;

  setOutline( QPen( color, outlineWidth ) );
  setFill( QBrush( color ) );
}

QString KadasGpxRouteItem::exportName() const
{
  if ( name().isEmpty() )
    return itemName();

  return name();
}

void KadasGpxRouteItem::setName( const QString &name )
{
  mName = name;
  QFontMetrics fm( mLabelFont );
  mLabelSize = fm.size( 0, name );
  update();
  // TODO !!! emit propertyChanged();
}

void KadasGpxRouteItem::setNumber( const QString &number )
{
  mNumber = number;
  update();
  // TODO !!! emit propertyChanged();
}

void KadasGpxRouteItem::setLabelFont( const QFont &labelFont )
{
  mLabelFont = labelFont;
  // TODO !!! emit propertyChanged();
}

void KadasGpxRouteItem::setLabelColor( const QColor &labelColor )
{
  mLabelColor = labelColor;
  // TODO !!! emit propertyChanged();
}

KadasMapItem::Margin KadasGpxRouteItem::margin() const
{
  Margin m = KadasLineItem::margin();
  if ( !mName.isEmpty() )
  {
    // See rendering routine below
    int maxLabelExtent = std::max( mLabelSize.height() / 4 + mLabelSize.width() + 1, mLabelSize.height() );
    m.right = std::max( m.right, maxLabelExtent );
    m.top = std::max( m.top, maxLabelExtent );
  }
  return m;
}

void KadasGpxRouteItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  KadasLineItem::render( context, feedback );

  // Draw name label at regular 5 * label width distance
  if ( !mName.isEmpty() && !constState()->points.isEmpty() && constState()->points.front().size() >= 2 )
  {
    const QList<KadasItemPos> points = constState()->points.front();

    QPainterPath path;
    path.addText( QPointF( 0, 0 ), mLabelFont, mName );

    double interval = mLabelSize.width() + 150;
    double walkDist = 0;

    QColor bufferColor = ( 0.2126 * mBrush.color().red() + 0.7152 * mBrush.color().green() + 0.0722 * mBrush.color().blue() ) > 128 ? Qt::black : Qt::white;

    for ( int i = 0, n = points.size(); i < n - 1; ++i )
    {
      QPointF p1 = context.mapToPixel().transform( context.coordinateTransform().transform( points[i] ) ).toQPointF();
      QPointF p2 = context.mapToPixel().transform( context.coordinateTransform().transform( points[i + 1] ) ).toQPointF();

      double dist = std::sqrt( ( p2.x() - p1.x() ) * ( p2.x() - p1.x() ) + ( p2.y() - p1.y() ) * ( p2.y() - p1.y() ) );
      while ( walkDist < dist )
      {
        QPointF dir( ( p2.x() - p1.x() ) / dist, ( p2.y() - p1.y() ) / dist );
        double angle = std::atan2( dir.y(), dir.x() ) / M_PI * 180.;
        while ( angle < 0 )
          angle += 360.;
        QPointF nor( dir.y(), -dir.x() );
        if ( angle > 90 && angle < 270 )
        {
          angle -= 180.;
          nor *= -1;
        }
        QPointF pos = p1 + walkDist * dir + 0.25 * mLabelSize.height() * nor;

        context.painter()->save();
        context.painter()->translate( pos );
        context.painter()->rotate( angle );
        context.painter()->setBrush( mBrush );
        context.painter()->setPen( QPen( bufferColor, 2 ) );
        context.painter()->drawPath( path );
        context.painter()->setBrush( QBrush( mLabelColor ) );
        context.painter()->setPen( Qt::NoPen );
        context.painter()->drawPath( path );
        context.painter()->restore();

        walkDist += interval;
      }
      walkDist -= dist;
    }
  }
}
