/***************************************************************************
    kadasgpxrouteannotationitem.cpp
    -------------------------------
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
#include <QPainter>
#include <QPainterPath>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgscurve.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/annotationitems/kadasgpxrouteannotationitem.h"


KadasGpxRouteAnnotationItem::KadasGpxRouteAnnotationItem( QgsCurve *curve )
  : QgsAnnotationLineItem( curve )
{
  mLabelFont.setPointSize( 10 );
  mLabelFont.setBold( true );
  installDefaultSymbol();
}

KadasGpxRouteAnnotationItem::KadasGpxRouteAnnotationItem()
  : KadasGpxRouteAnnotationItem( new QgsLineString() )
{}

QString KadasGpxRouteAnnotationItem::type() const
{
  return itemTypeId();
}

void KadasGpxRouteAnnotationItem::installDefaultSymbol()
{
  // Legacy KadasGpxRouteItem default: 2px yellow solid line.
  auto *layer = new QgsSimpleLineSymbolLayer( Qt::yellow, 0.6 );
  setSymbol( new QgsLineSymbol( QgsSymbolLayerList() << layer ) );
}

void KadasGpxRouteAnnotationItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  QgsAnnotationLineItem::render( context, feedback );

  if ( mName.isEmpty() )
    return;
  const QgsCurve *curve = geometry();
  if ( !curve || curve->numPoints() < 2 )
    return;

  QFont font = mLabelFont;
  font.setPointSizeF( font.pointSizeF() * context.scaleFactor() );
  QFontMetrics metrics( font );
  const QSize labelSize = metrics.size( 0, mName );

  QPainterPath path;
  path.addText( QPointF( 0, 0 ), font, mName );

  const double interval = labelSize.width() + 150;
  double walkDist = 0;

  QColor labelColor = mLabelColor.isValid() ? mLabelColor : ( symbol() ? symbol()->color() : QColor( Qt::yellow ) );
  const QColor strokeColor = symbol() ? symbol()->color() : QColor( Qt::yellow );
  const QColor bufferColor = ( 0.2126 * strokeColor.red() + 0.7152 * strokeColor.green() + 0.0722 * strokeColor.blue() ) > 128 ? Qt::black : Qt::white;

  // Project curve vertices through the layer's coordinate transform to screen.
  QVector<QPointF> screenPoints;
  screenPoints.reserve( curve->numPoints() );
  for ( int i = 0, n = curve->numPoints(); i < n; ++i )
  {
    QgsPointXY pt( curve->xAt( i ), curve->yAt( i ) );
    try
    {
      pt = context.coordinateTransform().transform( pt );
    }
    catch ( const QgsCsException & )
    {
      return;
    }
    screenPoints << context.mapToPixel().transform( pt ).toQPointF();
  }

  QPainter *painter = context.painter();
  for ( int i = 0, n = screenPoints.size(); i < n - 1; ++i )
  {
    const QPointF &p1 = screenPoints[i];
    const QPointF &p2 = screenPoints[i + 1];
    const double dx = p2.x() - p1.x();
    const double dy = p2.y() - p1.y();
    const double dist = std::sqrt( dx * dx + dy * dy );
    if ( dist <= 0 )
      continue;
    while ( walkDist < dist )
    {
      QPointF dir( dx / dist, dy / dist );
      double angle = std::atan2( dir.y(), dir.x() ) / M_PI * 180.;
      while ( angle < 0 )
        angle += 360.;
      QPointF nor( dir.y(), -dir.x() );
      if ( angle > 90 && angle < 270 )
      {
        angle -= 180.;
        nor *= -1;
      }
      const QPointF pos = p1 + walkDist * dir + 0.25 * labelSize.height() * nor;

      painter->save();
      painter->translate( pos );
      painter->rotate( angle );
      painter->setBrush( QBrush( strokeColor ) );
      painter->setPen( QPen( bufferColor, 2 ) );
      painter->drawPath( path );
      painter->setBrush( QBrush( labelColor ) );
      painter->setPen( Qt::NoPen );
      painter->drawPath( path );
      painter->restore();

      walkDist += interval;
    }
    walkDist -= dist;
  }
}

bool KadasGpxRouteAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QgsAnnotationLineItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "kadasName" ), mName );
  element.setAttribute( QStringLiteral( "kadasNumber" ), mNumber );
  element.setAttribute( QStringLiteral( "kadasLabelFont" ), mLabelFont.toString() );
  element.setAttribute( QStringLiteral( "kadasLabelColor" ), mLabelColor.isValid() ? QgsSymbolLayerUtils::encodeColor( mLabelColor ) : QString() );
  return true;
}

bool KadasGpxRouteAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationLineItem::readXml( element, context );
  mName = element.attribute( QStringLiteral( "kadasName" ) );
  mNumber = element.attribute( QStringLiteral( "kadasNumber" ) );
  const QString fontStr = element.attribute( QStringLiteral( "kadasLabelFont" ) );
  if ( !fontStr.isEmpty() )
    mLabelFont.fromString( fontStr );
  const QString colorStr = element.attribute( QStringLiteral( "kadasLabelColor" ) );
  mLabelColor = colorStr.isEmpty() ? QColor() : QgsSymbolLayerUtils::decodeColor( colorStr );
  return true;
}

KadasGpxRouteAnnotationItem *KadasGpxRouteAnnotationItem::clone() const
{
  QgsCurve *curveClone = geometry() ? geometry()->clone() : new QgsLineString();
  auto *item = new KadasGpxRouteAnnotationItem( curveClone );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->setName( mName );
  item->setNumber( mNumber );
  item->setLabelFont( mLabelFont );
  item->setLabelColor( mLabelColor );
  item->copyCommonProperties( this );
  return item;
}

KadasGpxRouteAnnotationItem *KadasGpxRouteAnnotationItem::create()
{
  return new KadasGpxRouteAnnotationItem();
}
