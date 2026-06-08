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
#include <QStringList>

#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/annotationitems/kadasannotationshadow.h"
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
 * \c center is always stored in the parent layer's CRS. \c size and
 * \c angle are expressed in the "drawing CRS" (\c drawCrs), which is the
 * map canvas CRS at the time the rectangle was drawn or last edited. When
 * \c drawCrs differs from the layer CRS, the polygon corners are produced
 * by laying the box out axis-aligned in \c drawCrs around the projected
 * center, then transforming each corner per-vertex back to the layer CRS.
 * This guarantees the rendered shape is a true rectangle on a map
 * displayed in \c drawCrs even when the layer CRS differs.
 *
 * If \c drawCrs is invalid (legacy items / programmatic creation without a
 * context) the box is laid out axis-aligned directly in the layer CRS,
 * preserving the earlier behavior.
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

    //! CRS in which \c size and \c angle are expressed; invalid for legacy
    //! items laid out axis-aligned in the layer CRS.
    QgsCoordinateReferenceSystem drawCrs() const { return mDrawCrs; }

    //! CRS of the parent layer. Used at corner-computation time to project
    //! per-vertex from \c drawCrs back to the layer CRS. Invalid for legacy
    //! items.
    QgsCoordinateReferenceSystem layerCrs() const { return mLayerCrs; }

    //! Sets all rectangle parameters at once and regenerates the polygon geometry.
    //! Legacy overload; lays the box out axis-aligned in the layer CRS.
    void setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees );

    //! Sets all rectangle parameters together with the drawing CRS used to
    //! interpret \c size and \c angle and the layer CRS used for the polygon
    //! geometry. Per-vertex transforms make the rendered polygon a true
    //! rectangle on a map shown in \c drawCrs even when \c layerCrs differs.
    void setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees, const QgsCoordinateReferenceSystem &drawCrs, const QgsCoordinateReferenceSystem &layerCrs );

    void setCenter( const QgsPointXY &center );
    void setSize( const QSizeF &size );
    void setAngle( double angleDegrees );

    //! Returns the four corners in CCW order: BL, BR, TR, TL (in the rotated frame).
    //! Index 0 corresponds to the polygon vertex 0 / closing vertex 4.
    QVector<QgsPointXY> corners() const;

    //! Returns the rotation handle position (the midpoint of the top edge,
    //! offset outward by a fraction of the height in CRS units).
    QgsPointXY rotationHandle() const;

    //! UUIDs of save-time QGIS-compat shadow items linked to this master.
    //! See \c KadasAnnotationShadow.
    const QStringList &shadowIds() const { return mShadow.ids(); }
    void setShadowIds( const QStringList &ids ) { mShadow.setIds( ids ); }

  private:
    QgsPointXY mCenter;
    QSizeF mSize;
    double mAngle = 0.0;
    QgsCoordinateReferenceSystem mDrawCrs;
    QgsCoordinateReferenceSystem mLayerCrs;
    KadasAnnotationShadow mShadow;

    void rebuildGeometry();
};

#endif // KADASRECTANGLEANNOTATIONITEM_H
