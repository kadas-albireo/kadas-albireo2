/***************************************************************************
    kadascoordcrossannotationcontroller.h
    -------------------------------------
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

#ifndef KADASCOORDCROSSANNOTATIONCONTROLLER_H
#define KADASCOORDCROSSANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

//! Controller for KadasCoordCrossAnnotationItem (type id "kadas:coordcross").
class KADAS_GUI_EXPORT KadasCoordCrossAnnotationController : public KadasMarkerAnnotationController
{
  public:
    KadasCoordCrossAnnotationController() = default;

    QString itemType() const override;
    QString itemName() const override;
    QgsAnnotationItem *createItem() const override;

    bool startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx ) override;
    void setPosition( QgsAnnotationItem *item, const QgsPointXY &pos ) override;
    void edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx ) override;

    QList<QgsAnnotationItem *> generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;
    QStringList shadowIds( const QgsAnnotationItem *item ) const override;
    void setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const override;

    //! Fixed visual drawn in render(); no style editor.
    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override { return nullptr; }

    //! The cross is drawn in render() over a hidden symbol; never adopt or write the shared point style.
    void applyPersistedStyle( QgsAnnotationItem * ) const override {}
    void persistStyle( const QgsAnnotationItem * ) const override {}

  protected:
    //! A symmetric cross has no meaningful orientation, so no rotation handle.
    bool hasRotationHandle() const override { return false; }
};

#endif // KADASCOORDCROSSANNOTATIONCONTROLLER_H
