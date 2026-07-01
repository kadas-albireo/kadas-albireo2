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

#include <QStringList>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscoordinatereferencesystem.h>

#include "kadas/gui/annotationitems/kadasannotationshadow.h"
#include "kadas/gui/kadas_gui.h"

//! "Coordinate cross" annotation item (type id "kadas:coordcross").
class KADAS_GUI_EXPORT KadasCoordCrossAnnotationItem : public QgsAnnotationMarkerItem
{
  public:
    KadasCoordCrossAnnotationItem( const QgsPoint &point = QgsPoint() );

    static QString itemTypeId() { return QStringLiteral( "kadas:coordcross" ); }

    //! Snapping/labelling CRS: layerCrs if metric, else EPSG:3857 (degrees would round to 0/0).
    static QgsCoordinateReferenceSystem labelCrs( const QgsCoordinateReferenceSystem &layerCrs );

    QString type() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    QgsRectangle boundingBox() const override;
    QgsRectangle boundingBox( QgsRenderContext &context ) const override;
    KadasCoordCrossAnnotationItem *clone() const override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;

    static KadasCoordCrossAnnotationItem *create();

    const QStringList &shadowIds() const { return mShadow.ids(); }
    void setShadowIds( const QStringList &ids ) { mShadow.setIds( ids ); }

  private:
    static constexpr int sCrossSizePx = 80;
    static constexpr int sFontSizePx = 24;

    KadasAnnotationShadow mShadow;

    void installDefaultSymbol();
};

#endif // KADASCOORDCROSSANNOTATIONITEM_H
