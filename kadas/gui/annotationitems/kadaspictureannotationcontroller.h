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

/**
 * \ingroup gui
 * \brief Controller for stock \c QgsAnnotationPictureItem (type id \c "picture").
 *
 * Drop-in replacement for \c KadasPictureItem. The geographic anchor lives
 * as the center of the item's \c bounds() (the item is configured with
 * \c Qgis::AnnotationPlacementMode::FixedSize at item creation time) and the
 * picture's offset from that anchor is stored as the base
 * \c QgsAnnotationItem::offsetFromCallout(). A simple-line callout is
 * auto-installed at item creation so the leader line from the anchor to the
 * picture is rendered for free by the parent layer.
 *
 * Single-click placement: the click sets the anchor (bounds center). The
 * picture defaults to a 200x150 px frame, which the user can resize via the
 * fixedSize handle.
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

    KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const override;

    void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx ) override;

    QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const override;

    //! Convenience: sets the picture's source path (auto-detects format from extension).
    static void setPath( QgsAnnotationPictureItem *item, const QString &path );

    /**
     * Idempotently installs a balloon callout on \a pic so it acts as a
     * cartoon speech-bubble: the picture frame is the bubble body, with a
     * wedge pointing back to the geographic anchor (bounds.center).
     *
     * Steps performed (each is skipped if already in place):
     *  - install a \c QgsBalloonCallout (white fill, 1px black stroke,
     *    4px margins, 6px wedge, 0 corner radius) when no callout exists
     *  - set \c calloutAnchor to \c bounds().center() when empty
     *  - set \c offsetFromCallout to \c -size/2 (centered-on-anchor,
     *    wedge has zero length) when the offset is invalid
     *
     * Centralized here because every picture creation site (controller,
     * \c KadasApplication::addImageItem, paste-from-clipboard,
     * \c KadasPictureItem / \c KadasSymbolItem migration) needs the same
     * callout layout. Calling this on a picture that already has one is
     * a no-op and never overwrites the user's customizations.
     */
    static void ensureBalloon( QgsAnnotationPictureItem *pic );

    /**
     * \brief Returns whether the picture's balloon callout is visible.
     *
     * The "Show callout" toggle in the style editor is encoded by setting
     * the balloon symbol to fully transparent fill+stroke and a zero
     * wedge width — the renderer still places the picture at
     * \c anchor+offset, but the balloon shape itself is invisible. Code
     * that depends on whether the callout is shown (e.g. picture-drag
     * behavior) checks this predicate rather than looking at
     * \c pic->callout() directly.
     */
    static bool isCalloutVisible( const QgsAnnotationPictureItem *pic );

    /**
     * \brief Session-wide preference: should corner-resize and the
     * style editor's width/height spinboxes preserve the picture's
     * aspect ratio?
     *
     * Stored as a static so the style editor (which owns the
     * checkbox) and the controller (which performs corner drags from
     * the map tool) agree without needing per-picture state. Default
     * is \c true (the safer default — free resize stretches images).
     */
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
