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
#include <QVector>

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

  /**
   * \ingroup gui
   * \brief Reusable per-drag rotation state for vertex-based annotations (line, polygon).
   *
   * Captures the geometry's vertices (map CRS) and a fixed pivot when the
   * rotation handle is grabbed, then produces rotated vertices for each drag or
   * numeric-angle step. Rotating the snapshot (rather than the live geometry)
   * keeps the drag drift-free. Both the line and polygon controllers share this
   * so the rotation behaviour stays identical.
   */
  class KADAS_GUI_EXPORT VertexRotationState
  {
    public:
      //! True while a rotation drag is in progress (handle follows the cursor).
      bool active() const { return mActive; }
      //! Map position at which to draw the handle while dragging.
      QgsPointXY handle() const { return mHandle; }
      //! Fixed rotation pivot (map CRS) captured when the handle was grabbed.
      QgsPointXY center() const { return mCenter; }
      //! True once a drag snapshot has been captured.
      bool hasSnapshot() const { return !mOrig.isEmpty(); }
      //! Number of snapshotted vertices (must match the live geometry on apply).
      int vertexCount() const { return mOrig.size(); }
      //! Clears the active flag; call on any hover hit-test so the handle returns to rest.
      void deactivate() { mActive = false; }

      //! Handle position at rest: \a offsetMap units north of \a centerMap.
      static QgsPointXY restHandle( const QgsPointXY &centerMap, double offsetMap );

      //! Captures \a verticesMap, pivot \a centerMap and \a handleMap when the handle is grabbed.
      void begin( const QVector<QgsPointXY> &verticesMap, const QgsPointXY &centerMap, const QgsPointXY &handleMap );

      //! Interactive drag toward \a cursorMap; returns the rotated vertices (map). \a snap snaps the angle.
      QVector<QgsPointXY> dragTo( const QgsPointXY &cursorMap, bool snap );

      //! Rotates by an absolute \a deltaDeg (numeric edit); updates the handle at \a offsetMap; returns rotated vertices (map).
      QVector<QgsPointXY> applyAngle( double deltaDeg, double offsetMap );

      //! Angle (deg) of \a cursorMap relative to the reference handle, for reporting the current rotation.
      double angleFromCursor( const QgsPointXY &cursorMap ) const;

      //! Handle map position for an absolute \a deltaDeg rotation, \a offsetMap above the pivot.
      QgsPointXY handleForAngle( double deltaDeg, double offsetMap ) const;

    private:
      QVector<QgsPointXY> mOrig;
      QgsPointXY mCenter;
      double mRefAngle = 0.0;
      bool mActive = false;
      QgsPointXY mHandle;
  };
} //namespace KadasAnnotationRotation

#endif // SIP_RUN

#endif // KADASANNOTATIONROTATION_H
