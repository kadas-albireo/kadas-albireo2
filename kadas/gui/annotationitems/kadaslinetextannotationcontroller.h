/***************************************************************************
    kadaslinetextannotationcontroller.h
    -----------------------------------
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

#ifndef KADASLINETEXTANNOTATIONCONTROLLER_H
#define KADASLINETEXTANNOTATIONCONTROLLER_H

#include <qgis/qgspointxy.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationrotation.h"

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationLineTextItem (type id \c "linetext").
 *
 * Digitizes a polyline like the plain line tool, but renders text along the
 * line geometry instead of drawing a stroke.
 */
class KADAS_GUI_EXPORT KadasLineTextAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasLineTextAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    bool isEmpty( const QgsAnnotationItem *item ) const override;

    QList<KadasNode> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;

#ifndef SIP_RUN
    //! The item renders only text along the line; expose the underlying polyline
    //! as a dashed guide so the line is visible (and pickable) while editing.
    QList<QList<QgsPointXY>> editGuide( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;
#endif

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

    //! The line-text item is not a QgsAnnotationLineItem, so the base fallback would
    //! preview its bounding box; return the actual polyline for the digitizing band.
    QgsGeometry representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;

    //! Re-render the layer live while dragging (the outline band is a poor stand-in for the text).
    bool liveRepaintOnEdit() const override { return true; }

    void applyPersistedStyle( QgsAnnotationItem *item ) const override;
    void persistStyle( const QgsAnnotationItem *item ) const override;
    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

#ifndef SIP_RUN
    static const QgsSettingsEntryDouble *settingsSize;
    static const QgsSettingsEntryColor *settingsColor;
    static const QgsSettingsEntryColor *settingsBufferColor;
    static const QgsSettingsEntryDouble *settingsBufferWidth;
#endif

  private:
    enum AttribIds
    {
      AttrX,
      AttrY,
      AttrAngle
    };

    // Per-drag rotation state, captured when the rotation handle is grabbed.
    // Shared with the line/polygon controllers so rotation behaves identically.
    mutable KadasAnnotationRotation::VertexRotationState mRotation;
};

#endif // KADASLINETEXTANNOTATIONCONTROLLER_H
