/***************************************************************************
    kadaspointtextannotationcontroller.h
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

#ifndef KADASPOINTTEXTANNOTATIONCONTROLLER_H
#define KADASPOINTTEXTANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationPointTextItem (type id \c "pointtext").
 *
 * Replaces the interactive behavior previously on \c KadasTextItem.
 * Text content, font, color and rotation live on the
 * \c QgsAnnotationPointTextItem itself (via \c QgsTextFormat); the
 * controller only drives the click-to-place state machine and KML export.
 */
class KADAS_GUI_EXPORT KadasPointTextAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasPointTextAnnotationController() = default;

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
};

#endif // KADASPOINTTEXTANNOTATIONCONTROLLER_H
