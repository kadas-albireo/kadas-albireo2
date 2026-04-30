/***************************************************************************
    kadascircleannotationitem.h
    ---------------------------
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

#ifndef KADASCIRCLEANNOTATIONITEM_H
#define KADASCIRCLEANNOTATIONITEM_H

#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal circle annotation item.
 *
 * Subclasses \c QgsAnnotationPolygonItem so the rendered geometry is a
 * \c QgsCurvePolygon whose exterior ring is a \c QgsCompoundCurve made of two
 * \c QgsCircularString arcs (top + bottom semicircle). The geometry is
 * therefore an exact circle — no segmentation. Rendering, callouts, fill
 * symbology and node-based editing are inherited from the parent class.
 *
 * Canonical state is \c center plus \c ringPoint (a point on the
 * circumference); both are stored in the parent layer's CRS, and any
 * mutation rebuilds the curve polygon from these parameters. \c writeXml
 * stamps \c cx/cy/rx/ry alongside the parent's WKT serialization, but
 * \c readXml reads only the canonical params and rebuilds the geometry, so
 * the two representations cannot drift.
 *
 * Type id: \c "kadas:circle".
 */
class KADAS_GUI_EXPORT KadasCircleAnnotationItem : public QgsAnnotationPolygonItem
{
  public:
    KadasCircleAnnotationItem( const QgsPointXY &center = QgsPointXY(), const QgsPointXY &ringPoint = QgsPointXY() );

    static QString itemTypeId() { return QStringLiteral( "kadas:circle" ); }

    QString type() const override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasCircleAnnotationItem *clone() const override;

    static KadasCircleAnnotationItem *create();

    QgsPointXY center() const { return mCenter; }
    //! Returns a point on the circumference; the radius is the distance to \c center().
    QgsPointXY ringPoint() const { return mRingPoint; }

    void setCenter( const QgsPointXY &center );
    void setRingPoint( const QgsPointXY &ringPoint );
    void setCircle( const QgsPointXY &center, const QgsPointXY &ringPoint );

    //! Returns the (planar) radius in CRS units.
    double radius() const;

  private:
    QgsPointXY mCenter;
    QgsPointXY mRingPoint;

    void rebuildGeometry();
};

#endif // KADASCIRCLEANNOTATIONITEM_H
