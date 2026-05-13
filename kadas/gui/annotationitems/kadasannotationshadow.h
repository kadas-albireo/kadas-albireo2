/***************************************************************************
    kadasannotationshadow.h
    -----------------------
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

#ifndef KADASANNOTATIONSHADOW_H
#define KADASANNOTATIONSHADOW_H

#define SIP_NO_FILE

#include <QDomElement>
#include <QString>
#include <QStringList>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Helpers for QGIS-compat "shadow" annotation items.
 *
 * Kadas-specific annotation items (rectangle, circle, pin, coord-cross)
 * keep their \c kadas:* type id and round-trip through QGIS-aware
 * controllers in Kadas. To stay visible when the same project is opened in
 * vanilla QGIS, master items emit parallel "shadow" items in stock QGIS
 * types (polygon / marker / pointtext) at project-save time. Shadows live
 * only in saved XML; they are added to the layer right before save and
 * stripped right after save and right after load.
 *
 * Linkage: each master item stores the list of UUIDs of its shadows in a
 * \c kadasShadowIds XML attribute. The helpers below centralize the XML
 * attribute name and serialization format.
 */
namespace KadasAnnotationShadow
{
  //! XML attribute name on a master item element listing comma-separated shadow UUIDs.
  inline QString shadowIdsAttribute()
  {
    return QStringLiteral( "kadasShadowIds" );
  }

  //! Encodes a list of shadow ids into the value used in the \c kadasShadowIds attribute.
  inline QString encodeIds( const QStringList &ids )
  {
    return ids.join( QLatin1Char( ',' ) );
  }

  //! Decodes a comma-separated shadow id list. Empty string yields an empty list.
  inline QStringList decodeIds( const QString &value )
  {
    if ( value.isEmpty() )
      return QStringList();
    return value.split( QLatin1Char( ',' ), Qt::SkipEmptyParts );
  }
} // namespace KadasAnnotationShadow

#endif // KADASANNOTATIONSHADOW_H
