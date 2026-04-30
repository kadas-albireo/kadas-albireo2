/***************************************************************************
    kadasrectangleannotationitem.cpp
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

#include <cmath>

#include <qgis/qgis.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>

#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"


KadasRectangleAnnotationItem::KadasRectangleAnnotationItem( const QgsPointXY &center, const QSizeF &size, double angleDegrees )
  : QgsAnnotationPolygonItem( new QgsPolygon() )
  , mCenter( center )
  , mSize( size )
  , mAngle( angleDegrees )
{
  rebuildGeometry();
}

QString KadasRectangleAnnotationItem::type() const
{
  return itemTypeId();
}

QVector<QgsPointXY> KadasRectangleAnnotationItem::corners() const
{
  const double halfW = 0.5 * mSize.width();
  const double halfH = 0.5 * mSize.height();
  // Local-frame corners in CCW order: BL, BR, TR, TL.
  const double localX[4] = { -halfW, halfW, halfW, -halfW };
  const double localY[4] = { -halfH, -halfH, halfH, halfH };

  const double a = mAngle * M_PI / 180.0;
  const double cosA = std::cos( a );
  const double sinA = std::sin( a );

  QVector<QgsPointXY> out;
  out.reserve( 4 );
  for ( int i = 0; i < 4; ++i )
  {
    const double rx = cosA * localX[i] - sinA * localY[i];
    const double ry = sinA * localX[i] + cosA * localY[i];
    out.append( QgsPointXY( mCenter.x() + rx, mCenter.y() + ry ) );
  }
  return out;
}

QgsPointXY KadasRectangleAnnotationItem::rotationHandle() const
{
  const double halfH = 0.5 * mSize.height();
  // Local position: midpoint of top edge, pushed outward by 25% of the
  // height (so the handle sits clearly outside the rectangle).
  const double localX = 0.0;
  const double localY = halfH + 0.25 * std::abs( mSize.height() );

  const double a = mAngle * M_PI / 180.0;
  const double cosA = std::cos( a );
  const double sinA = std::sin( a );
  const double rx = cosA * localX - sinA * localY;
  const double ry = sinA * localX + cosA * localY;
  return QgsPointXY( mCenter.x() + rx, mCenter.y() + ry );
}

void KadasRectangleAnnotationItem::rebuildGeometry()
{
  const QVector<QgsPointXY> c = corners();
  auto *ring = new QgsLineString();
  for ( const QgsPointXY &p : c )
    ring->addVertex( QgsPoint( p.x(), p.y() ) );
  // Close the ring.
  ring->addVertex( QgsPoint( c.first().x(), c.first().y() ) );
  auto *poly = new QgsPolygon();
  poly->setExteriorRing( ring );
  setGeometry( poly );
}

void KadasRectangleAnnotationItem::setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees )
{
  mCenter = center;
  mSize = size;
  mAngle = angleDegrees;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setCenter( const QgsPointXY &center )
{
  mCenter = center;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setSize( const QSizeF &size )
{
  mSize = size;
  rebuildGeometry();
}

void KadasRectangleAnnotationItem::setAngle( double angleDegrees )
{
  mAngle = angleDegrees;
  rebuildGeometry();
}

bool KadasRectangleAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  // Lets the parent serialize the polygon geometry + fill symbol + common
  // properties; we just stamp the high-level rect parameters alongside.
  QgsAnnotationPolygonItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "cx" ), qgsDoubleToString( mCenter.x() ) );
  element.setAttribute( QStringLiteral( "cy" ), qgsDoubleToString( mCenter.y() ) );
  element.setAttribute( QStringLiteral( "w" ), qgsDoubleToString( mSize.width() ) );
  element.setAttribute( QStringLiteral( "h" ), qgsDoubleToString( mSize.height() ) );
  element.setAttribute( QStringLiteral( "angle" ), qgsDoubleToString( mAngle ) );
  return true;
}

bool KadasRectangleAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  // Read parent first (polygon geometry + symbol + common props), then override
  // with the canonical rect parameters (geometry will be rebuilt from them).
  QgsAnnotationPolygonItem::readXml( element, context );
  mCenter = QgsPointXY( element.attribute( QStringLiteral( "cx" ) ).toDouble(), element.attribute( QStringLiteral( "cy" ) ).toDouble() );
  mSize = QSizeF( element.attribute( QStringLiteral( "w" ) ).toDouble(), element.attribute( QStringLiteral( "h" ) ).toDouble() );
  mAngle = element.attribute( QStringLiteral( "angle" ) ).toDouble();
  rebuildGeometry();
  return true;
}

KadasRectangleAnnotationItem *KadasRectangleAnnotationItem::clone() const
{
  auto *item = new KadasRectangleAnnotationItem( mCenter, mSize, mAngle );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->copyCommonProperties( this );
  return item;
}

KadasRectangleAnnotationItem *KadasRectangleAnnotationItem::create()
{
  return new KadasRectangleAnnotationItem();
}
