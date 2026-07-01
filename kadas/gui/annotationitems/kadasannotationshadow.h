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
 */
class KadasAnnotationShadow
{
  public:
    const QStringList &ids() const { return mIds; }

    void setIds( const QStringList &ids ) { mIds = ids; }

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
