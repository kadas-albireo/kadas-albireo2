/***************************************************************************
    kadasannotationvertexedit.cpp
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

#include <QPainter>
#include <QPolygonF>

#include "kadas/gui/annotationitems/kadasannotationvertexedit.h"


void KadasAnnotationVertexEdit::renderHandle( QPainter *painter, const QPointF &pt, int size )
{
  // A diamond at roughly three quarters of the vertex handle's footprint: close
  // enough to read as a grabbable handle, different enough that it never gets
  // mistaken for a vertex the shape already has.
  const double half = 0.4 * size;
  QPolygonF diamond;
  diamond << QPointF( pt.x(), pt.y() - half ) << QPointF( pt.x() + half, pt.y() ) << QPointF( pt.x(), pt.y() + half ) << QPointF( pt.x() - half, pt.y() );

  painter->save();
  painter->setRenderHint( QPainter::Antialiasing, true );
  painter->setBrush( QBrush( QColor( 255, 255, 255, 170 ) ) );
  painter->setPen( QPen( QColor( 0, 0, 0, 170 ), 1 ) );
  painter->drawPolygon( diamond );
  painter->restore();
}
