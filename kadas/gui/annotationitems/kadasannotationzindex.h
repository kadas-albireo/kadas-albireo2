/***************************************************************************
    kadasannotationzindex.h
    -----------------------
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

#ifndef KADASANNOTATIONZINDEX_H
#define KADASANNOTATIONZINDEX_H

#include "kadas/gui/kadas_gui.h"

/**
 * Default z-index buckets for Kadas annotation items.
 *
 * QgsAnnotationLayer renders items by ascending zIndex(); items with the same
 * zIndex are ordered by item id (UUID), which is effectively random. To
 * preserve the legacy KadasItemLayer painter order, every annotation created
 * by Kadas is assigned one of these bucketed values. Within a bucket, callers
 * that need stable ordering (e.g. importers) should apply a small monotonic
 * offset so file order is preserved.
 */
namespace KadasAnnotationZIndex
{
  // Filled / area items go first so lines and markers paint on top.
  constexpr int Polygon = 0;
  constexpr int Rectangle = Polygon;
  constexpr int Circle = Polygon;

  // Linear items.
  constexpr int Line = 10000;
  constexpr int GpxRoute = Line;

  // Bitmap / raster pictures (above polygons but below markers and labels).
  constexpr int Picture = 20000;

  // Point markers.
  constexpr int Marker = 30000;
  constexpr int Pin = Marker;
  constexpr int CoordCross = Marker;
  constexpr int GpxWaypoint = Marker;

  // Text / labels paint above everything else.
  constexpr int PointText = 40000;
} // namespace KadasAnnotationZIndex

#endif // KADASANNOTATIONZINDEX_H
