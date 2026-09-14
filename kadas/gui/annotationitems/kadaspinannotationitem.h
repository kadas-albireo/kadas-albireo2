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

//! "Pin" annotation item (type id "kadas:pin").
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

    //! Rich text (Qt's HTML subset), or plain text; images are attachment:/// references within it.
    QString remarks() const { return mRemarks; }
    void setRemarks( const QString &remarks ) { mRemarks = remarks; }

    /**
     * \a remarks as markup ready to be appended to a document: rich text as it
     * stands, plain text escaped and wrapped in a block.
     *
     * Which of the two a description is, is a guess — Kadas 2.x stored both in
     * this one field, with nothing to tell them apart — so it is made here once
     * and everything that renders or searches a description guesses alike.
     */
    static QString remarksAsHtml( const QString &remarks );

    //! \a remarks as searchable text, with any markup resolved rather than matched against.
    static QString remarksAsPlainText( const QString &remarks );

    static QString defaultIconPath();

    const QStringList &shadowIds() const { return mShadow.ids(); }
    void setShadowIds( const QStringList &ids ) { mShadow.setIds( ids ); }

  private:
    QString mName;
    QString mRemarks;
    KadasAnnotationShadow mShadow;

    void installDefaultSymbol();
};

#endif // KADASPINANNOTATIONITEM_H
