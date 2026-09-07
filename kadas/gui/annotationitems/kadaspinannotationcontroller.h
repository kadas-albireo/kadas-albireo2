/***************************************************************************
    kadaspinannotationcontroller.h
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

#ifndef KADASPINANNOTATIONCONTROLLER_H
#define KADASPINANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

class QgsMarkerSymbol;
class QgsCoordinateReferenceSystem;

//! Controller for KadasPinAnnotationItem (type id "kadas:pin").
class KADAS_GUI_EXPORT KadasPinAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasPinAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

    QString tooltip( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs ) const override;

    QList<QgsAnnotationItem *> generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;
    QStringList shadowIds( const QgsAnnotationItem *item ) const override;
    void setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const override;

  private:
    //! Inlines qrc SVG paths in \a symbol as base64, so a saved QGIS shadow is self-contained (vanilla QGIS cannot resolve Kadas qrc paths).
    static void embedQrcSvgPaths( QgsMarkerSymbol *symbol );

    //! TRUE if the project names a raster layer as its heightmap.
    static bool projectHasHeightmap();

    //! The tooltip's position/altitude band for a pin at \a pos, expressed in \a crs.
    static QString headerHtml( const QgsPointXY &pos, const QgsCoordinateReferenceSystem &crs );
};

#endif // KADASPINANNOTATIONCONTROLLER_H
