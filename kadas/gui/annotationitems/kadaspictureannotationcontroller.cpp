/***************************************************************************
    kadaspictureannotationcontroller.cpp
    ------------------------------------
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

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QTextStream>

#include <qgis/qgis.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgscallout.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsmargins.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgssymbol.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"

namespace
{
  inline QgsAnnotationPictureItem *asPicture( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "picture" ) );
    return static_cast<QgsAnnotationPictureItem *>( item );
  }
  inline const QgsAnnotationPictureItem *asPicture( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "picture" ) );
    return static_cast<const QgsAnnotationPictureItem *>( item );
  }

  // KadasEditContext::vidx sentinels used by the picture controller to
  // distinguish what the user grabbed: the geographic anchor handle vs
  // the picture frame body. Stored in the part field of the QgsVertexId
  // so that other consumers (state history) treat them as opaque labels.
  constexpr int kPartAnchor = 0;
  constexpr int kPartFrame = 1;

  /**
   * Compute the picture's frame screen rectangle for the given map
   * settings. Returns an invalid rect if the placement is not FixedSize.
   *
   * IMPORTANT: QgsAnnotationRectItem interprets `offsetFromCallout` as
   * the **top-left** of the picture relative to the anchor (see
   * qgsannotationrectitem.cpp ~line 113:
   *   painterBounds = QRectF( anchor + offset, fixedSize )
   * ), NOT a center offset. The hit-test must match that geometry, or
   * clicks on the visible picture fall through to the anchor-handle
   * path and the user can never grab the frame body.
   */
  /// Returns the picture's geographic anchor in item-CRS coordinates.
  /// This is what the renderer uses to place the rect — the callout
  /// anchor geometry, NOT bounds.center. Falls back to bounds.center
  /// only if calloutAnchor is empty (defensive — startPart sets both).
  QgsPointXY pictureAnchorItem( const QgsAnnotationPictureItem *pic )
  {
    const QgsGeometry g = pic->calloutAnchor();
    if ( !g.isEmpty() )
    {
      const QPointF p = g.asQPointF();
      return QgsPointXY( p.x(), p.y() );
    }
    return pic->bounds().center();
  }

  QRectF frameScreenRect( const QgsAnnotationPictureItem *pic, const QgsMapSettings &ms, const QgsCoordinateTransform &xform )
  {
    if ( pic->placementMode() != Qgis::AnnotationPlacementMode::FixedSize )
      return QRectF();
    const QgsPointXY itemAnchor = pictureAnchorItem( pic );
    QgsPointXY mapAnchor = itemAnchor;
    try
    {
      if ( xform.isValid() )
        mapAnchor = xform.transform( itemAnchor );
    }
    catch ( const QgsCsException & )
    {
      return QRectF();
    }
    const QPointF anchorPx = ms.mapToPixel().transform( mapAnchor ).toQPointF();
    // The renderer (qgsannotationrectitem.cpp ~line 99,113) converts
    // both `fixedSize` and `offsetFromCallout` to painter units using
    // the item's *unit* (Pixels, Millimeters, …). Migrated/legacy
    // pictures often have these units set to Millimeters, so taking
    // the raw values as pixels makes the hit-test rect appear in a
    // completely different place than the visible image. Build a
    // QgsRenderContext and run the same conversion the renderer does.
    QgsRenderContext rc = QgsRenderContext::fromMapSettings( ms );
    const double w = rc.convertToPainterUnits( pic->fixedSize().width(), pic->fixedSizeUnit() );
    const double h = rc.convertToPainterUnits( pic->fixedSize().height(), pic->fixedSizeUnit() );
    // Two render branches in qgsannotationrectitem.cpp:
    //   (A) callout + non-empty calloutAnchor:
    //         painterBounds = QRectF(anchorPx + offset, size)
    //   (B) otherwise:
    //         painterBounds centered on bounds.center
    // Pictures created without a callout (legacy paths, untouched
    // migrations) use branch B and ignore offsetFromCallout. Mirror
    // the same branching here so the hit-test rect always matches the
    // visible picture.
    if ( pic->callout() && !pic->calloutAnchor().isEmpty() )
    {
      const double offW = rc.convertToPainterUnits( pic->offsetFromCallout().width(), pic->offsetFromCalloutUnit() );
      const double offH = rc.convertToPainterUnits( pic->offsetFromCallout().height(), pic->offsetFromCalloutUnit() );
      return QRectF( anchorPx.x() + offW, anchorPx.y() + offH, w, h );
    }
    return QRectF( anchorPx.x() - w / 2.0, anchorPx.y() - h / 2.0, w, h );
  }

  /// The default offset used when placing a fresh picture: centers the
  /// picture rect onto its anchor (so the balloon wedge is invisible at
  /// creation time). The legacy KadasPictureItem started in this state.
  inline QSizeF defaultCenteredOffset( const QgsAnnotationPictureItem *pic )
  {
    const QSizeF size = pic->fixedSize();
    return QSizeF( -size.width() / 2.0, -size.height() / 2.0 );
  }

  /// Returns the picture's visual center in map coordinates. This is
  /// what the renderer actually shows the image centered on, taking
  /// the offset and the picture's own size unit into account. The drag
  /// math (and the edit-tool's mMoveOffset) is anchored on this point
  /// rather than on `bounds.center()`/`anchor` — they only coincide
  /// when offsetFromCallout is the default `-size/2`.
  QgsPointXY imageCenterMap( const QgsAnnotationPictureItem *pic, const QgsMapSettings &ms, const QgsCoordinateTransform &xform )
  {
    const QRectF screen = frameScreenRect( pic, ms, xform );
    if ( !screen.isValid() )
      return pic->bounds().center();
    const QgsPointXY centerMap = ms.mapToPixel().toMapCoordinates( screen.center().toPoint() );
    return centerMap;
  }

  Qgis::PictureFormat formatFromPath( const QString &path )
  {
    const QString ext = QFileInfo( path ).suffix().toLower();
    if ( ext == QLatin1String( "svg" ) )
      return Qgis::PictureFormat::SVG;
    if (
      ext == QLatin1String( "png" )
      || ext == QLatin1String( "jpg" )
      || ext == QLatin1String( "jpeg" )
      || ext == QLatin1String( "bmp" )
      || ext == QLatin1String( "gif" )
      || ext == QLatin1String( "tif" )
      || ext == QLatin1String( "tiff" )
    )
      return Qgis::PictureFormat::Raster;
    return Qgis::PictureFormat::Unknown;
  }
} // namespace


QString KadasPictureAnnotationController::itemType() const
{
  return QStringLiteral( "picture" );
}

QString KadasPictureAnnotationController::itemName() const
{
  return QObject::tr( "Picture" );
}

void KadasPictureAnnotationController::setPath( QgsAnnotationPictureItem *item, const QString &path )
{
  if ( !item )
    return;
  item->setPath( formatFromPath( path ), path );
}

void KadasPictureAnnotationController::ensureBalloon( QgsAnnotationPictureItem *pic )
{
  if ( !pic )
    return;
  // 1. Install a balloon callout if none exists. Replicates the legacy
  //    KadasPictureItem look: white fill, 1px sharp black border, 4px
  //    margin, 6px wedge, 0 corner radius. All units in Pixels so the
  //    bubble keeps its on-screen size regardless of map scale.
  if ( !pic->callout() )
  {
    auto *callout = new QgsBalloonCallout();
    auto *fillLayer = new QgsSimpleFillSymbolLayer( Qt::white, Qt::SolidPattern, Qt::black, Qt::SolidLine, 1.0 );
    fillLayer->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
    callout->setFillSymbol( new QgsFillSymbol( QgsSymbolLayerList() << fillLayer ) );
    callout->setMargins( QgsMargins( 4, 4, 4, 4 ) );
    callout->setMarginsUnit( Qgis::RenderUnit::Pixels );
    callout->setWedgeWidth( 6 );
    callout->setWedgeWidthUnit( Qgis::RenderUnit::Pixels );
    callout->setCornerRadius( 0 );
    callout->setCornerRadiusUnit( Qgis::RenderUnit::Pixels );
    pic->setCallout( callout );
  }
  // 2. Anchor the callout at the bounds center (= the geographic point
  //    the user originally clicked / the migrated item carried) when no
  //    explicit anchor is set yet.
  if ( pic->calloutAnchor().isEmpty() )
  {
    const QgsPointXY c = pic->bounds().center();
    pic->setCalloutAnchor( QgsGeometry( new QgsPoint( c.x(), c.y() ) ) );
  }
  // 3. Initialize the offset to -size/2 so the picture frame starts
  //    centered on the anchor and the wedge has zero visible length
  //    (legacy "no balloon" look). Only do this when the offset is the
  //    default-invalid sentinel — never overwrite a user-positioned
  //    balloon. Use the picture's existing fixedSize unit so the math
  //    is consistent with the renderer's convertToPainterUnits call.
  //
  //    NOTE: this offset is negative, which `QSizeF::isValid()` rejects;
  //    QGIS guards `writeCommonProperties` with that check and will not
  //    persist negative offsets. The save side-channel installed by
  //    `KadasAnnotationProjectIntegration` writes the offset to a layer
  //    customProperty so the balloon position survives a save/reload.
  if ( !pic->offsetFromCallout().isValid() )
  {
    const QSizeF size = pic->fixedSize();
    pic->setOffsetFromCallout( QSizeF( -size.width() / 2.0, -size.height() / 2.0 ) );
    pic->setOffsetFromCalloutUnit( pic->fixedSizeUnit() );
  }
}

bool KadasPictureAnnotationController::isCalloutVisible( const QgsAnnotationPictureItem *pic )
{
  if ( !pic )
    return false;
  const auto *balloon = dynamic_cast<const QgsBalloonCallout *>( pic->callout() );
  if ( !balloon )
    return false;
  // "Hidden" state encoded by the style editor: transparent fill,
  // transparent stroke and zero-width wedge. Any one being non-default
  // means the user wants the balloon shown.
  if ( balloon->wedgeWidth() > 0.0 )
    return true;
  if ( const QgsFillSymbol *sym = const_cast<QgsBalloonCallout *>( balloon )->fillSymbol() )
  {
    if ( sym->symbolLayerCount() > 0 )
    {
      if ( const auto *sl = dynamic_cast<const QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) ) )
      {
        if ( sl->color().alpha() > 0 || sl->strokeColor().alpha() > 0 )
          return true;
      }
    }
  }
  return false;
}

QgsAnnotationItem *KadasPictureAnnotationController::createItem() const
{
  // Default 200x150 pixel picture frame placed at the origin; the actual
  // anchor is set by startPart().
  auto *item = new QgsAnnotationPictureItem( Qgis::PictureFormat::Unknown, QString(), QgsRectangle( 0, 0, 0, 0 ) );
  item->setZIndex( KadasAnnotationZIndex::Picture );
  item->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
  item->setFixedSize( QSizeF( 200, 150 ) );
  item->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
  ensureBalloon( item );
  return item;
}

QList<KadasNode> KadasPictureAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  // Single edit handle = the geographic anchor (the callout anchor,
  // which is what the renderer uses to position the picture).
  return { { toMapPos( pictureAnchorItem( asPicture( item ) ), ctx ) } };
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Bounds center is the anchor; size doesn't matter in FixedSize mode but
  // keeping it as a degenerate rectangle is clearer than a 0x0 area.
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  // Callout anchor mirrors the bounds center (a Point geometry).
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
  return false; // single-click placement
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasPictureAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( p, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasPictureAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasPictureAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasPictureAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasPictureAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasPictureAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasPictureAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsAnnotationPictureItem *pic = asPicture( item );
  const QgsPointXY anchorMap = toMapPos( pictureAnchorItem( pic ), ctx );

  // Anchor handle wins when the click is within pick tolerance of it,
  // even when that point is also inside the frame rectangle. Otherwise
  // a balloon whose anchor sits over (or very near) the frame body
  // would be impossible to relocate — every click would land on the
  // much larger frame area and trigger a frame drag. Pick tolerance is
  // small (a few pixels) so the frame remains easy to grab anywhere
  // else.
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  if ( pos.sqrDist( anchorMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( kPartAnchor, 0, 0 ), anchorMap, drawAttribs() );
  const QRectF frameRect = frameScreenRect( pic, ctx.mapSettings(), xform );
  const QPointF screenPos = ctx.mapSettings().mapToPixel().transform( pos ).toQPointF();
  if ( frameRect.isValid() && frameRect.contains( screenPos ) )
  {
    // Returned `pos` field is the image's *visual center* in map
    // coords — not the click point. The edit-tool computes
    //   mMoveOffset = clickPos - editContext.pos
    // and on each drag passes
    //   newPoint = mousePos - mMoveOffset = mousePos - clickPos + imageCenter
    // i.e. newPoint always equals the desired NEW image center. That
    // simplifies the edit() math (just set the image center to
    // newPoint) and prevents the image from snapping under the cursor
    // on press when offsetFromCallout is non-default.
    const QgsPointXY centerMap = imageCenterMap( pic, ctx.mapSettings(), xform );
    return KadasEditContext( QgsVertexId( kPartFrame, 0, 0 ), centerMap, KadasAttribDefs(), Qt::SizeAllCursor );
  }
  return KadasEditContext();
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPictureItem *pic = asPicture( item );

  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartFrame )
  {
    // newPoint is the new desired image center (see getEditContext).
    //
    // Callout hidden: per the user-visible rule "dragging the image
    // deletes the anchor's old position so it cannot drift". Snap
    // bounds + anchor to the new center and reset offset to -size/2,
    // i.e. centered-on-anchor. Toggling the callout back on later
    // makes the wedge appear at the new image location with zero
    // visible length.
    if ( !isCalloutVisible( pic ) )
    {
      const QgsPointXY ip = toItemPos( newPoint, ctx );
      pic->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
      pic->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
      const QSizeF size = pic->fixedSize();
      pic->setOffsetFromCallout( QSizeF( -size.width() / 2.0, -size.height() / 2.0 ) );
      pic->setOffsetFromCalloutUnit( pic->fixedSizeUnit() );
      return;
    }

    // Callout visible: anchor stays put, image center moves to
    // newPoint. The renderer draws the picture top-left at
    // anchor+offset, so to land its CENTER at newPoint we need
    //   anchor_px + offset_px + size_px/2 == target_px
    //   offset_px = target_px - anchor_px - size_px/2.
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    QgsPointXY anchorMap = pictureAnchorItem( pic );
    try
    {
      if ( xform.isValid() )
        anchorMap = xform.transform( anchorMap );
    }
    catch ( const QgsCsException & )
    {
      return;
    }
    const QPointF anchorPx = ctx.mapSettings().mapToPixel().transform( anchorMap ).toQPointF();
    const QPointF targetPx = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF();
    QgsRenderContext rc = QgsRenderContext::fromMapSettings( ctx.mapSettings() );
    const double w = rc.convertToPainterUnits( pic->fixedSize().width(), pic->fixedSizeUnit() );
    const double h = rc.convertToPainterUnits( pic->fixedSize().height(), pic->fixedSizeUnit() );
    const QPointF offsetPx = targetPx - anchorPx - QPointF( w / 2.0, h / 2.0 );
    pic->setOffsetFromCallout( QSizeF( offsetPx.x(), offsetPx.y() ) );
    pic->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
    return;
  }

  // Anchor handle drag (and any whole-item move that lands here): move
  // the bounds + callout anchor together, leaving the offset alone so
  // the balloon shape is preserved.
  const QgsPointXY ip = toItemPos( newPoint, ctx );
  pic->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  pic->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasPictureAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasPictureAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasPictureAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsRectangle b = asPicture( item )->bounds();
  return QgsPointXY( b.center().x(), b.center().y() );
}

void KadasPictureAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  asPicture( item )->setBounds( QgsRectangle( pos.x(), pos.y(), pos.x(), pos.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( pos.x(), pos.y() ) ) );
}

void KadasPictureAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsPointXY c = b.center();
  setPosition( item, QgsPointXY( c.x() + dx, c.y() + dy ) );
}

KadasAnnotationStyleEditor *KadasPictureAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasPictureStyleEditor( parent );
}

void KadasPictureAnnotationController::populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPictureItem *pic = asPicture( item );

  // "Change image source..." — opens a file picker preseeded with the
  // current path, applies the new file via setPath() (auto-detects
  // raster vs SVG from the suffix), and triggers a layer repaint.
  QPointer<QgsAnnotationLayer> layerPtr( ctx.layer() );
  QAction *changeAction = menu->addAction( QObject::tr( "Change image source…" ) );
  QObject::connect( changeAction, &QAction::triggered, changeAction, [pic, layerPtr]() {
    const QString current = pic->path();
    const QString chosen = QFileDialog::
      getOpenFileName( nullptr, QObject::tr( "Select picture" ), current.isEmpty() ? QDir::homePath() : QFileInfo( current ).absolutePath(), QObject::tr( "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff *.svg)" ) );
    if ( chosen.isEmpty() )
      return;
    KadasPictureAnnotationController::setPath( pic, chosen );
    if ( layerPtr )
      layerPtr->triggerRepaint();
  } );

  // Show "Reset balloon" only when the picture is actually offset off
  // its centered-on-anchor position (i.e. the wedge is visible). The
  // canonical "no balloon" state is offset = -size/2 (top-left form,
  // picture centered on anchor), see createItem().
  const QSizeF off = pic->offsetFromCallout();
  const QSizeF centered = defaultCenteredOffset( pic );
  if ( qFuzzyCompare( off.width(), centered.width() ) && qFuzzyCompare( off.height(), centered.height() ) )
    return;
  QAction *resetAction = menu->addAction( QObject::tr( "Reset balloon" ) );
  QObject::connect( resetAction, &QAction::triggered, resetAction, [pic, layerPtr]() {
    const QSizeF size = pic->fixedSize();
    pic->setOffsetFromCallout( QSizeF( -size.width() / 2.0, -size.height() / 2.0 ) );
    if ( layerPtr )
      layerPtr->triggerRepaint();
  } );
}

QString KadasPictureAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  // KMZ embedding of the image asset is deferred; emit a Point Placemark at
  // the anchor so the picture is at least geographically locatable in KML.
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  const QgsPointXY anchor = ct.transform( QgsPointXY( b.center() ) );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<Point><coordinates>" << QString::number( anchor.x(), 'f', 10 ) << "," << QString::number( anchor.y(), 'f', 10 ) << "</coordinates></Point>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
