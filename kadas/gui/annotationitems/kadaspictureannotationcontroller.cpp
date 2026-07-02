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

#include <cmath>

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
#include <qgis/qgsapplication.h>
#include <qgis/qgscallout.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsimagecache.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsmargins.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgssvgcache.h>
#include <qgis/qgssymbol.h>

#include "kadas/gui/annotationitems/kadasannotationrotation.h"
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

  // vidx.part sentinels: what the user grabbed.
  constexpr int kPartAnchor = 0;
  constexpr int kPartFrame = 1;
  constexpr int kPartCorner = 2;
  constexpr int kPartRotate = 3;
  // Corner order matches frameCornersScreen().
  constexpr int kCornerTL = 0;
  constexpr int kCornerTR = 1;
  constexpr int kCornerBR = 2;
  constexpr int kCornerBL = 3;

  // Pixels above the frame's top edge where the rotation handle sits.
  constexpr double kRotationHandleOffsetPx = 30.0;

  // Rotate a screen point around \a center by \a deg degrees clockwise (matching
  // QPainter::rotate in screen space, where Y points down).
  QPointF rotatePointScreen( const QPointF &p, const QPointF &center, double deg )
  {
    if ( qgsDoubleNear( deg, 0.0 ) )
      return p;
    const double rad = deg * M_PI / 180.0;
    const double c = std::cos( rad );
    const double s = std::sin( rad );
    const double dx = p.x() - center.x();
    const double dy = p.y() - center.y();
    return QPointF( center.x() + dx * c - dy * s, center.y() + dx * s + dy * c );
  }

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
    // qgsannotationrectitem.cpp ~line 99,113 converts size/offset to painter units via the item's unit, not raw pixels.
    QgsRenderContext rc = QgsRenderContext::fromMapSettings( ms );
    const double w = rc.convertToPainterUnits( pic->fixedSize().width(), pic->fixedSizeUnit() );
    const double h = rc.convertToPainterUnits( pic->fixedSize().height(), pic->fixedSizeUnit() );
    // qgsannotationrectitem.cpp: with callout+anchor, top-left = anchor+offset; otherwise centered on bounds.center.
    if ( pic->callout() && !pic->calloutAnchor().isEmpty() )
    {
      const double offW = rc.convertToPainterUnits( pic->offsetFromCallout().width(), pic->offsetFromCalloutUnit() );
      const double offH = rc.convertToPainterUnits( pic->offsetFromCallout().height(), pic->offsetFromCalloutUnit() );
      return QRectF( anchorPx.x() + offW, anchorPx.y() + offH, w, h );
    }
    return QRectF( anchorPx.x() - w / 2.0, anchorPx.y() - h / 2.0, w, h );
  }

  inline QSizeF defaultCenteredOffset( const QgsAnnotationPictureItem *pic )
  {
    const QSizeF size = pic->fixedSize();
    return QSizeF( -size.width() / 2.0, -size.height() / 2.0 );
  }

  QgsPointXY imageCenterMap( const QgsAnnotationPictureItem *pic, const QgsMapSettings &ms, const QgsCoordinateTransform &xform )
  {
    const QRectF screen = frameScreenRect( pic, ms, xform );
    if ( !screen.isValid() )
      return pic->bounds().center();
    const QgsPointXY centerMap = ms.mapToPixel().toMapCoordinates( screen.center().toPoint() );
    return centerMap;
  }

  QVector<QPointF> frameCornersScreen( const QgsAnnotationPictureItem *pic, const QgsMapSettings &ms, const QgsCoordinateTransform &xform )
  {
    const QRectF r = frameScreenRect( pic, ms, xform );
    if ( !r.isValid() )
      return {};
    return { r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft() };
  }

  // Frame corners rotated around the frame center by the item's rotation, so the
  // handles track the rendered (rotated) image.
  QVector<QPointF> frameCornersScreenRotated( const QgsAnnotationPictureItem *pic, const QgsMapSettings &ms, const QgsCoordinateTransform &xform )
  {
    const QRectF r = frameScreenRect( pic, ms, xform );
    if ( !r.isValid() )
      return {};
    const QPointF center = r.center();
    const double rot = pic->rotation();
    return { rotatePointScreen( r.topLeft(), center, rot ), rotatePointScreen( r.topRight(), center, rot ), rotatePointScreen( r.bottomRight(), center, rot ), rotatePointScreen( r.bottomLeft(), center, rot ) };
  }

  // Screen position of the rotation handle: above the (rotated) top edge midpoint.
  QPointF rotationHandleScreen( const QRectF &frame, double rot )
  {
    const QPointF center = frame.center();
    const QPointF topMid( center.x(), frame.top() - kRotationHandleOffsetPx );
    return rotatePointScreen( topMid, center, rot );
  }

  // Item rotation (degrees clockwise) that places the handle/top edge towards a
  // screen cursor relative to the frame center. Up (0,-1) maps to rotation 0.
  double rotationFromHandleScreen( const QPointF &centerPx, const QPointF &cursorPx )
  {
    const double dx = cursorPx.x() - centerPx.x();
    const double dy = cursorPx.y() - centerPx.y();
    return std::atan2( dx, -dy ) * 180.0 / M_PI;
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

  // Native aspect ratio (height / width) of the source image, or 0 if unknown.
  double sourceAspectRatio( const QString &path, Qgis::PictureFormat format )
  {
    switch ( format )
    {
      case Qgis::PictureFormat::SVG:
      {
        const QSizeF vb = QgsApplication::svgCache()->svgViewboxSize( path, 100, QColor(), QColor(), 1.0, 1.0 );
        if ( vb.width() > 0 && vb.height() > 0 )
          return vb.height() / vb.width();
        break;
      }
      case Qgis::PictureFormat::Raster:
      {
        const QSize s = QgsApplication::imageCache()->originalSize( path );
        if ( s.width() > 0 && s.height() > 0 )
          return static_cast<double>( s.height() ) / s.width();
        break;
      }
      case Qgis::PictureFormat::Unknown:
        break;
    }
    return 0.0;
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
  const Qgis::PictureFormat format = formatFromPath( path );
  item->setPath( format, path );
  // Match the frame aspect to the image so the balloon/frame (and the edit
  // handles drawn on its corners) tightly wrap the picture. Otherwise the image
  // is letterboxed inside the frame and the corner handles look misaligned,
  // which is especially obvious once the item is rotated.
  if ( lockAspectRatio() )
  {
    const double aspect = sourceAspectRatio( path, format );
    if ( aspect > 0.0 )
    {
      const double width = item->fixedSize().width() > 0 ? item->fixedSize().width() : 200.0;
      item->setFixedSize( QSizeF( width, width * aspect ) );
    }
  }
}

void KadasPictureAnnotationController::ensureBalloon( QgsAnnotationPictureItem *pic )
{
  if ( !pic )
    return;
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
  if ( pic->calloutAnchor().isEmpty() )
  {
    const QgsPointXY c = pic->bounds().center();
    pic->setCalloutAnchor( QgsGeometry( new QgsPoint( c.x(), c.y() ) ) );
  }
  // Negative offset: QSizeF::isValid() rejects it so QGIS won't persist it; saved via a layer customProperty.
  if ( !pic->offsetFromCallout().isValid() )
  {
    const QSizeF size = pic->fixedSize();
    pic->setOffsetFromCallout( QSizeF( -size.width() / 2.0, -size.height() / 2.0 ) );
    pic->setOffsetFromCalloutUnit( pic->fixedSizeUnit() );
  }
}

int KadasPictureAnnotationController::nextPictureZIndex( const QgsAnnotationLayer *layer )
{
  if ( !layer )
    return KadasAnnotationZIndex::Picture;
  int maxZ = KadasAnnotationZIndex::Picture - 1;
  const QMap<QString, QgsAnnotationItem *> all = layer->items();
  for ( auto it = all.constBegin(), itEnd = all.constEnd(); it != itEnd; ++it )
  {
    if ( !it.value() || it.value()->type() != QLatin1String( "picture" ) )
      continue;
    maxZ = std::max( maxZ, it.value()->zIndex() );
  }
  return maxZ + 1;
}

bool KadasPictureAnnotationController::isCalloutVisible( const QgsAnnotationPictureItem *pic )
{
  if ( !pic )
    return false;
  const auto *balloon = dynamic_cast<const QgsBalloonCallout *>( pic->callout() );
  if ( !balloon )
    return false;
  // Hidden state = transparent fill+stroke and zero wedge.
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

void KadasPictureAnnotationController::setCalloutVisible( QgsAnnotationPictureItem *pic, bool visible )
{
  if ( !pic )
    return;
  ensureBalloon( pic );
  auto *balloon = dynamic_cast<QgsBalloonCallout *>( pic->callout() );
  if ( !balloon )
    return;
  if ( visible == isCalloutVisible( pic ) )
    return;
  if ( visible )
  {
    auto *sl = new QgsSimpleFillSymbolLayer( Qt::white, Qt::SolidPattern, Qt::black, Qt::SolidLine, 1.0 );
    sl->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
    balloon->setFillSymbol( new QgsFillSymbol( QgsSymbolLayerList() << sl ) );
    balloon->setWedgeWidth( 6 );
    balloon->setWedgeWidthUnit( Qgis::RenderUnit::Pixels );
  }
  else
  {
    auto *sl = new QgsSimpleFillSymbolLayer( QColor( 0, 0, 0, 0 ), Qt::SolidPattern, QColor( 0, 0, 0, 0 ), Qt::SolidLine, 0.0 );
    sl->setStrokeWidthUnit( Qgis::RenderUnit::Pixels );
    balloon->setFillSymbol( new QgsFillSymbol( QgsSymbolLayerList() << sl ) );
    balloon->setWedgeWidth( 0 );
    pic->setOffsetFromCallout( defaultCenteredOffset( pic ) );
    pic->setOffsetFromCalloutUnit( pic->fixedSizeUnit() );
  }
}

namespace
{
  bool s_lockAspectRatio = true;
} //namespace

bool KadasPictureAnnotationController::lockAspectRatio()
{
  return s_lockAspectRatio;
}

void KadasPictureAnnotationController::setLockAspectRatio( bool on )
{
  s_lockAspectRatio = on;
}

QgsAnnotationItem *KadasPictureAnnotationController::createItem() const
{
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
  const QgsAnnotationPictureItem *pic = asPicture( item );
  QList<KadasNode> result;
  if ( isCalloutVisible( pic ) )
    result.append( { toMapPos( pictureAnchorItem( pic ), ctx ) } );
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
  if ( !frame.isValid() )
    return result;
  const QVector<QPointF> corners = frameCornersScreenRotated( pic, ctx.mapSettings(), xform );
  for ( const QPointF &c : corners )
    result.append( { ctx.mapSettings().mapToPixel().toMapCoordinates( c.toPoint() ) } );
  const QPointF handlePx = rotationHandleScreen( frame, pic->rotation() );
  result.append( { ctx.mapSettings().mapToPixel().toMapCoordinates( handlePx.toPoint() ), []( QPainter *p, const QPointF &pt, int size ) { KadasAnnotationRotation::renderHandle( p, pt, size ); } } );
  return result;
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
  item->setZIndex( nextPictureZIndex( ctx.layer() ) );
  return false;
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
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );

  const QRectF frameRect = frameScreenRect( pic, ctx.mapSettings(), xform );

  // Corner handles win over anchor/frame (they sit on the outline).
  const QVector<QPointF> corners = frameCornersScreenRotated( pic, ctx.mapSettings(), xform );
  for ( int i = 0; i < corners.size(); ++i )
  {
    const QgsPointXY cornerMap = ctx.mapSettings().mapToPixel().toMapCoordinates( corners[i].toPoint() );
    if ( pos.sqrDist( cornerMap ) < pickTolSqr( ctx ) )
    {
      const Qt::CursorShape c = ( i == kCornerTL || i == kCornerBR ) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
      return KadasEditContext( QgsVertexId( kPartCorner, 0, i ), cornerMap, KadasAttribDefs(), c );
    }
  }

  // Rotation handle.
  if ( frameRect.isValid() )
  {
    const QPointF handlePx = rotationHandleScreen( frameRect, pic->rotation() );
    const QgsPointXY handleMap = ctx.mapSettings().mapToPixel().toMapCoordinates( handlePx.toPoint() );
    if ( pos.sqrDist( handleMap ) < pickTolSqr( ctx ) )
    {
      KadasAttribDefs rotAttribs;
      rotAttribs.insert( AttrAngle, KadasNumericAttribute { "angle", KadasNumericAttribute::Type::TypeAngle } );
      return KadasEditContext( QgsVertexId( kPartRotate, 0, 0 ), handleMap, rotAttribs, Qt::CrossCursor );
    }
  }

  // Anchor handle wins within pick tolerance even inside the frame; skipped when the callout is hidden (handle also hidden in nodes()).
  const QgsPointXY anchorMap = toMapPos( pictureAnchorItem( pic ), ctx );
  if ( isCalloutVisible( pic ) && pos.sqrDist( anchorMap ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( kPartAnchor, 0, 0 ), anchorMap, drawAttribs() );
  if ( frameRect.isValid() )
  {
    const QPointF screenPos = ctx.mapSettings().mapToPixel().transform( pos ).toQPointF();
    // Un-rotate the cursor into the frame's local (axis-aligned) space before the contains() test.
    const QPointF localPos = rotatePointScreen( screenPos, frameRect.center(), -pic->rotation() );
    if ( frameRect.contains( localPos ) )
    {
      // pos = image visual center, so edit() receives newPoint == desired new center.
      const QgsPointXY centerMap = imageCenterMap( pic, ctx.mapSettings(), xform );
      return KadasEditContext( QgsVertexId( kPartFrame, 0, 0 ), centerMap, KadasAttribDefs(), Qt::SizeAllCursor );
    }
  }
  return KadasEditContext();
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPictureItem *pic = asPicture( item );

  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartRotate )
  {
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
    if ( !frame.isValid() )
      return;
    const QPointF cursorPx = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF();
    const double angle = rotationFromHandleScreen( frame.center(), cursorPx );
    pic->setRotation( KadasAnnotationRotation::snapAngle( angle, ctx.modifiers() & Qt::ShiftModifier ) );
    return;
  }

  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartCorner )
  {
    // Corner resize: opposite corner fixed, dragged corner follows the cursor (px space).
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    const QVector<QPointF> corners = frameCornersScreen( pic, ctx.mapSettings(), xform );
    const int idx = editContext.vidx.vertex;
    if ( corners.size() != 4 || idx < 0 || idx > 3 )
      return;
    // Diagonal opposite: TL<->BR (0<->2), TR<->BL (1<->3).
    const QPointF fixedPx = corners[( idx + 2 ) % 4];
    // Resize math works in the unrotated frame: un-rotate the cursor around the
    // (unrotated) frame center so the dragged corner maps back to local axes.
    const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
    const QPointF cursorPx = rotatePointScreen( ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF(), frame.center(), -pic->rotation() );
    double newW = std::abs( cursorPx.x() - fixedPx.x() );
    double newH = std::abs( cursorPx.y() - fixedPx.y() );
    constexpr double kMinSizePx = 16.0;
    newW = std::max( newW, kMinSizePx );
    newH = std::max( newH, kMinSizePx );

    QgsRenderContext rc = QgsRenderContext::fromMapSettings( ctx.mapSettings() );
    const double oldWpx = rc.convertToPainterUnits( pic->fixedSize().width(), pic->fixedSizeUnit() );
    const double oldHpx = rc.convertToPainterUnits( pic->fixedSize().height(), pic->fixedSizeUnit() );
    if ( lockAspectRatio() && oldWpx > 0.0 && oldHpx > 0.0 )
    {
      // Lock aspect: grow along the dominant axis.
      const double aspect = oldWpx / oldHpx;
      if ( newW / oldWpx > newH / oldHpx )
        newH = newW / aspect;
      else
        newW = newH * aspect;
    }

    // New top-left in pixels: depends on which corner is fixed.
    QPointF newTL;
    newTL.setX( ( idx == kCornerTL || idx == kCornerBL ) ? fixedPx.x() - newW : fixedPx.x() );
    newTL.setY( ( idx == kCornerTL || idx == kCornerTR ) ? fixedPx.y() - newH : fixedPx.y() );

    // Convert the new size back to the picture's size unit to avoid changing the unit.
    const double pxPerUnitW = rc.convertToPainterUnits( 1.0, pic->fixedSizeUnit() );
    if ( pxPerUnitW > 0.0 )
      pic->setFixedSize( QSizeF( newW / pxPerUnitW, newH / pxPerUnitW ) );

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
    const QPointF offsetPx = newTL - anchorPx;
    pic->setOffsetFromCallout( QSizeF( offsetPx.x(), offsetPx.y() ) );
    pic->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );

    // Callout hidden: anchor follows the image.
    if ( !isCalloutVisible( pic ) )
    {
      const QPointF newCenterPx = newTL + QPointF( newW / 2.0, newH / 2.0 );
      const QgsPointXY newCenterMap = ctx.mapSettings().mapToPixel().toMapCoordinates( newCenterPx.toPoint() );
      const QgsPointXY ip = toItemPos( newCenterMap, ctx );
      pic->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
      pic->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
      pic->setOffsetFromCallout( QSizeF( -newW / 2.0, -newH / 2.0 ) );
    }
    return;
  }

  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartFrame )
  {
    // newPoint is the new desired image center (see getEditContext).
    // Callout hidden: snap bounds+anchor to the new center, offset = -size/2.
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

    // Callout visible: anchor fixed; offset = target - anchor - size/2 to center on newPoint.
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

  // Anchor drag.
  // Callout hidden: no wedge to stretch, so move the image with the anchor (keep offset).
  if ( !isCalloutVisible( pic ) )
  {
    const QgsPointXY ip = toItemPos( newPoint, ctx );
    pic->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
    pic->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
    return;
  }

  // Callout visible: move only the anchor (wedge tip). Keep the balloon image
  // fixed on screen by re-deriving the offset, so the wedge stretches.
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
  const QgsPointXY ip = toItemPos( newPoint, ctx );
  pic->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  pic->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
  if ( frame.isValid() )
  {
    const QPointF newAnchorPx = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF();
    const QPointF offsetPx = frame.topLeft() - newAnchorPx;
    pic->setOffsetFromCallout( QSizeF( offsetPx.x(), offsetPx.y() ) );
    pic->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
  }
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartRotate )
  {
    asPicture( item )->setRotation( values[AttrAngle] );
    return;
  }
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasPictureAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartRotate )
  {
    const QgsAnnotationPictureItem *pic = asPicture( item );
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
    KadasAttribValues values;
    if ( frame.isValid() )
    {
      const QPointF cursorPx = ctx.mapSettings().mapToPixel().transform( pos ).toQPointF();
      values.insert( AttrAngle, rotationFromHandleScreen( frame.center(), cursorPx ) );
    }
    return values;
  }
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasPictureAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.isValid() && editContext.vidx.part == kPartRotate )
  {
    const QgsAnnotationPictureItem *pic = asPicture( item );
    const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
    const QRectF frame = frameScreenRect( pic, ctx.mapSettings(), xform );
    if ( frame.isValid() )
    {
      const QPointF handlePx = rotationHandleScreen( frame, values[AttrAngle] );
      return ctx.mapSettings().mapToPixel().toMapCoordinates( handlePx.toPoint() );
    }
  }
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

QgsGeometry KadasPictureAnnotationController::representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  // boundingBox() is degenerate for a FixedSize picture; build the band from the visual footprint.
  const auto *pic = asPicture( item );
  const QgsCoordinateTransform xform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  const QRectF screenRect = frameScreenRect( pic, ctx.mapSettings(), xform );
  if ( !screenRect.isValid() )
    return KadasAnnotationItemController::representativeGeometry( item, ctx );
  const QVector<QPointF> cornersPx = frameCornersScreenRotated( pic, ctx.mapSettings(), xform );
  QVector<QgsPointXY> ring;
  ring.reserve( 5 );
  for ( const QPointF &px : cornersPx )
  {
    const QgsPointXY mapPt = ctx.mapSettings().mapToPixel().toMapCoordinates( px.toPoint() );
    ring.append( toItemPos( mapPt, ctx ) );
  }
  ring.append( ring.first() );
  return QgsGeometry::fromPolygonXY( { ring } );
}

KadasAnnotationStyleEditor *KadasPictureAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasPictureStyleEditor( parent );
}

void KadasPictureAnnotationController::populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPictureItem *pic = asPicture( item );

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

  QAction *calloutAction = menu->addAction( QObject::tr( "Show callout frame" ) );
  calloutAction->setCheckable( true );
  calloutAction->setChecked( isCalloutVisible( pic ) );
  QObject::connect( calloutAction, &QAction::toggled, calloutAction, [pic, layerPtr]( bool on ) {
    KadasPictureAnnotationController::setCalloutVisible( pic, on );
    if ( layerPtr )
      layerPtr->triggerRepaint();
  } );

  if ( !qgsDoubleNear( pic->rotation(), 0.0 ) )
  {
    QAction *resetRotationAction = menu->addAction( QObject::tr( "Reset rotation" ) );
    QObject::connect( resetRotationAction, &QAction::triggered, resetRotationAction, [pic, layerPtr]() {
      pic->setRotation( 0.0 );
      if ( layerPtr )
        layerPtr->triggerRepaint();
    } );
  }

  // Offer "Reset balloon" only when the offset differs from centered (-size/2).
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
  // Image embedding deferred; emit a Point Placemark at the anchor.
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
