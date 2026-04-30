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
#include <QStringList>

#include "kadas/gui/kadas_gui.h"

class QgsAnnotationLayer;

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
    //! Returns the editor widget name stored for \a itemId on \a layer, or
    //! an empty string if none is set.
    static QString editorName( const QgsAnnotationLayer *layer, const QString &itemId );

    //! Sets the editor widget \a name for \a itemId on \a layer. Passing an
    //! empty \a name removes the override.
    static void setEditorName( QgsAnnotationLayer *layer, const QString &itemId, const QString &name );

    //! Returns the tooltip stored for \a itemId on \a layer, or an empty
    //! string if none is set.
    static QString tooltip( const QgsAnnotationLayer *layer, const QString &itemId );

    //! Sets the \a tooltip for \a itemId on \a layer. Passing an empty
    //! \a tooltip removes it.
    static void setTooltip( QgsAnnotationLayer *layer, const QString &itemId, const QString &tooltip );

    //! Removes all Kadas metadata stored for \a itemId on \a layer.
    //! Should be invoked when the item is removed from the layer.
    static void clearMetadata( QgsAnnotationLayer *layer, const QString &itemId );

    //! Returns all item ids that currently have any Kadas metadata stored on \a layer.
    static QStringList itemsWithMetadata( const QgsAnnotationLayer *layer );

  private:
    KadasAnnotationLayerHelpers() = delete;
};

#endif // KADASANNOTATIONLAYERHELPERS_H
