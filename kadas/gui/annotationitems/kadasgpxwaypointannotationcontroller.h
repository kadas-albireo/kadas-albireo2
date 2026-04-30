/***************************************************************************
    kadasgpxwaypointannotationcontroller.h
    --------------------------------------
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

#ifndef KADASGPXWAYPOINTANNOTATIONCONTROLLER_H
#define KADASGPXWAYPOINTANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasGpxWaypointAnnotationItem (type id
 * \c "kadas:gpxwaypoint").
 *
 * Inherits the marker controller's draw / edit / KML behavior; only the
 * type id, display name and item factory differ.
 */
class KADAS_GUI_EXPORT KadasGpxWaypointAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasGpxWaypointAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;
};

#endif // KADASGPXWAYPOINTANNOTATIONCONTROLLER_H
