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

#include <QStringList>

#include "kadas/gui/annotationitems/kadasannotationshadow.h"
#include "kadas/gui/kadas_gui.h"

//! Circle annotation item (type id "kadas:circle").
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
    QgsPointXY ringPoint() const { return mRingPoint; }

    void setCenter( const QgsPointXY &center );
    void setRingPoint( const QgsPointXY &ringPoint );

    const QStringList &shadowIds() const { return mShadow.ids(); }
    void setShadowIds( const QStringList &ids ) { mShadow.setIds( ids ); }
    void setCircle( const QgsPointXY &center, const QgsPointXY &ringPoint );

    double radius() const;

  private:
    QgsPointXY mCenter;
    QgsPointXY mRingPoint;
    KadasAnnotationShadow mShadow;

    void rebuildGeometry();
};

#endif // KADASCIRCLEANNOTATIONITEM_H
