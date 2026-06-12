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
#include <QStringList>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/kadasattributetypes.h"

class QMenu;
class QWidget;
class QgsAnnotationItem;
class QgsCoordinateReferenceSystem;
class QgsGeometry;
class QgsRectangle;
class QgsRenderContext;
class QgsSettingsEntryColor;
class QgsSettingsEntryDouble;
class QgsSettingsEntryInteger;
class QuaZip;
class KadasAnnotationStyleEditor;

#ifndef SIP_RUN
/**
 * \ingroup gui
 * \brief A single on-canvas measurement label (e.g. segment length, total
 *        area) emitted by an annotation item controller.
 *
 * Restored from the Kadas 2.3 \c KadasGeometryItem on-canvas measurement
 * labels: lines emit one label per segment plus a total at the trailing
 * vertex; polygons emit a single area label at the centroid.
 *
 * \a mapPos is in the map canvas CRS. \a centered controls whether the
 * label is drawn centered on \a mapPos (segment midpoints, polygon
 * centroid) or offset below it (line totals at the trailing vertex).
 */
struct KadasAnnotationMeasurementLabel
{
    QgsPointXY mapPos;
    QString text;
    bool centered = true;
};
#endif

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

#ifndef SIP_RUN
    //! Settings tree node shared by all canonical controllers for their
    //! persisted last-used style entries (kadas/annotation/...).
    static inline QgsSettingsTreeNode *sTreeAnnotation = KadasSettingsTree::sTreeKadas->createChildNode( QStringLiteral( "annotation" ) );
#endif

    // ----- Identity --------------------------------------------------------

    //! QgsAnnotationItem type id this controller handles (e.g. "marker", "kadas:circle").
    virtual QString itemType() const = 0;

    //! Human-readable name for the item kind (used by UI; tr-able).
    virtual QString itemName() const = 0;

    // ----- Factory ---------------------------------------------------------

    //! Creates a fresh, blank annotation item ready to be driven through the draw
    //! state machine. Caller takes ownership.
    virtual QgsAnnotationItem *createItem() const = 0;

    // ----- Edit nodes (snap & vertex handles) ------------------------------

    virtual QList<KadasNode> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const = 0;

    // ----- Draw state machine ---------------------------------------------

    //! Begins a new part at \a firstPoint. Returns true if the item is ready to receive subsequent points.
    virtual bool startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx ) = 0;
    virtual bool startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual bool continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void endPart( QgsAnnotationItem *item ) = 0;

    virtual KadasAttribDefs drawAttribs() const = 0;
    virtual KadasAttribValues drawAttribsFromPosition( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual QgsPointXY positionFromDrawAttribs( const QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const = 0;

    // ----- Edit interface -------------------------------------------------

    virtual KadasEditContext getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual void edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx ) = 0;
    virtual void edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) = 0;
    virtual KadasAttribValues editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual QgsPointXY positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const = 0;
    virtual void populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx );
    virtual void onDoubleClick( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx );

    // ----- Digitizing / drag preview geometry ------------------------------

    /**
     * Returns a cheap geometry (in the item's layer CRS) representing the
     * item's current shape, used by the edit/create map tool as a
     * QgsRubberBand preview drawn above all layers while the item is being
     * digitized or dragged. Mirrors QGIS's
     * QgsAnnotationItemEditOperationTransientResults::representativeGeometry().
     *
     * Default implementation: the real geometry for line / polygon backed
     * items, the bounding-box outline otherwise.
     */
    virtual QgsGeometry representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const;

    /**
     * When TRUE, the edit map tool re-renders the annotation layer on every
     * drag step instead of only on release. The default (FALSE) matches the
     * QGIS behavior where the rubber-band outline is the only live feedback
     * and the layer keeps showing the pre-drag rendering. Controllers whose
     * items cannot be meaningfully previewed by an outline band (e.g.
     * pictures — the user expects the image itself to follow the cursor)
     * should return TRUE.
     */
    virtual bool liveRepaintOnEdit() const { return false; }

    // ----- Position helpers ----------------------------------------------

    virtual QgsPointXY position( const QgsAnnotationItem *item ) const = 0;
    virtual void setPosition( QgsAnnotationItem *item, const QgsPointXY &pos ) = 0;
    virtual void translate( QgsAnnotationItem *item, double dx, double dy ) = 0;

    // ----- Hit testing ----------------------------------------------------

    virtual bool hitTest( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const;
#ifndef SIP_RUN
    virtual QPair<QgsPointXY, double> closestPoint( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const;
#endif
    virtual bool intersects( const QgsAnnotationItem *item, const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx, bool contains = false ) const;

    // ----- On-canvas measurement labels ------------------------------------
    //
    // Restored from Kadas 2.3 (legacy KadasGeometryItem). The edit/create
    // map tool overlays these labels on the canvas while the item is being
    // traced or while it is selected for editing. Default: empty list.
    // Concrete controllers (line, polygon, ...) override.
#ifndef SIP_RUN
    virtual QList<KadasAnnotationMeasurementLabel> measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ctx );
      return {};
    }
#endif

    // ----- KML export ----------------------------------------------------

#ifndef SIP_RUN
    virtual QString asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip = nullptr ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( itemCrs );
      Q_UNUSED( renderContext );
      Q_UNUSED( kmzZip );
      return QString();
    }
#endif

    // ----- Persisted last-used style --------------------------------------
    //
    // Hooks for the styling row in KadasMapToolEditAnnotationItem to feed the
    // user's last-used style back into a freshly created item. Only the
    // canonical generic controllers (marker / linestring / polygon /
    // pointtext) override these; specialized controllers (rectangle,
    // circle, coord-cross, pin, gpx, milx, picture, ...) inherit the no-op
    // default so their controller-supplied symbol stays untouched.

    //! Applies the persisted defaults stored via QgsSettingsEntry to \a item.
    //! Called once on creation, before the item is added to the layer.
    virtual void applyPersistedStyle( QgsAnnotationItem *item ) const { Q_UNUSED( item ); }

    //! Stores the current style of \a item back into QgsSettingsEntry as the
    //! new defaults. Called whenever the user edits the styling row.
    virtual void persistStyle( const QgsAnnotationItem *item ) const { Q_UNUSED( item ); }

    //! Returns a freshly constructed style editor widget for this item type,
    //! parented to \a parent and owned by the caller. Default returns
    //! \c nullptr — specialized controllers (rectangle, circle, coord-cross,
    //! pin, gpx, milx, picture, ...) inherit this and contribute no styling
    //! row. Only the canonical generic controllers (marker / linestring /
    //! polygon / pointtext) override it.
    virtual KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const
    {
      Q_UNUSED( parent );
      return nullptr;
    }

    // ----- QGIS-compat shadows --------------------------------------------
    //
    // Kadas-specific item types (kadas:rectangle, kadas:circle, kadas:pin,
    // kadas:coordcross) are not registered in vanilla QGIS and would be
    // dropped on read. To keep them visible when the same project is
    // opened in QGIS, controllers can emit parallel "shadow" items in
    // stock QGIS types (polygon / marker / pointtext / linestring).
    // Shadows are added to the layer immediately before save and removed
    // immediately after; on load, shadows present in the layer XML are
    // also stripped (they were re-emitted from the master items by the
    // pre-save pass that wrote the project).
    //
    // Returns freshly allocated shadow items; caller takes ownership.
    // Default implementation returns an empty list.
    virtual QList<QgsAnnotationItem *> generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ctx );
      return {};
    }

    // Read/write the shadow id list stored on a master item. Controllers
    // whose master type carries shadow ids must override both. The
    // helpers in KadasAnnotationLayerHelpers dispatch through the
    // controller registry so adding a new master type only requires
    // overriding these (no central registry of dynamic_casts to keep in
    // sync).
    virtual QStringList shadowIds( const QgsAnnotationItem *item ) const
    {
      Q_UNUSED( item );
      return {};
    }
    virtual void setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ids );
    }

#ifndef SIP_RUN
    /**
     * Ratio of the render device's DPI to the user's primary screen DPI.
     *
     * Use this when scaling pixel-defined visual sizes (cross arms, label
     * fonts in points) inside an item's \c render() override so the
     * on-screen size stays constant across zoom levels and only scales up
     * for high-DPI export devices. On screen the value is ≈1.0; on a
     * 300 dpi print export it is ≈3.1.
     *
     * Mirrors the legacy \c KadasMapItem::outputDpiScale() helper from
     * Kadas 2.3. Prefer this over \c QgsRenderContext::scaleFactor()
     * which returns pixels-per-millimetre (~3.78 at 96 dpi) and would
     * make pixel-defined visuals visibly larger than their legacy size.
     *
     * Public so annotation \c QgsAnnotationItem subclasses (which do not
     * inherit from this controller class) can use it directly from their
     * \c render() override.
     */
    static double outputDpiScale( const QgsRenderContext &context );
#endif

  protected:
    // ----- Transform helpers (mirror KadasMapItem's) ---------------------

    static QgsPointXY toMapPos( const QgsPointXY &itemPos, const KadasAnnotationItemContext &ctx );
    static QgsPointXY toItemPos( const QgsPointXY &mapPos, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toItemRect( const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toMapRect( const QgsRectangle &itemRect, const KadasAnnotationItemContext &ctx );
    static double pickTolSqr( const KadasAnnotationItemContext &ctx );

#ifndef SIP_RUN
    // Formatting helpers for measurementLabels() overrides. Honor the
    // shared "/kadas/measure_decimals" setting (same one KadasMapToolMeasure
    // uses).
    static QString formatLengthMeters( double meters );
    static QString formatAreaSquareMeters( double sqMeters );
#endif
};

#endif // KADASANNOTATIONITEMCONTROLLER_H
