/***************************************************************************
    kadasannotationitemcontroller.h
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

#ifndef KADASANNOTATIONITEMCONTROLLER_H
#define KADASANNOTATIONITEMCONTROLLER_H

#include <QString>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/mapitems/kadasmapitem.h"

class QMenu;
class QgsAnnotationItem;
class QgsCoordinateReferenceSystem;
class QgsRectangle;
class QgsRenderContext;
class QuaZip;

/**
 * \ingroup gui
 * \brief Per-type controller for QgsAnnotationItem instances managed by Kadas.
 *
 * Concrete controllers encapsulate the interactive (draw / edit / snap / KML / editor)
 * behavior that previously lived on \c KadasMapItem subclasses.  One controller is
 * registered per QgsAnnotationItem type id (see \c QgsAnnotationItem::type()) in the
 * \c KadasAnnotationControllerRegistry; map tools look the controller up by type id
 * and delegate to it, so a tool never needs to know about concrete annotation
 * subclasses.
 *
 * \note Coordinates passed to draw/edit hooks are in the map CRS unless otherwise
 *       noted; numeric attribute distances are in map units. Methods that perform
 *       map ↔ item space transforms receive a \c KadasAnnotationItemContext bundling
 *       the item's CRS and the map settings.
 */
class KADAS_GUI_EXPORT KadasAnnotationItemController
{
  public:
    virtual ~KadasAnnotationItemController() = default;

    // ----- Identity --------------------------------------------------------

    //! QgsAnnotationItem type id this controller handles (e.g. "marker", "kadas:circle").
    virtual QString itemType() const = 0;

    //! Human-readable name for the item kind (used by UI; tr-able).
    virtual QString itemName() const = 0;

    // ----- Factory ---------------------------------------------------------

    //! Creates a fresh, blank annotation item ready to be driven through the draw
    //! state machine. Caller takes ownership.
    virtual QgsAnnotationItem *createItem() const = 0;

    // ----- Metadata --------------------------------------------------------

    //! Returns the type-level default editor widget name for items of this kind.
    //! May be empty if no editor is associated. Per-instance overrides are stored
    //! by \c KadasAnnotationLayerHelpers on the owning QgsAnnotationLayer.
    virtual QString defaultEditorName() const { return QString(); }

    // ----- Edit nodes (snap & vertex handles) ------------------------------

    virtual QList<KadasMapItem::Node> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const = 0;

    // ----- Draw state machine ---------------------------------------------

    //! Begins a new part at \a firstPoint. Returns true if the item is ready to receive subsequent points.
    virtual bool startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx ) = 0;
    virtual bool startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void setCurrentPoint( QgsAnnotationItem *item, const KadasMapPos &p, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void setCurrentAttributes( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual bool continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void endPart( QgsAnnotationItem *item ) = 0;

    virtual KadasMapItem::AttribDefs drawAttribs() const = 0;
    virtual KadasMapItem::AttribValues drawAttribsFromPosition( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual KadasMapPos positionFromDrawAttribs( const QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) const = 0;

    // ----- Edit interface -------------------------------------------------

    virtual KadasMapItem::EditContext getEditContext( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual void edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual KadasMapItem::AttribValues editAttribsFromPosition(
      const QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx
    ) const = 0;
    virtual KadasMapPos positionFromEditAttribs(
      const QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
    ) const = 0;
    virtual void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasMapItem::EditContext &editContext, const KadasMapPos &clickPos, const KadasAnnotationItemContext &ctx );
    virtual void onDoubleClick( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx );

    // ----- Position helpers ----------------------------------------------

    virtual KadasItemPos position( const QgsAnnotationItem *item ) const = 0;
    virtual void setPosition( QgsAnnotationItem *item, const KadasItemPos &pos ) = 0;
    virtual void translate( QgsAnnotationItem *item, double dx, double dy ) = 0;

    // ----- Hit testing ----------------------------------------------------

    virtual bool hitTest( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const;
    virtual QPair<KadasMapPos, double> closestPoint( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const;
    virtual bool intersects( const QgsAnnotationItem *item, const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx, bool contains = false ) const;

    // ----- KML export ----------------------------------------------------

    virtual QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const = 0;

  protected:
    // ----- Transform helpers (mirror KadasMapItem's) ---------------------

    static KadasMapPos toMapPos( const KadasItemPos &itemPos, const KadasAnnotationItemContext &ctx );
    static QgsPointXY toMapPos( const QgsPointXY &itemPos, const KadasAnnotationItemContext &ctx );
    static KadasItemPos toItemPos( const KadasMapPos &mapPos, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toItemRect( const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toMapRect( const QgsRectangle &itemRect, const KadasAnnotationItemContext &ctx );
    static double pickTolSqr( const KadasAnnotationItemContext &ctx );
};

#endif // KADASANNOTATIONITEMCONTROLLER_H
