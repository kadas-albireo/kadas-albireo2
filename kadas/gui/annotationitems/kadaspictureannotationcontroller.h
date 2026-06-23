/***************************************************************************
    kadaspictureannotationcontroller.h
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

#ifndef KADASPICTUREANNOTATIONCONTROLLER_H
#define KADASPICTUREANNOTATIONCONTROLLER_H

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

class QgsAnnotationPictureItem;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationPictureItem (type id \c "picture").
 */
class KADAS_GUI_EXPORT KadasPictureAnnotationController : public KadasAnnotationItemController
{
  public:
    KadasPictureAnnotationController() = default;

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

    QgsGeometry representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const override;

    //! Re-render the layer live while dragging (the outline band is a poor stand-in for the image).
    bool liveRepaintOnEdit() const override { return true; }

    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

    void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx ) override;

#ifndef SIP_RUN
    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;
#endif

    static void setPath( QgsAnnotationPictureItem *item, const QString &path );

    /**
     * Idempotently installs a balloon callout on \a pic (white fill, 1px
     * black stroke, 4px margins, 6px wedge); a no-op if already present.
     */
    static void ensureBalloon( QgsAnnotationPictureItem *pic );

    //! Next zIndex so a freshly created picture stacks above existing pictures in \a layer.
    static int nextPictureZIndex( const QgsAnnotationLayer *layer );

    /**
     * Returns whether the picture's balloon callout is visible. Hidden is
     * encoded as transparent fill+stroke and zero wedge width.
     */
    static bool isCalloutVisible( const QgsAnnotationPictureItem *pic );

    //! Shows or hides the balloon callout; hiding re-centers the picture on its former anchor.
    static void setCalloutVisible( QgsAnnotationPictureItem *pic, bool visible );

    //! Session-wide preference: should corner-resize preserve the picture's aspect ratio?
    static bool lockAspectRatio();
    static void setLockAspectRatio( bool on );

  private:
    enum AttribIds
    {
      AttrX,
      AttrY
    };
};

#endif // KADASPICTUREANNOTATIONCONTROLLER_H
