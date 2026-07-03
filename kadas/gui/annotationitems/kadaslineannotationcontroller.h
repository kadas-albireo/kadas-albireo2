/***************************************************************************
    kadaslineannotationcontroller.h
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

#ifndef KADASLINEANNOTATIONCONTROLLER_H
#define KADASLINEANNOTATIONCONTROLLER_H

#include <QVector>

#include <qgis/qgspointxy.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationLineItem (type id \c "linestring").
 */
class KADAS_GUI_EXPORT KadasLineAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasLineAnnotationController() = default;

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

#ifndef SIP_RUN
    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;

    QList<KadasAnnotationMeasurementLabel> measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;
#endif

    void applyPersistedStyle( QgsAnnotationItem *item ) const override;
    void persistStyle( const QgsAnnotationItem *item ) const override;
    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

#ifndef SIP_RUN
    static const QgsSettingsEntryDouble *settingsWidth;
    static const QgsSettingsEntryColor *settingsColor;
    static const QgsSettingsEntryInteger *settingsStyle;
#endif

  private:
    enum AttribIds
    {
      AttrX,
      AttrY,
      AttrAngle
    };

    // Per-drag rotation state, captured when the rotation handle is grabbed:
    // the line vertices in map coords, the fixed pivot, and the handle's
    // reference angle. Lets edit() rotate the original (no drift).
    mutable QVector<QgsPointXY> mRotateOrigMap;
    mutable QgsPointXY mRotateCenterMap;
    mutable double mRotateRefAngle = 0.0;
    // While a rotation drag is active, the handle is drawn at this angle so it
    // tracks the cursor instead of snapping back above the bounding box centre.
    mutable bool mRotateActive = false;
    mutable double mRotateCurrentAngle = 0.0;
};

#endif // KADASLINEANNOTATIONCONTROLLER_H
