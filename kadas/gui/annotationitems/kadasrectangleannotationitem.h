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

//! Rotated rectangle annotation item (type id "kadas:rectangle").
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
    double angle() const { return mAngle; }

    //! CRS in which size and angle are expressed (invalid = legacy).
    QgsCoordinateReferenceSystem drawCrs() const { return mDrawCrs; }

    //! Parent layer CRS, used to project corners from drawCrs (invalid = legacy).
    QgsCoordinateReferenceSystem layerCrs() const { return mLayerCrs; }

    //! Legacy overload: axis-aligned layout in the layer CRS.
    void setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees );

    void setBox( const QgsPointXY &center, const QSizeF &size, double angleDegrees, const QgsCoordinateReferenceSystem &drawCrs, const QgsCoordinateReferenceSystem &layerCrs );

    void setCenter( const QgsPointXY &center );
    void setSize( const QSizeF &size );
    void setAngle( double angleDegrees );

    //! Corners in CCW order: BL, BR, TR, TL.
    QVector<QgsPointXY> corners() const;

    QgsPointXY rotationHandle() const;

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
