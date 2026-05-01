/***************************************************************************
    kadascoordcrossannotationitem.h
    -------------------------------
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

#ifndef KADASCOORDCROSSANNOTATIONITEM_H
#define KADASCOORDCROSSANNOTATIONITEM_H

#include <qgis/qgsannotationmarkeritem.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal "coordinate cross" annotation item.
 *
 * Renders a black-on-white cross at a point with the X/Y coordinates of
 * its position labelled in kilometres. Subclasses \c QgsAnnotationMarkerItem
 * with a transparent marker symbol so the underlying marker paint does
 * not draw anything; the cross and labels are drawn by the overridden
 * \c render().
 *
 * Coordinates are rounded to the nearest kilometre by the controller on
 * placement. The annotation layer is expected to use a metric CRS
 * (EPSG:3857 in standard Kadas layouts).
 *
 * Type id: \c "kadas:coordcross".
 */
class KADAS_GUI_EXPORT KadasCoordCrossAnnotationItem : public QgsAnnotationMarkerItem
{
  public:
    KadasCoordCrossAnnotationItem( const QgsPoint &point = QgsPoint() );

    static QString itemTypeId() { return QStringLiteral( "kadas:coordcross" ); }

    QString type() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    QgsRectangle boundingBox() const override;
    QgsRectangle boundingBox( QgsRenderContext &context ) const override;
    KadasCoordCrossAnnotationItem *clone() const override;

    static KadasCoordCrossAnnotationItem *create();

  private:
    static constexpr int sCrossSizePx = 80;
    static constexpr int sFontSizePx = 24;

    void installDefaultSymbol();
};

#endif // KADASCOORDCROSSANNOTATIONITEM_H
