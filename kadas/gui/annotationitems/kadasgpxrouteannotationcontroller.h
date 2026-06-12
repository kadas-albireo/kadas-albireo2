/***************************************************************************
    kadasgpxrouteannotationcontroller.h
    -----------------------------------
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

#ifndef KADASGPXROUTEANNOTATIONCONTROLLER_H
#define KADASGPXROUTEANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasGpxRouteAnnotationItem (type id
 * \c "kadas:gpxroute").
 *
 * Inherits the line controller's draw / edit / KML behavior; only the
 * type id, display name and item factory differ.
 */
class KADAS_GUI_EXPORT KadasGpxRouteAnnotationController : public KadasLineAnnotationController
{
  public:
    KadasGpxRouteAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;
};

#endif // KADASGPXROUTEANNOTATIONCONTROLLER_H
