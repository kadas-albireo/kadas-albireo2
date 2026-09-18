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
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Bundles the per-call context that \c KadasAnnotationItemController
 *        needs for map-space ↔ item-space transforms.
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

    //! Keyboard modifiers active during the current edit (e.g. Shift for angle snapping).
    Qt::KeyboardModifiers modifiers() const { return mModifiers; }
    void setModifiers( Qt::KeyboardModifiers modifiers ) { mModifiers = modifiers; }

    //! TRUE while the item is still being digitized. Handles that only make sense on a finished shape (midpoint insert handles) are left out then, so the trailing rubber-band segment does not sprout one that chases the cursor.
    bool digitizing() const { return mDigitizing; }
    void setDigitizing( bool digitizing ) { mDigitizing = digitizing; }

    //! Pointer position in map coordinates, empty when it is unknown (pointer off the canvas, or a caller that has none). Handles that would clutter the shape if they were all shown at once - the midpoint insert handles - only appear near it.
    QgsPointXY cursorPos() const { return mCursorPos; }
    void setCursorPos( const QgsPointXY &cursorPos ) { mCursorPos = cursorPos; }

  private:
    QgsAnnotationLayer *mLayer = nullptr;
    QgsMapSettings mMapSettings;
    Qt::KeyboardModifiers mModifiers = Qt::NoModifier;
    bool mDigitizing = false;
    QgsPointXY mCursorPos;
};

#endif // KADASANNOTATIONITEMCONTEXT_H
