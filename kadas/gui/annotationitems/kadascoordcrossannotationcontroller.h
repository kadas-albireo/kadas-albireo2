/***************************************************************************
    kadascoordcrossannotationcontroller.h
    -------------------------------------
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

#ifndef KADASCOORDCROSSANNOTATIONCONTROLLER_H
#define KADASCOORDCROSSANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasCoordCrossAnnotationItem (type id
 * \c "kadas:coordcross").
 *
 * The coordinate cross is a single-point item whose position is rounded
 * to the nearest kilometre on placement. Otherwise it follows the same
 * placement / edit workflow as a generic marker, so this controller
 * inherits from \c KadasMarkerAnnotationController and only overrides
 * \c itemType / \c itemName / \c createItem and the position setters
 * that need to apply the rounding.
 */
class KADAS_GUI_EXPORT KadasCoordCrossAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasCoordCrossAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    bool startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx ) override;
    void setPosition( QgsAnnotationItem *item, const KadasItemPos &pos ) override;
    void edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx ) override;
};

#endif // KADASCOORDCROSSANNOTATIONCONTROLLER_H
