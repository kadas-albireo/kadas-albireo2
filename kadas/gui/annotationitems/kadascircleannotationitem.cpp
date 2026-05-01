/***************************************************************************
    kadascircleannotationitem.cpp
    -----------------------------
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
#include <qgis/qgscircularstring.h>
#include <qgis/qgscompoundcurve.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"


KadasCircleAnnotationItem::KadasCircleAnnotationItem( const QgsPointXY &center, const QgsPointXY &ringPoint )
  : QgsAnnotationPolygonItem( new QgsPolygon() )
  , mCenter( center )
  , mRingPoint( ringPoint )
{
  setZIndex( KadasAnnotationZIndex::Circle );
  rebuildGeometry();
}

QString KadasCircleAnnotationItem::type() const
{
  return itemTypeId();
}

double KadasCircleAnnotationItem::radius() const
{
  const double dx = mRingPoint.x() - mCenter.x();
  const double dy = mRingPoint.y() - mCenter.y();
  return std::sqrt( dx * dx + dy * dy );
}

void KadasCircleAnnotationItem::rebuildGeometry()
{
  const double r = radius();
  if ( r <= 0 )
  {
    // Degenerate: keep a non-curved empty polygon to avoid render glitches.
    setGeometry( new QgsPolygon() );
    return;
  }

  // Build the circle as two circular-string arcs (top + bottom semicircles)
  // joined into a closed compound curve. The points are picked at fixed
  // angles relative to the +X axis: 0 (east), 90 (north), 180 (west),
  // 270 (south). Each arc passes through start, mid, end.
  const double cx = mCenter.x();
  const double cy = mCenter.y();
  const QgsPoint pE( cx + r, cy );
  const QgsPoint pN( cx, cy + r );
  const QgsPoint pW( cx - r, cy );
  const QgsPoint pS( cx, cy - r );

  auto *top = new QgsCircularString( pE, pN, pW );
  auto *bottom = new QgsCircularString( pW, pS, pE );

  auto *compound = new QgsCompoundCurve();
  compound->addCurve( top );
  compound->addCurve( bottom );

  auto *poly = new QgsCurvePolygon();
  poly->setExteriorRing( compound );
  setGeometry( poly );
}

void KadasCircleAnnotationItem::setCenter( const QgsPointXY &center )
{
  mCenter = center;
  rebuildGeometry();
}

void KadasCircleAnnotationItem::setRingPoint( const QgsPointXY &ringPoint )
{
  mRingPoint = ringPoint;
  rebuildGeometry();
}

void KadasCircleAnnotationItem::setCircle( const QgsPointXY &center, const QgsPointXY &ringPoint )
{
  mCenter = center;
  mRingPoint = ringPoint;
  rebuildGeometry();
}

bool KadasCircleAnnotationItem::writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  // The parent serializes the curve-polygon geometry + fill symbol + common
  // properties. We add the canonical center/ring parameters so loading is
  // exact (the parent's WKT is a fall-back / sanity record).
  QgsAnnotationPolygonItem::writeXml( element, document, context );
  element.setAttribute( QStringLiteral( "cx" ), qgsDoubleToString( mCenter.x() ) );
  element.setAttribute( QStringLiteral( "cy" ), qgsDoubleToString( mCenter.y() ) );
  element.setAttribute( QStringLiteral( "rx" ), qgsDoubleToString( mRingPoint.x() ) );
  element.setAttribute( QStringLiteral( "ry" ), qgsDoubleToString( mRingPoint.y() ) );
  return true;
}

bool KadasCircleAnnotationItem::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{
  QgsAnnotationPolygonItem::readXml( element, context );
  mCenter = QgsPointXY( element.attribute( QStringLiteral( "cx" ) ).toDouble(), element.attribute( QStringLiteral( "cy" ) ).toDouble() );
  mRingPoint = QgsPointXY( element.attribute( QStringLiteral( "rx" ) ).toDouble(), element.attribute( QStringLiteral( "ry" ) ).toDouble() );
  rebuildGeometry();
  return true;
}

KadasCircleAnnotationItem *KadasCircleAnnotationItem::clone() const
{
  auto *item = new KadasCircleAnnotationItem( mCenter, mRingPoint );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->copyCommonProperties( this );
  return item;
}

KadasCircleAnnotationItem *KadasCircleAnnotationItem::create()
{
  return new KadasCircleAnnotationItem();
}
