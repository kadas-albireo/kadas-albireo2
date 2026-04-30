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

#include <memory>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

class QgsFillSymbol;

/**
 * \ingroup gui
 * \brief Kadas-internal circle annotation item.
 *
 * Defined by a center point and a ring point (a point on the circumference);
 * radius is the distance between the two. Both points are stored in the parent
 * layer's CRS. The item renders as a 64-segment polygon approximation using a
 * \c QgsFillSymbol.
 *
 * Type id: \c "kadas:circle".
 *
 * Multi-part and geodesic mode are intentionally not supported in v1; they
 * will return as additional Kadas-internal items if needed.
 */
class KADAS_GUI_EXPORT KadasCircleAnnotationItem : public QgsAnnotationItem
{
  public:
    KadasCircleAnnotationItem( const QgsPointXY &center = QgsPointXY(), const QgsPointXY &ringPoint = QgsPointXY() );
    ~KadasCircleAnnotationItem() override;

    static QString itemTypeId() { return QStringLiteral( "kadas:circle" ); }

    QString type() const override;
    Qgis::AnnotationItemFlags flags() const override;
    QgsRectangle boundingBox() const override;
    QgsRectangle boundingBox( QgsRenderContext &context ) const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    QList<QgsAnnotationItemNode> nodesV2( const QgsAnnotationItemEditContext &context ) const override;
    KadasCircleAnnotationItem *clone() const override;

    static KadasCircleAnnotationItem *create();

    QgsPointXY center() const { return mCenter; }
    void setCenter( const QgsPointXY &center ) { mCenter = center; }

    //! Returns a point on the circumference; radius is the distance to center().
    QgsPointXY ringPoint() const { return mRingPoint; }
    void setRingPoint( const QgsPointXY &ringPoint ) { mRingPoint = ringPoint; }

    //! Returns the (planar) radius in CRS units.
    double radius() const;

    const QgsFillSymbol *symbol() const;
    //! Sets the fill symbol used to render the circle. Takes ownership.
    void setSymbol( QgsFillSymbol *symbol );

  private:
    QgsPointXY mCenter;
    QgsPointXY mRingPoint;
    std::unique_ptr<QgsFillSymbol> mSymbol;
};

#endif // KADASCIRCLEANNOTATIONITEM_H
