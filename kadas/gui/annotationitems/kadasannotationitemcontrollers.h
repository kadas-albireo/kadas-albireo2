/***************************************************************************
    kadasannotationitemcontrollers.h
    --------------------------------
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

#ifndef KADASANNOTATIONITEMCONTROLLERS_H
#define KADASANNOTATIONITEMCONTROLLERS_H

#include "kadas/gui/kadas_gui.h"

namespace KadasAnnotationItemControllers
{
  /**
   * Registers the built-in Kadas annotation item controllers (marker, line,
   * polygon, picture, pointtext, circle, rectangle, pin, gpxwaypoint,
   * gpxroute) with the global \c KadasAnnotationControllerRegistry.
   *
   * Safe to call more than once: subsequent calls are no-ops.
   */
  KADAS_GUI_EXPORT void registerBuiltins();
} // namespace KadasAnnotationItemControllers

#endif // KADASANNOTATIONITEMCONTROLLERS_H
