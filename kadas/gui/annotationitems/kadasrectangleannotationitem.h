/***************************************************************************
    kadasrectangleannotationitem.h
    ------------------------------
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

#ifndef KADASRECTANGLEANNOTATIONITEM_H
#define KADASRECTANGLEANNOTATIONITEM_H

#include <QSizeF>

#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas-internal rotated rectangle annotation item.
 *
 * Subclasses \c QgsAnnotationPolygonItem so the actual rendered geometry is
 * the rectangle's 4 corners as a polygon, but additionally stores the
 * higher-level rectangle parameters (\c center, \c size, \c angle in degrees
 * counter-clockwise) as first-class state. Mutating any rect parameter
 * regenerates the polygon geometry; reading the polygon back is therefore
 * always exact.
 *
 * Type id: \c "kadas:rectangle".
 *
 * Coordinates and size are in the parent layer's CRS.
 */
class KADAS_GUI_EXPORT KadasRectangleAnnotationItem : public QgsAnnotationPolygonItem
{
  public:
    KadasRectangleAnnotationItem( const QgsPointXY &center = QgsPointXY(), const QSizeF &size = QSizeF( 0, 0 ), double angleDegrees = 0.0 );

    static QString itemTypeId() { return QStringLiteral( "kadas:rectangle" ); }

    QString type() const override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasRectangleAnnotationItem *clone() const override;

    static KadasRectangleAnnotationItem *create();

    QgsPointXY center() const { return mCenter; }
    QSizeF size() const { return mSize; }
    //! Angle in degrees, counter-clockwise around \c center().
    double angle() const { return mAngle; }

    //! Sets all rectangle parameters at once and regenerates the polygon geometry.
    void setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees );

    void setCenter( const QgsPointXY &center );
    void setSize( const QSizeF &size );
    void setAngle( double angleDegrees );

    //! Returns the four corners in CCW order: BL, BR, TR, TL (in the rotated frame).
    //! Index 0 corresponds to the polygon vertex 0 / closing vertex 4.
    QVector<QgsPointXY> corners() const;

    //! Returns the rotation handle position (the midpoint of the top edge,
    //! offset outward by a fraction of the height in CRS units).
    QgsPointXY rotationHandle() const;

  private:
    QgsPointXY mCenter;
    QSizeF mSize;
    double mAngle = 0.0;

    void rebuildGeometry();
};

#endif // KADASRECTANGLEANNOTATIONITEM_H
