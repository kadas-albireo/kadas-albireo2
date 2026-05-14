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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmapsettings.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Bundles the per-call context that \c KadasAnnotationItemController
 *        needs for map-space ↔ item-space transforms.
 *
 * A \c QgsAnnotationItem stores its geometry in the CRS of the
 * \c QgsAnnotationLayer it belongs to.  Map tools in turn work in the map
 * canvas CRS.  Controllers therefore need both pieces of information to
 * project geometry between the two; this lightweight struct passes them
 * together.  Aligns with QGIS's own annotation tooling, where the layer is
 * the single source of truth for the item CRS.
 */
class KADAS_GUI_EXPORT KadasAnnotationItemContext
{
  public:
    KadasAnnotationItemContext() = default;
    KadasAnnotationItemContext( QgsAnnotationLayer *layer, const QgsMapSettings &mapSettings )
      : mLayer( layer )
      , mMapSettings( mapSettings )
    {}

    //! Owning annotation layer.
    QgsAnnotationLayer *layer() const { return mLayer; }

    //! Map canvas settings; \c destinationCrs() is the map CRS.
    const QgsMapSettings &mapSettings() const { return mMapSettings; }

    //! CRS of the item, i.e. the parent annotation layer's CRS.
    QgsCoordinateReferenceSystem itemCrs() const { return mLayer ? mLayer->crs() : QgsCoordinateReferenceSystem(); }

  private:
    QgsAnnotationLayer *mLayer = nullptr;
    QgsMapSettings mMapSettings;
};

#endif // KADASANNOTATIONITEMCONTEXT_H
