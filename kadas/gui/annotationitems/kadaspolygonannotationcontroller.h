/***************************************************************************
    kadaspolygonannotationcontroller.h
    ----------------------------------
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

#ifndef KADASPOLYGONANNOTATIONCONTROLLER_H
#define KADASPOLYGONANNOTATIONCONTROLLER_H

#include <qgis/qgspointxy.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationrotation.h"
#include "kadas/gui/annotationitems/kadasannotationvertexedit.h"

class QgsCurve;
class QgsCurvePolygon;
class QMenu;

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationPolygonItem (type id \c "polygon").
 */
class KADAS_GUI_EXPORT KadasPolygonAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasPolygonAnnotationController() = default;

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
    void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx ) override;

    QgsPointXY position( const QgsAnnotationItem *item ) const override;
    void setPosition( QgsAnnotationItem *item, const QgsPointXY &pos ) override;
    void translate( QgsAnnotationItem *item, double dx, double dy ) override;

#ifndef SIP_RUN
    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;

    QList<KadasAnnotationMeasurementLabel> measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;
#endif

    void applyPersistedStyle( QgsAnnotationItem *item ) const override;
    void persistStyle( const QgsAnnotationItem *item ) const override;
    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

#ifndef SIP_RUN
    static const QgsSettingsEntryDouble *settingsStrokeWidth;
    static const QgsSettingsEntryColor *settingsFillColor;
    static const QgsSettingsEntryColor *settingsStrokeColor;
    static const QgsSettingsEntryInteger *settingsStrokeStyle;
    static const QgsSettingsEntryInteger *settingsBrushStyle;
#endif

  private:
    enum AttribIds
    {
      AttrX,
      AttrY,
      AttrAngle
    };

    // Per-drag rotation state, captured when the rotation handle is grabbed.
    // Shared with the line controller so rotation behaves identically.
    mutable KadasAnnotationRotation::VertexRotationState mRotation;

    // Rest position of the rotation handle: due north of the centroid but lifted
    // clear of the polygon's northmost extent so it never overlaps the shape.
    QgsPointXY restHandleMap( const QgsCurvePolygon *poly, const KadasAnnotationItemContext &ctx ) const;

    // Per-drag midpoint-handle state, armed when such a handle is grabbed.
    mutable KadasAnnotationVertexEdit::VertexInsertState mInsert;

    //! Number of vertices the ring carries once its closing duplicate is discounted.
    static int distinctVertexCount( const QgsCurve *ring );

    //! Number of segments that carry a midpoint handle, the closing one included; 1 for a two-vertex ring, whose two segments coincide.
    static int segmentCount( const QgsCurve *ring );

    //! Midpoint (map CRS) of the segment starting at vertex \a segment, or an invalid point when the segment does not exist.
    static QgsPointXY segmentMidpointMap( const QgsCurve *ring, int segment, const KadasAnnotationItemContext &ctx );

    //! Removes vertex \a vertex, re-closing the ring; a no-op when it would leave fewer than three vertices behind.
    static void deleteVertex( QgsAnnotationItem *item, int vertex );
};

#endif // KADASPOLYGONANNOTATIONCONTROLLER_H
