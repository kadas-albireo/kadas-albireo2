/***************************************************************************
    kadasannotationrotation.cpp
    ---------------------------
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

#include <QBrush>
#include <QPainter>
#include <QPen>

#include "kadas/gui/annotationitems/kadasannotationrotation.h"

QgsPointXY KadasAnnotationRotation::handlePos( const QgsPointXY &centerMap, double angleDeg, double offsetMap )
{
  // angleDeg is clockwise from north (matching QGIS symbol/text rotation).
  const double rad = angleDeg * M_PI / 180.0;
  return QgsPointXY( centerMap.x() + offsetMap * std::sin( rad ), centerMap.y() + offsetMap * std::cos( rad ) );
}

double KadasAnnotationRotation::angleFromHandle( const QgsPointXY &centerMap, const QgsPointXY &handleMap )
{
  const double dx = handleMap.x() - centerMap.x();
  const double dy = handleMap.y() - centerMap.y();
  // Clockwise from north: north=0, east=+90, west=-90.
  return std::atan2( dx, dy ) * 180.0 / M_PI;
}

QgsPointXY KadasAnnotationRotation::rotatePoint( const QgsPointXY &p, const QgsPointXY &center, double angleDeg )
{
  // Clockwise rotation to match the handle convention (north vertex at angle +90 -> east).
  const double rad = -angleDeg * M_PI / 180.0;
  const double c = std::cos( rad );
  const double s = std::sin( rad );
  const double dx = p.x() - center.x();
  const double dy = p.y() - center.y();
  return QgsPointXY( center.x() + dx * c - dy * s, center.y() + dx * s + dy * c );
}

double KadasAnnotationRotation::snapAngle( double deg, bool snap )
{
  if ( snap )
    deg = std::round( deg / sSnapStep ) * sSnapStep;
  deg = std::fmod( deg, 360.0 );
  if ( deg < 0 )
    deg += 360.0;
  return deg;
}

void KadasAnnotationRotation::renderHandle( QPainter *painter, const QPointF &pt, int size )
{
  const double r = 0.5 * size;
  painter->save();
  painter->setRenderHint( QPainter::Antialiasing, true );
  painter->setPen( QPen( QColor( 0, 122, 0 ), 1.5 ) );
  painter->setBrush( QBrush( QColor( 255, 255, 255 ) ) );
  painter->drawEllipse( pt, r, r );
  painter->setPen( QPen( QColor( 0, 122, 0 ), 1.0 ) );
  painter->drawLine( QPointF( pt.x() - 0.4 * r, pt.y() ), QPointF( pt.x() + 0.4 * r, pt.y() ) );
  painter->drawLine( QPointF( pt.x(), pt.y() - 0.4 * r ), QPointF( pt.x(), pt.y() + 0.4 * r ) );
  painter->restore();
}
