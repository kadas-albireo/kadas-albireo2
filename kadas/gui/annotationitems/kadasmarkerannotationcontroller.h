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

    void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx ) override;

    //! Re-render the layer live while dragging (the outline band is a poor stand-in for the symbol).
    bool liveRepaintOnEdit() const override { return true; }

#ifndef SIP_RUN
    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;
#endif

    void applyPersistedStyle( QgsAnnotationItem *item ) const override;
    void persistStyle( const QgsAnnotationItem *item ) const override;
    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

#ifndef SIP_RUN
    static const QgsSettingsEntryInteger *settingsShape;
    static const QgsSettingsEntryInteger *settingsSize;
    static const QgsSettingsEntryDouble *settingsStrokeWidth;
    static const QgsSettingsEntryColor *settingsFillColor;
    static const QgsSettingsEntryColor *settingsStrokeColor;
    static const QgsSettingsEntryInteger *settingsStrokeStyle;
#endif

  protected:
    //! Whether this marker type exposes a rotation handle (drawn node + hit-test). Symmetric markers like the coordinate cross have none.
    virtual bool hasRotationHandle() const { return true; }

  private:
    enum AttribIds
    {
      AttrX,
      AttrY,
      AttrAngle
    };

    //! Vertex 1 is the rotation handle; vertex 0 is the anchor point.
    static constexpr int RotationHandleVertex = 1;
};

#endif // KADASMARKERANNOTATIONCONTROLLER_H
