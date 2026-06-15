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

#define SIP_NO_FILE

#include "kadas/gui/kadas_gui.h"

/**
 * \brief Default z-index buckets for Kadas annotation items.
 *
 * QgsAnnotationLayer renders by ascending zIndex(); ties are ordered by UUID (random).
 */
class KadasAnnotationZIndex
{
  public:
    // Filled / area items go first so lines and markers paint on top.
    static constexpr int Polygon = 0;
    static constexpr int Rectangle = Polygon;
    static constexpr int Circle = Polygon;

    // Linear items.
    static constexpr int Line = 10000;
    static constexpr int GpxRoute = Line;

    // Bitmap / raster pictures (above polygons but below markers and labels).
    static constexpr int Picture = 20000;

    // Point markers.
    static constexpr int Marker = 30000;
    static constexpr int Pin = Marker;
    static constexpr int CoordCross = Marker;
    static constexpr int GpxWaypoint = Marker;
    static constexpr int Milx = Marker;

    // Text / labels paint above everything else.
    static constexpr int PointText = 40000;
};

#endif // KADASANNOTATIONZINDEX_H
