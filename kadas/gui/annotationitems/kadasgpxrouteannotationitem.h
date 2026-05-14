/***************************************************************************
    kadasgpxrouteannotationitem.h
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

#ifndef KADASGPXROUTEANNOTATIONITEM_H
#define KADASGPXROUTEANNOTATIONITEM_H

#include <QColor>
#include <QFont>
#include <QString>

#include <qgis/qgsannotationlineitem.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal annotation item representing a GPX route.
 *
 * Subclasses \c QgsAnnotationLineItem and pre-installs a yellow line
 * symbol matching the legacy \c KadasGpxRouteItem default style. Adds
 * \c name, \c number, \c labelFont and \c labelColor fields, and renders
 * the name as repeating buffered labels along the route.
 *
 * Type id: \c "kadas:gpxroute".
 */
class KADAS_GUI_EXPORT KadasGpxRouteAnnotationItem : public QgsAnnotationLineItem
{
  public:
    explicit KadasGpxRouteAnnotationItem( QgsCurve *curve );
    KadasGpxRouteAnnotationItem();

    static QString itemTypeId() { return QStringLiteral( "kadas:gpxroute" ); }

    QString type() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasGpxRouteAnnotationItem *clone() const override;

    static KadasGpxRouteAnnotationItem *create();

    QString name() const { return mName; }
    void setName( const QString &name ) { mName = name; }

    QString number() const { return mNumber; }
    void setNumber( const QString &number ) { mNumber = number; }

    QFont labelFont() const { return mLabelFont; }
    void setLabelFont( const QFont &font ) { mLabelFont = font; }

    QColor labelColor() const { return mLabelColor; }
    void setLabelColor( const QColor &color ) { mLabelColor = color; }

  private:
    QString mName;
    QString mNumber;
    QFont mLabelFont;
    QColor mLabelColor;

    void installDefaultSymbol();
};

#endif // KADASGPXROUTEANNOTATIONITEM_H
