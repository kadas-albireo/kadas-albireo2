/***************************************************************************
    kadasmilxannotationcontroller.h
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

#ifndef KADASMILXANNOTATIONCONTROLLER_H
#define KADASMILXANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationrotation.h"

/**
 * \ingroup gui
 * \brief Controller for \c KadasMilxAnnotationItem (type id \c "kadas:milx").
 */
class KADAS_GUI_EXPORT KadasMilxAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasMilxAnnotationController() = default;

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

    //! Re-render the layer live while dragging (the outline band is a poor stand-in for the symbol).
    bool liveRepaintOnEdit() const override { return true; }

    //! Preview the real symbol while drawing: a rubber band through the control points shows nothing of an MSS graphic.
    bool symbolPreviewWhileDrawing( const QgsAnnotationItem *item ) const override;

    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

    bool hitTest( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const override;
    void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx ) override;
    void onDoubleClick( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) override;

#ifndef SIP_RUN
    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;
#endif

  private:
    // Position attribute ids for the whole-symbol X/Y inputs. They are negative
    // so they never collide with the non-negative libmss KadasMilxAttrType ids,
    // which double as KadasAttribDefs/KadasAttribValues keys for the per-symbol
    // attribute control points (see getEditContext ring 1).
    enum AttribIds
    {
      AttrAngle = -3,
      AttrX = -2,
      AttrY = -1,
    };

    // vidx.part sentinel for the rotation handle; real vertices live in part 0,
    // rings 0 (geometry) and 1 (attribute control points).
    static constexpr int kPartRotate = 1;

    //! Rest position of the rotation handle (map CRS), or an invalid point when \a item cannot be rotated.
    QgsPointXY rotationHandle( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const;

    //! Map position a single point symbol rotates about: its anchor, shifted by the user offset.
    static QgsPointXY singlePointPivot( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx );

    //! Map position a multi point symbol rotates about: the mean of its control points.
    static QgsPointXY multiPointCenter( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx );

    //! Rotates a multi point symbol's control and attribute points to \a rotated (map CRS); sizes must match.
    static void applyRotatedPoints( QgsAnnotationItem *item, const QVector<QgsPointXY> &rotated, const KadasAnnotationItemContext &ctx );

    //! Geometry and attribute points of a multi point symbol in map CRS, in the order applyRotatedPoints() expects.
    static QVector<QgsPointXY> rotationSnapshot( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx );

    // Per-drag rotation state of a multi point symbol, captured when the handle is
    // grabbed. Single point symbols rotate their graphic instead and need none.
    mutable KadasAnnotationRotation::VertexRotationState mRotation;
};

#endif // KADASMILXANNOTATIONCONTROLLER_H
