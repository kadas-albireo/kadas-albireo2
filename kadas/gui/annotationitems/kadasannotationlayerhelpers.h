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

#include <qgis/qgscoordinatereferencesystem.h>

#include "kadas/gui/kadas_gui.h"

class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Static helpers for attaching Kadas-specific per-item metadata to a stock QgsAnnotationLayer.
 */
class KADAS_GUI_EXPORT KadasAnnotationLayerHelpers
{
  public:
    //! TRUE if \a layer is a parametric Kadas annotation layer (carries the kadas/annotation-type customProperty).
    static bool isParametricLayer( const QgsAnnotationLayer *layer );

    //! Tooltip stored for \a itemId on \a layer, or empty.
    static QString tooltip( const QgsAnnotationLayer *layer, const QString &itemId );

    //! Sets \a tooltip for \a itemId; empty removes it.
    static void setTooltip( QgsAnnotationLayer *layer, const QString &itemId, const QString &tooltip );

    //! Creates a QgsAnnotationLayer named \a name (using \a preferredCrs, else project CRS, else EPSG:3857). Not added to the project.
    static QgsAnnotationLayer *createLayer( const QString &name, const QgsCoordinateReferenceSystem &preferredCrs = QgsCoordinateReferenceSystem() );

    //! Generates and inserts QGIS-compat shadow items for every master item. Call before QgsProject::write(). Idempotent.
    static void prepareLayerForSave( QgsAnnotationLayer *layer );

    //! Removes shadow items and clears master shadow id lists. Call after QgsProject::write() and after read().
    static void stripShadowsFromLayer( QgsAnnotationLayer *layer );

    //! Rebuilds coordinate-cross items orphaned by a vanilla-QGIS round trip (unknown master type dropped, stock shadows kept), recovering position from the shadow marker. Consumes the orphan side-channel. Call after read().
    static void reconstructOrphanCrosses( QgsAnnotationLayer *layer );

  private:
    KadasAnnotationLayerHelpers() = delete;
#ifdef SIP_RUN
    KadasAnnotationLayerHelpers();
#endif
};

#endif // KADASANNOTATIONLAYERHELPERS_H
