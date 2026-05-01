/***************************************************************************
    kadasmarkerannotationcontroller.h
    ---------------------------------
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

#ifndef KADASMARKERANNOTATIONCONTROLLER_H
#define KADASMARKERANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationMarkerItem (type id \c "marker").
 *
 * Drop-in replacement for \c KadasPointItem's interactive behavior. The
 * symbology of the marker (shape, fill, stroke) is stored directly on the
 * \c QgsAnnotationMarkerItem's \c QgsMarkerSymbol, so the controller does not
 * carry any styling state itself.
 */
class KADAS_GUI_EXPORT KadasMarkerAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasMarkerAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    QList<KadasNode> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;

    bool startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx ) override;
    bool startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    void setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx ) override;
    void setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    bool continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) override;
    void endPart( QgsAnnotationItem *item ) override;

    KadasAttribDefs drawAttribs() const override;
    KadasAttribValues drawAttribsFromPosition( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const override;
    QgsPointXY positionFromDrawAttribs( const QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const override;

    KadasEditContext getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const override;
    void edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx ) override;
    void edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    KadasAttribValues editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const override;
    QgsPointXY positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const override;

    QgsPointXY position( const QgsAnnotationItem *item ) const override;
    void setPosition( QgsAnnotationItem *item, const QgsPointXY &pos ) override;
    void translate( QgsAnnotationItem *item, double dx, double dy ) override;

    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;

  private:
    enum AttribIds
    {
      AttrX,
      AttrY
    };
};

#endif // KADASMARKERANNOTATIONCONTROLLER_H
