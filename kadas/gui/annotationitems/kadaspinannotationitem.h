/***************************************************************************
    kadaspinannotationitem.h
    ------------------------
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

#ifndef KADASPINANNOTATIONITEM_H
#define KADASPINANNOTATIONITEM_H

#include <QString>
#include <QStringList>

#include <qgis/qgsannotationmarkeritem.h>

#include "kadas/gui/annotationitems/kadasannotationshadow.h"
#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal "pin" annotation item.
 *
 * Subclasses \c QgsAnnotationMarkerItem and pre-configures the marker
 * symbol with the standard Kadas pin SVG (anchored at the bottom tip).
 * Adds two extra text fields, \c name and \c remarks, that are used by
 * the layer tree for tooltips.
 *
 * Type id: \c "kadas:pin".
 */
class KADAS_GUI_EXPORT KadasPinAnnotationItem : public QgsAnnotationMarkerItem
{
  public:
    KadasPinAnnotationItem( const QgsPoint &point = QgsPoint() );

    static QString itemTypeId() { return QStringLiteral( "kadas:pin" ); }

    QString type() const override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasPinAnnotationItem *clone() const override;

    static KadasPinAnnotationItem *create();

    QString name() const { return mName; }
    void setName( const QString &name ) { mName = name; }

    QString remarks() const { return mRemarks; }
    void setRemarks( const QString &remarks ) { mRemarks = remarks; }

    //! Path of the SVG icon used by default for new pins (resource path).
    static QString defaultIconPath();

    //! UUIDs of save-time QGIS-compat shadow items linked to this master.
    //! See \c KadasAnnotationShadow.
    const QStringList &shadowIds() const { return mShadow.ids(); }
    void setShadowIds( const QStringList &ids ) { mShadow.setIds( ids ); }

  private:
    QString mName;
    QString mRemarks;
    KadasAnnotationShadow mShadow;

    void installDefaultSymbol();
};

#endif // KADASPINANNOTATIONITEM_H
