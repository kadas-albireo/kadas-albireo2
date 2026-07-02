/***************************************************************************
    kadasannotationrotation.h
    -------------------------
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

#ifndef KADASANNOTATIONROTATION_H
#define KADASANNOTATIONROTATION_H

#include <QPointF>

#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

class QPainter;

#ifndef SIP_RUN

/**
 * \ingroup gui
 * \brief Shared rotation-handle helpers for parametric annotation items.
 *
 * Angles are degrees, clockwise positive, with 0 pointing to map north (+Y);
 * this matches the rectangle item's existing convention and QGIS symbol/text
 * rotation. The handle sits a fixed pixel distance above the item centre.
 */
namespace KadasAnnotationRotation
{
  //! Pixel distance from the centre to the rotation handle.
  constexpr double sHandleOffsetPixels = 30.0;
  //! Snap step (degrees) applied while a modifier is held.
  constexpr double sSnapStep = 15.0;

  //! Map position of the rotation handle for \a centerMap at \a angleDeg, \a offsetMap units above centre.
  QgsPointXY handlePos( const QgsPointXY &centerMap, double angleDeg, double offsetMap );

  //! Clockwise angle (deg) from north so the handle lands under \a handleMap relative to \a centerMap.
  double angleFromHandle( const QgsPointXY &centerMap, const QgsPointXY &handleMap );

  //! Rotates \a p by \a angleDeg around \a center (same convention as handlePos()/angleFromHandle()).
  QgsPointXY rotatePoint( const QgsPointXY &p, const QgsPointXY &center, double angleDeg );

  //! Returns \a deg normalised to [0,360); snapped to sSnapStep when \a snap.
  double snapAngle( double deg, bool snap );

  //! Paints the circular rotation handle at screen \a pt with nominal \a size.
  void renderHandle( QPainter *painter, const QPointF &pt, int size );
} //namespace KadasAnnotationRotation

#endif // SIP_RUN

#endif // KADASANNOTATIONROTATION_H
