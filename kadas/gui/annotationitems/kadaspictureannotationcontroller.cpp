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
    const QSizeF off = pic->offsetFromCallout();
    const QSizeF size = pic->fixedSize();
    return QRectF( anchorPx.x() + off.width(), anchorPx.y() + off.height(), size.width(), size.height() );
  }

  /// The default offset used when placing a fresh picture: centers the
  /// picture rect onto its anchor (so the balloon wedge is invisible at
  /// creation time). The legacy KadasPictureItem started in this state.
  inline QSizeF defaultCenteredOffset( const QgsAnnotationPictureItem *pic )
  {
    const QSizeF size = pic->fixedSize();
    return QSizeF( -size.width() / 2.0, -size.height() / 2.0 );
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

QgsAnnotationItem *KadasPictureAnnotationController::createItem() const
{
  // Default 200x150 pixel picture frame placed at the origin; the actual
  // anchor is set by startPart().
  auto *item = new QgsAnnotationPictureItem( Qgis::PictureFormat::Unknown, QString(), QgsRectangle( 0, 0, 0, 0 ) );
  item->setZIndex( KadasAnnotationZIndex::Picture );
  item->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
  // The picture starts centered on its anchor (offset = -size/2, top-left
  // form). With the picture centered on the anchor the balloon wedge is
  // invisible — the legacy KadasPictureItem look. Dragging the frame
  // body shifts the offset off-center; once the anchor falls outside the
  // picture rect, the wedge becomes visible.
  item->setFixedSize( QSizeF( 200, 150 ) );
  item->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
  item->setOffsetFromCallout( QSizeF( -100, -75 ) );
  item->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
  // Auto-install a balloon callout (cartoon speech-bubble) — the picture
  // frame becomes the bubble body, with a wedge pointing back to the
  // geographic anchor. Replicates the legacy KadasPictureItem look:
  //   * white fill with a 1px sharp-corner black border (sFramePadding=4
  //     and sArrowWidth=6 from the legacy KadasRectangleItemBase);
  //   * 4px margin so the bubble extends slightly beyond the picture
  //     content like the legacy frame did;
  //   * 0px corner radius so the bubble is a sharp rectangle (legacy
  //     drew a polygon, not a rounded rect).
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
  item->setCallout( callout );
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

  // Frame body wins over the anchor handle. Rationale: when the balloon
  // is collapsed (offset 0) the anchor and the frame center share a
  // pixel; if the anchor handle won, dragging the picture would just
  // translate the whole assembly and the user could never inflate the
  // balloon. Hitting the frame body always sets the offset (anchor
  // stays put → balloon inflates / re-shapes), and the anchor handle
  // is reachable only when it is visually outside the frame, i.e. once
  // the balloon is already inflated.
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  const QRectF frameRect = frameScreenRect( pic, ctx.mapSettings(), xform );
  if ( frameRect.isValid() )
  {
    const QPointF screenPos = ctx.mapSettings().mapToPixel().transform( pos ).toQPointF();
    if ( frameRect.contains( screenPos ) )
    {
      // Returned `pos` field is the click in map coords so the edit-tool
      // can compute mMoveOffset. The vidx part slot marks this as the
      // frame body grab (QgsVertexId ctor signature is (part, ring,
      // vertex), so the discriminator must live in the first arg).
      return KadasEditContext( QgsVertexId( kPartFrame, 0, 0 ), pos, KadasAttribDefs(), Qt::SizeAllCursor );
    }
  }
  if ( pos.sqrDist( anchorMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( kPartAnchor, 0, 0 ), anchorMap, drawAttribs() );
  return KadasEditContext();
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPictureItem *pic = asPicture( item );

  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartFrame )
  {
    // Frame-body drag: keep the geographic anchor fixed, move the
    // picture frame instead by adjusting the screen-space offset. The
    // wedge of the balloon callout grows out from the anchor toward the
    // new frame position.
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    const QgsPointXY itemAnchor = pictureAnchorItem( pic );
    QgsPointXY anchorMap = itemAnchor;
    try
    {
      if ( xform.isValid() )
        anchorMap = xform.transform( itemAnchor );
    }
    catch ( const QgsCsException & )
    {
      return;
    }
    const QPointF anchorPx = ctx.mapSettings().mapToPixel().transform( anchorMap ).toQPointF();
    const QPointF targetPx = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF();
    // Top-left form: target should become the picture's CENTER, so the
    // top-left lives at target - size/2. offset = (top-left - anchor).
    const QSizeF size = pic->fixedSize();
    const QPointF topLeft = targetPx - QPointF( size.width() / 2.0, size.height() / 2.0 );
    const QPointF delta = topLeft - anchorPx;
    pic->setOffsetFromCallout( QSizeF( delta.x(), delta.y() ) );
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
