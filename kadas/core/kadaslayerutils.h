/***************************************************************************
    kadaslayerutils.h
    -----------------
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

#ifndef KADASLAYERUTILS_H
#define KADASLAYERUTILS_H

#include <QString>

#include "kadas/core/kadas_core.h"

class QgsMapLayer;

/**
 * \ingroup core
 * \brief Static helpers for locking layers: a locked layer can neither be removed nor edited.
 */
class KADAS_CORE_EXPORT KadasLayerUtils
{
  public:
    //! Custom property holding the lock state; QgsMapLayer::customPropertyChanged() reports it with this key when the lock changes.
    static QString lockedPropertyKey();

    //! TRUE if \a layer is locked.
    static bool isLocked( const QgsMapLayer *layer );

    //! Locks or unlocks \a layer: makes it (non-)removable and, for vector layers, (not) read-only.
    static void setLocked( QgsMapLayer *layer, bool locked );

    //! Message telling the user that \a layer cannot be edited because it is locked.
    static QString lockedMessage( const QgsMapLayer *layer );
};

#endif // KADASLAYERUTILS_H
