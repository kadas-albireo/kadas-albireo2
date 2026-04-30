/***************************************************************************
    kadasrectangleannotationcontroller.h
    ------------------------------------
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

#ifndef KADASRECTANGLEANNOTATIONCONTROLLER_H
#define KADASRECTANGLEANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasRectangleAnnotationItem (type id \c "kadas:rectangle").
 *
 * Two-click drawing UX: first click sets the center, second click sets the
 * opposite corner (interpreted as half-extents in the current rotated frame;
 * during draw the angle stays at zero so the user gets an axis-aligned
 * rectangle, rotation can be applied later via the rotation handle).
 *
 * Edit handles: 4 corner vertices (resize) + a rotation handle exposed as
 * vertex index 4. A whole-item move is offered when no handle is hit.
 */
class KADAS_GUI_EXPORT KadasRectangleAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasRectangleAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    QList<KadasMapItem::Node> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;

    bool startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx ) override;
    bool startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    void setCurrentPoint( QgsAnnotationItem *item, const KadasMapPos &p, const KadasAnnotationItemContext &ctx ) override;
    void setCurrentAttributes( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    bool continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) override;
    void endPart( QgsAnnotationItem *item ) override;

    KadasMapItem::AttribDefs drawAttribs() const override;
    KadasMapItem::AttribValues drawAttribsFromPosition( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const override;
    KadasMapPos positionFromDrawAttribs( const QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) const override;

    KadasMapItem::EditContext getEditContext( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const override;
    void edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx ) override;
    void edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) override;
    KadasMapItem::AttribValues editAttribsFromPosition(
      const QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx
    ) const override;
    KadasMapPos positionFromEditAttribs(
      const QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
    ) const override;

    KadasItemPos position( const QgsAnnotationItem *item ) const override;
    void setPosition( QgsAnnotationItem *item, const KadasItemPos &pos ) override;
    void translate( QgsAnnotationItem *item, double dx, double dy ) override;

    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;

  private:
    enum AttribIds
    {
      AttrX,
      AttrY
    };

    //! Vertex 4 is the rotation handle; vertices 0..3 are the corners.
    static constexpr int RotationHandleVertex = 4;
};

#endif // KADASRECTANGLEANNOTATIONCONTROLLER_H
