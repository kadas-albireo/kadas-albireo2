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

#include "kadas/gui/annotationitems/kadasannotationvertexedit.h"


void KadasAnnotationVertexEdit::renderHandle( QPainter *painter, const QPointF &pt, int size )
{
  // A circled plus: the shape says "adds something" at a glance, and being round
  // it never gets mistaken for one of the square vertices the shape already has.
  const double radius = 0.55 * size;
  const double arm = 0.5 * radius;
  const QColor ink( 30, 30, 30, 235 );

  painter->save();
  painter->setRenderHint( QPainter::Antialiasing, true );
  // White halo first, so the handle keeps its contrast over a dark background or
  // over the item's own stroke, which runs straight through it.
  painter->setBrush( Qt::NoBrush );
  painter->setPen( QPen( QColor( 255, 255, 255, 230 ), 3 ) );
  painter->drawEllipse( pt, radius, radius );

  painter->setBrush( QBrush( QColor( 255, 255, 255, 235 ) ) );
  painter->setPen( QPen( ink, 1.2 ) );
  painter->drawEllipse( pt, radius, radius );

  painter->setPen( QPen( ink, 1.4, Qt::SolidLine, Qt::RoundCap ) );
  painter->drawLine( QPointF( pt.x() - arm, pt.y() ), QPointF( pt.x() + arm, pt.y() ) );
  painter->drawLine( QPointF( pt.x(), pt.y() - arm ), QPointF( pt.x(), pt.y() + arm ) );
  painter->restore();
}

bool KadasAnnotationVertexEdit::isRevealed( const QgsPointXY &midpointMap, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY cursor = ctx.cursorPos();
  if ( cursor.isEmpty() || midpointMap.isEmpty() )
    return false;
  const double radius = kRevealRadiusPixels * ctx.mapSettings().mapUnitsPerPixel();
  return cursor.sqrDist( midpointMap ) < radius * radius;
}
