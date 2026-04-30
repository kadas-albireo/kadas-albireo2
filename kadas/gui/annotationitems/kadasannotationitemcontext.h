/***************************************************************************
    kadasannotationitemcontext.h
    ----------------------------
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

#ifndef KADASANNOTATIONITEMCONTEXT_H
#define KADASANNOTATIONITEMCONTEXT_H

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmapsettings.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Bundles the per-call context that \c KadasAnnotationItemController
 *        needs for map-space ↔ item-space transforms.
 *
 * A \c QgsAnnotationItem stores its geometry in the CRS of the
 * \c QgsAnnotationLayer it belongs to (or in the layer's CRS once added).
 * Map tools in turn work in the map canvas CRS.  Controllers therefore need
 * both pieces of information to project geometry between the two; this
 * lightweight struct passes them together.
 */
class KADAS_GUI_EXPORT KadasAnnotationItemContext
{
  public:
    KadasAnnotationItemContext() = default;
    KadasAnnotationItemContext( const QgsCoordinateReferenceSystem &itemCrs, const QgsMapSettings &mapSettings )
      : mItemCrs( itemCrs )
      , mMapSettings( mapSettings )
    {}

    //! CRS of the item (i.e. the parent annotation layer's CRS).
    const QgsCoordinateReferenceSystem &itemCrs() const { return mItemCrs; }

    //! Map canvas settings; \c destinationCrs() is the map CRS.
    const QgsMapSettings &mapSettings() const { return mMapSettings; }

  private:
    QgsCoordinateReferenceSystem mItemCrs;
    QgsMapSettings mMapSettings;
};

#endif // KADASANNOTATIONITEMCONTEXT_H
