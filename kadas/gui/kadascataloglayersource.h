/***************************************************************************
    kadascataloglayersource.h
    -------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASCATALOGLAYERSOURCE_H
#define KADASCATALOGLAYERSOURCE_H

#include <QString>
#include <QVariant>

#include <qgis/qgsmimedatautils.h>

#include "kadas/gui/kadas_gui.h"

class QgsCoordinateReferenceSystem;

/**
 * What a layer for a catalog entry is created from: the provider, the source
 * string, the layer name and the layer type.
 *
 * Adding a catalog entry and previewing it have to resolve the very same
 * source, otherwise the preview would not show what gets added, so both go
 * through resolve() here.
 */
struct KADAS_GUI_EXPORT KadasCatalogLayerSource
{
    enum class Type
    {
      Unknown,
      Raster,
      Vector,
      VectorTile
    };

    Type type = Type::Unknown;
    QString providerKey;
    QString uri;
    QString name;

    /**
     * TRUE if this resolves to a layer. Only a default constructed source is
     * invalid by type: resolve() always yields a concrete one, falling back to
     * a raster layer for providers it does not know.
     */
    bool isValid() const { return type != Type::Unknown && !uri.isEmpty(); }

    /**
     * Rewrites the entry uri of \a uri to \a canvasCrs and to the last used
     * image format, if the entry advertises support for them.
     */
    static QString adjustUri( const QgsMimeDataUtils::Uri &uri, const QgsCoordinateReferenceSystem &canvasCrs );

    /**
     * Resolves the source a layer named \a name would be created from, for a
     * catalog entry of \a providerKey whose uri went through adjustUri() into
     * \a adjustedUri.
     *
     * \a sublayerId is the "id" of the sublayer to resolve; pass an invalid
     * QVariant for entries which carry no sublayers.
     */
    static KadasCatalogLayerSource resolve( const QString &providerKey, const QString &adjustedUri, const QVariant &sublayerId, const QString &name );
};

#endif // KADASCATALOGLAYERSOURCE_H
