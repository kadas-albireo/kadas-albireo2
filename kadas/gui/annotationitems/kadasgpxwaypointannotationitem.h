/***************************************************************************
    kadasgpxwaypointannotationitem.h
    --------------------------------
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

#ifndef KADASGPXWAYPOINTANNOTATIONITEM_H
#define KADASGPXWAYPOINTANNOTATIONITEM_H

#include <QColor>
#include <QFont>
#include <QString>

#include <qgis/qgsannotationmarkeritem.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal annotation item representing a GPX waypoint.
 *
 * Subclasses \c QgsAnnotationMarkerItem and pre-installs a yellow circle
 * simple-marker symbol matching the legacy \c KadasGpxWaypointItem default
 * style. Adds \c name, \c labelFont and \c labelColor fields and renders
 * the name as a buffered label next to the marker.
 *
 * Type id: \c "kadas:gpxwaypoint".
 */
class KADAS_GUI_EXPORT KadasGpxWaypointAnnotationItem : public QgsAnnotationMarkerItem
{
  public:
    KadasGpxWaypointAnnotationItem( const QgsPoint &point = QgsPoint() );

    static QString itemTypeId() { return QStringLiteral( "kadas:gpxwaypoint" ); }

    QString type() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasGpxWaypointAnnotationItem *clone() const override;

    static KadasGpxWaypointAnnotationItem *create();

    QString name() const { return mName; }
    void setName( const QString &name ) { mName = name; }

    QFont labelFont() const { return mLabelFont; }
    void setLabelFont( const QFont &font ) { mLabelFont = font; }

    QColor labelColor() const { return mLabelColor; }
    void setLabelColor( const QColor &color ) { mLabelColor = color; }

  private:
    QString mName;
    QFont mLabelFont;
    QColor mLabelColor;

    void installDefaultSymbol();
};

#endif // KADASGPXWAYPOINTANNOTATIONITEM_H
