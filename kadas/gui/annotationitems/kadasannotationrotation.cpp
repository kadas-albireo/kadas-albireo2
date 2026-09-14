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
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QSvgRenderer>

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

void KadasAnnotationRotation::renderHandle( QPainter *painter, const QPointF &pt, int )
{
  // The knob has its own fixed size (sHandleRadiusPixels), not the caller's
  // vertex-node size: a rotation handle must never read as one more vertex.
  // The asset is authored at 1 unit = 1 pixel around a centred sHandleRadiusPixels
  // circle, with room for the arrowhead and its white buffer beyond that radius.
  constexpr double boxPixels = 26.0;
  // Parsed once: QSvgRenderer keeps the document tree, so painting is just a replay.
  static QSvgRenderer sRenderer { QStringLiteral( ":/kadas/icons/rotate_handle" ) };

  painter->save();
  painter->setRenderHint( QPainter::Antialiasing, true );
  sRenderer.render( painter, QRectF( pt.x() - 0.5 * boxPixels, pt.y() - 0.5 * boxPixels, boxPixels, boxPixels ) );
  painter->restore();
}

QgsPointXY KadasAnnotationRotation::VertexRotationState::restHandle( const QgsPointXY &centerMap, double offsetMap )
{
  return handlePos( centerMap, 0.0, offsetMap );
}

void KadasAnnotationRotation::VertexRotationState::begin( const QVector<QgsPointXY> &verticesMap, const QgsPointXY &centerMap, const QgsPointXY &handleMap )
{
  mOrig = verticesMap;
  mCenter = centerMap;
  mHandle = handleMap;
  mRefAngle = angleFromHandle( centerMap, handleMap );
  mActive = false;
}

QVector<QgsPointXY> KadasAnnotationRotation::VertexRotationState::dragTo( const QgsPointXY &cursorMap, bool snap )
{
  const double target = angleFromHandle( mCenter, cursorMap );
  const double delta = snapAngle( target - mRefAngle, snap );
  mActive = true;
  mHandle = cursorMap;
  QVector<QgsPointXY> out;
  out.reserve( mOrig.size() );
  for ( const QgsPointXY &p : mOrig )
    out.append( rotatePoint( p, mCenter, delta ) );
  return out;
}

QVector<QgsPointXY> KadasAnnotationRotation::VertexRotationState::applyAngle( double deltaDeg, double offsetMap )
{
  const double delta = snapAngle( deltaDeg, false );
  mActive = true;
  mHandle = handlePos( mCenter, mRefAngle + delta, offsetMap );
  QVector<QgsPointXY> out;
  out.reserve( mOrig.size() );
  for ( const QgsPointXY &p : mOrig )
    out.append( rotatePoint( p, mCenter, delta ) );
  return out;
}

double KadasAnnotationRotation::VertexRotationState::angleFromCursor( const QgsPointXY &cursorMap ) const
{
  return angleFromHandle( mCenter, cursorMap ) - mRefAngle;
}

QgsPointXY KadasAnnotationRotation::VertexRotationState::handleForAngle( double deltaDeg, double offsetMap ) const
{
  return handlePos( mCenter, mRefAngle + deltaDeg, offsetMap );
}
