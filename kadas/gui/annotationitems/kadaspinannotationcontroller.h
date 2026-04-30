/***************************************************************************
    kadaspinannotationcontroller.h
    ------------------------------
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

#ifndef KADASPINANNOTATIONCONTROLLER_H
#define KADASPINANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasPinAnnotationItem (type id \c "kadas:pin").
 *
 * Pins are markers with a fixed default SVG icon and Kadas-specific
 * \c name / \c remarks fields. Behavior (single-click placement, vertex
 * edit, KML export) is identical to the generic marker, so this controller
 * inherits from \c KadasMarkerAnnotationController and only overrides
 * \c itemType / \c itemName / \c createItem.
 */
class KADAS_GUI_EXPORT KadasPinAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasPinAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;
};

#endif // KADASPINANNOTATIONCONTROLLER_H
