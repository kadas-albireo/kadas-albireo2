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
 * \brief Linkage to QGIS-compat "shadow" annotation items.
 *
 * Kadas-specific annotation items (rectangle, circle, pin, coord-cross)
 * keep their \c kadas:* type id and round-trip through QGIS-aware
 * controllers in Kadas. To stay visible when the same project is opened in
 * vanilla QGIS, master items emit parallel "shadow" items in stock QGIS
 * types (polygon / marker / pointtext) at project-save time. Shadows live
 * only in saved XML; they are added to the layer right before save and
 * stripped right after save and right after load.
 *
 * Each master item owns one of these value objects to track the UUIDs of
 * its emitted shadows. The list is persisted as a comma-separated value
 * under the \c kadasShadowIds XML attribute.
 */
class KadasAnnotationShadow
{
  public:
    //! Returns the shadow UUIDs tracked by this object.
    const QStringList &ids() const { return mIds; }

    //! Replaces the tracked shadow UUIDs.
    void setIds( const QStringList &ids ) { mIds = ids; }

    //! \c true when no shadows are tracked.
    bool isEmpty() const { return mIds.isEmpty(); }

    //! Writes the \c kadasShadowIds attribute on \a element when non-empty.
    void writeXml( QDomElement &element ) const
    {
      if ( !mIds.isEmpty() )
        element.setAttribute( attributeName(), mIds.join( QLatin1Char( ',' ) ) );
    }

    //! Reads the \c kadasShadowIds attribute from \a element. Missing attribute yields an empty list.
    void readXml( const QDomElement &element )
    {
      const QString value = element.attribute( attributeName() );
      mIds = value.isEmpty() ? QStringList() : value.split( QLatin1Char( ',' ), Qt::SkipEmptyParts );
    }

  private:
    static QString attributeName() { return QStringLiteral( "kadasShadowIds" ); }

    QStringList mIds;
};

#endif // KADASANNOTATIONSHADOW_H
