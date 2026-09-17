/***************************************************************************
    kadasannotationvertexedit.h
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

#ifndef KADASANNOTATIONVERTEXEDIT_H
#define KADASANNOTATIONVERTEXEDIT_H

#define SIP_NO_FILE

#include <QPointF>

#include "kadas/gui/kadas_gui.h"

class QPainter;

/**
 * \ingroup gui
 * \brief Shared midpoint ("insert vertex") handle helpers for vertex-based annotations.
 *
 * A midpoint handle sits halfway along every segment of a finished line or
 * polygon. Dragging or clicking one turns it into a real vertex, which is the
 * only way to grow a shape once it is finished: digitizing itself cannot be
 * resumed. The line and polygon controllers share this so the two behave alike.
 */
namespace KadasAnnotationVertexEdit
{
  //! vidx.part sentinel for a midpoint handle. Real vertices use part 0, the rotation handle part 1.
  constexpr int kPartInsert = 2;

  //! Paints the midpoint handle (a small hollow diamond) at screen \a pt. Deliberately smaller than a vertex handle so the real vertices stay the prominent ones.
  void renderHandle( QPainter *painter, const QPointF &pt, int size );

  /**
   * \ingroup gui
   * \brief Per-drag state for a midpoint handle.
   *
   * getEditContext() arms the segment when the handle is grabbed; the first
   * edit() of that drag consumes the arming and inserts the vertex, and every
   * later step only moves it. Without the arming the insert would repeat on
   * every mouse move and leave a trail of vertices behind the cursor.
   */
  class KADAS_GUI_EXPORT VertexInsertState
  {
    public:
      //! Segment the armed insert belongs to (the vertex lands at index segment + 1), or -1 when nothing is armed.
      int segment() const { return mSegment; }

      //! Arms an insert on \a segment.
      void arm( int segment )
      {
        mSegment = segment;
        mPending = true;
      }

      //! Clears the armed segment; call on any hover hit-test so a finished drag does not leak into the next one.
      void disarm()
      {
        mSegment = -1;
        mPending = false;
      }

      //! TRUE exactly once per armed segment: the drag step that has to materialise the vertex.
      bool takePending()
      {
        const bool pending = mPending;
        mPending = false;
        return pending;
      }

    private:
      int mSegment = -1;
      bool mPending = false;
  };
} // namespace KadasAnnotationVertexEdit

#endif // KADASANNOTATIONVERTEXEDIT_H
