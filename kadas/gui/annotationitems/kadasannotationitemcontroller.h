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

#include <QColor>
#include <QString>
#include <QStringList>
#include <optional>

#include <qgis/qgis.h>

#include "kadas/core/kadassettingstree.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/kadasattributetypes.h"

class QMenu;
class QWidget;
class QgsAbstractGeometry;
class QgsAnnotationItem;
class QgsAnnotationLayer;
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
 * \brief A single on-canvas measurement label emitted by an annotation item controller.
 */
struct KadasAnnotationMeasurementLabel
{
    /**
     * \ingroup gui
     * \brief Placement of a label that measures a segment.
     *
     * Such a label is laid along its segment rather than upright on top of it, and
     * pushed clear of it, so the number never hides the line it belongs to nor the
     * midpoint handle sitting there.
     */
    struct Segment
    {
        //! Segment ends, in map coordinates. The label runs parallel to them, in whichever of the two directions reads left to right.
        QgsPointXY start;
        QgsPointXY end;
        //! Map point the label is pushed away from - a point just inside the shape, so edge labels land outside it. Unset: the label goes on whichever side is up on screen.
        std::optional<QgsPointXY> away;
    };

    QgsPointXY mapPos;
    QString text;
    bool centered = true;
    //! Set when the label measures a segment; unset for a label that just marks a spot (a total, an area).
    std::optional<Segment> segment;
};

/**
 * \ingroup gui
 * \brief Styling for the annotation edit tool's drag-preview rubber band.
 *
 * Supplied by a controller via KadasAnnotationItemController::previewStyle() so the
 * map tool does not need to introspect QGIS symbol layers. \a width is expressed in
 * \a widthUnit; the tool converts it to canvas pixels and floors it to a minimum.
 */
struct KadasAnnotationPreviewStyle
{
    QColor strokeColor = QColor( 50, 50, 50, 200 );
    QColor fillColor = QColor( Qt::transparent );
    QColor secondaryColor = QColor( 255, 255, 255, 100 );
    Qt::BrushStyle brushStyle = Qt::SolidPattern;
    Qt::PenStyle lineStyle = Qt::SolidLine;
    double width = 0;
    Qgis::RenderUnit widthUnit = Qgis::RenderUnit::Millimeters;
};
#endif

/**
 * \ingroup gui
 * \brief Per-type controller for QgsAnnotationItem instances managed by Kadas.
 */
class KADAS_GUI_EXPORT KadasAnnotationItemController
{
  public:
    virtual ~KadasAnnotationItemController() = default;

#ifndef SIP_RUN
    //! Settings tree node for persisted last-used style entries (kadas/annotation/...).
    static inline QgsSettingsTreeNode *sTreeAnnotation = KadasSettingsTree::sTreeKadas->createChildNode( QStringLiteral( "annotation" ) );
#endif

    // ----- Identity --------------------------------------------------------

    //! QgsAnnotationItem type id this controller handles (e.g. "marker", "kadas:circle").
    virtual QString itemType() const = 0;

    //! Human-readable name for the item kind (used by UI; tr-able).
    virtual QString itemName() const = 0;

    // ----- Factory ---------------------------------------------------------

    //! Creates a fresh annotation item. Caller takes ownership.
    virtual QgsAnnotationItem *createItem() const = 0;

    // ----- Edit nodes (snap & vertex handles) ------------------------------

    virtual QList<KadasNode> nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const = 0;

#ifndef SIP_RUN
    //! Optional dashed guide polylines (map CRS) drawn under the edit handles for
    //! items whose own rendering does not reveal their geometry (e.g. text along a
    //! line). Each inner list is one polyline. Empty by default.
    virtual QList<QList<QgsPointXY>> editGuide( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ctx );
      return {};
    }
#endif

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

    //! Geometry (item CRS) for the edit tool's rubber-band preview. Mirrors QgsAnnotationItemEditOperationTransientResults::representativeGeometry().
    virtual QgsGeometry representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const;

#ifndef SIP_RUN
    //! Styling for the edit tool's rubber-band preview. The default reads the item's line/fill symbol; controllers may override for custom previews.
    virtual KadasAnnotationPreviewStyle previewStyle( const QgsAnnotationItem *item ) const;
#endif

    //! When TRUE, the edit tool re-renders the layer on every drag step (e.g. pictures) rather than only on release.
    virtual bool liveRepaintOnEdit() const { return false; }

    /**
     * When TRUE, the create tool previews the in-progress \a item by rendering its
     * real symbol rather than tracing its geometry with a rubber band. Worth it only
     * where the symbol adds something the geometry does not carry, such as a line's
     * arrow head; a rubber band is cheaper for everything else.
     */
    virtual bool symbolPreviewWhileDrawing( const QgsAnnotationItem *item ) const
    {
      Q_UNUSED( item );
      return false;
    }

    //! When TRUE, \a item carries no meaningful content yet (e.g. a text item with no text); the create tool discards such items if the user leaves them untouched.
    virtual bool isEmpty( const QgsAnnotationItem *item ) const
    {
      Q_UNUSED( item );
      return false;
    }

    // ----- Target layer ---------------------------------------------------

    /**
     * TRUE if items of this type can be drawn onto \a layer, which is what the
     * layer chooser of the create tool offers.
     *
     * The default accepts every annotation layer except the parametric Kadas
     * overlays (bullseye, map grid, ...), whose content is generated from layer
     * settings rather than drawn. Override to constrain the choice further.
     */
    virtual bool supportsLayer( const QgsAnnotationLayer *layer ) const;

    //! CRS a layer newly created for this item type carries; an invalid CRS (the default) means "follow the project".
    virtual QgsCoordinateReferenceSystem preferredLayerCrs() const;

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
#ifndef SIP_RUN
    virtual QList<KadasAnnotationMeasurementLabel> measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ctx );
      return {};
    }
#endif

    // ----- Tooltip --------------------------------------------------------

    /**
     * Rich-text tooltip for \a item, whose geometry is expressed in \a itemCrs.
     *
     * Recomputed on every hover, so it may include values that the item does not
     * own and that change under it (the coordinate display format, the project
     * heightmap). Returning an empty string means "no live tooltip": the caller
     * then falls back to the static text stored on the layer via
     * KadasAnnotationLayerHelpers::tooltip().
     */
    virtual QString tooltip( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( itemCrs );
      return QString();
    }

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

    //! Applies persisted style defaults to \a item on creation.
    virtual void applyPersistedStyle( QgsAnnotationItem *item ) const { Q_UNUSED( item ); }

    //! Stores \a item's current style as the new persisted defaults.
    virtual void persistStyle( const QgsAnnotationItem *item ) const { Q_UNUSED( item ); }

    //! Returns a style editor widget for this item type, or nullptr if none. Caller takes ownership.
    virtual KadasAnnotationStyleEditor *createStyleEditor( QWidget *parent = nullptr ) const
    {
      Q_UNUSED( parent );
      return nullptr;
    }

    // ----- QGIS-compat shadows --------------------------------------------

    //! Returns freshly allocated shadow items for QGIS compatibility; caller takes ownership.
    virtual QList<QgsAnnotationItem *> generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
    {
      Q_UNUSED( item );
      Q_UNUSED( ctx );
      return {};
    }

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
    //! Ratio of render device DPI to primary screen DPI, for scaling pixel-defined visuals in render() (≈1.0 on screen).
    static double outputDpiScale( const QgsRenderContext &context );
#endif

  protected:
    // ----- Transform helpers ---------------------

    static QgsPointXY toMapPos( const QgsPointXY &itemPos, const KadasAnnotationItemContext &ctx );
    static QgsPointXY toItemPos( const QgsPointXY &mapPos, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toItemRect( const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx );
    static QgsRectangle toMapRect( const QgsRectangle &itemRect, const KadasAnnotationItemContext &ctx );
    static double pickTolSqr( const KadasAnnotationItemContext &ctx );
    //! Squared pick tolerance for rotation handles; wider than pickTolSqr() to match the larger knob.
    static double rotationPickTolSqr( const KadasAnnotationItemContext &ctx );

#ifndef SIP_RUN
    //! Geometric centroid of \a geom (item CRS) expressed in map coords; the natural rotation pivot for vertex geometries.
    static QgsPointXY centroidMap( const QgsAbstractGeometry *geom, const KadasAnnotationItemContext &ctx );
#endif

#ifndef SIP_RUN
    // Honor the shared "/kadas/measure_decimals" setting.
    static QString formatLengthMeters( double meters );
    static QString formatAreaSquareMeters( double sqMeters );
#endif

#ifndef SIP_RUN
    // ----- Segment measurement labels --------------------------------------

    //! Side of a directed edge the shape's interior lies on, which is the side an edge label must avoid.
    enum class InteriorSide
    {
      None, //!< The edge belongs to no closed shape (an open line): the label picks the side that is up on screen.
      Left,
      Right,
    };

    //! Interior side of the closed ring through \a ring (item coordinates), from its winding; InteriorSide::None when it is degenerate.
    static InteriorSide ringInteriorSide( const QVector<QgsPointXY> &ring );

    //! Label \a text for the edge \a a to \a b (item coordinates), anchored on its midpoint, laid along it and pushed to the side opposite \a side.
    static KadasAnnotationMeasurementLabel segmentLabel( const QgsPointXY &a, const QgsPointXY &b, const QString &text, const KadasAnnotationItemContext &ctx, InteriorSide side = InteriorSide::None );
#endif
};

#endif // KADASANNOTATIONITEMCONTROLLER_H
