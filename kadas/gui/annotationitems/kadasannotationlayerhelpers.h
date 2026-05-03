/***************************************************************************
    kadasannotationlayerhelpers.h
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

#ifndef KADASANNOTATIONLAYERHELPERS_H
#define KADASANNOTATIONLAYERHELPERS_H

#include <QString>

#include "kadas/gui/kadas_gui.h"

class QgsAnnotationLayer;
class QgsCoordinateReferenceSystem;

/**
 * \ingroup gui
 * \brief Static helpers for attaching Kadas-specific per-item metadata
 *        (editor widget name, tooltip) to a stock \c QgsAnnotationLayer.
 *
 * Stock \c QgsAnnotationItem subclasses do not expose a custom-property bag,
 * so per-instance metadata that Kadas needs (and which has no upstream
 * equivalent) is stored on the owning layer instead.  This keeps the items
 * themselves stock and round-trippable through vanilla QGIS, at the cost of
 * losing metadata when an item is moved between layers (which Kadas does not
 * currently allow from the UI).
 *
 * Storage schema: each datum is stored as a layer custom property keyed
 * <tt>kadas:item-meta:&lt;itemId&gt;:&lt;field&gt;</tt>.  Project save/load
 * preserves these because layer custom properties are part of the standard
 * QGIS layer XML.
 */
class KADAS_GUI_EXPORT KadasAnnotationLayerHelpers
{
  public:
    //! Returns the tooltip stored for \a itemId on \a layer, or an empty
    //! string if none is set.
    static QString tooltip( const QgsAnnotationLayer *layer, const QString &itemId );

    //! Sets the \a tooltip for \a itemId on \a layer. Passing an empty
    //! \a tooltip removes it.
    static void setTooltip( QgsAnnotationLayer *layer, const QString &itemId, const QString &tooltip );

    /**
     * Creates a new \c QgsAnnotationLayer named \a name. If \a preferredCrs is
     * valid it is used as the layer CRS (so callers importing data in a
     * known native CRS can avoid round-trip reprojection). Otherwise the
     * current project CRS is used, falling back to EPSG:3857 if the project
     * has no CRS. The returned layer is NOT added to the project; the
     * caller is expected to call \c QgsProject::instance()->addMapLayer().
     */
    static QgsAnnotationLayer *createLayer( const QString &name, const QgsCoordinateReferenceSystem &preferredCrs = QgsCoordinateReferenceSystem() );

  private:
    KadasAnnotationLayerHelpers() = delete;
};

#endif // KADASANNOTATIONLAYERHELPERS_H
