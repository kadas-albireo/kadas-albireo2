/***************************************************************************
    kadaslinetextannotationcontroller.cpp
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

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QPointer>
#include <QTextStream>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationlinetextitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsgeometryutils_base.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/annotationitems/kadasannotationrotation.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaslinetextannotationcontroller.h"


// ----- Persisted last-used style ----------------------------------------

const QgsSettingsEntryDouble *KadasLineTextAnnotationController::settingsSize
  = new QgsSettingsEntryDouble( QStringLiteral( "linetext-size" ), sTreeAnnotation, 10.0, QStringLiteral( "Last-used line-text size (points)." ) );
const QgsSettingsEntryColor *KadasLineTextAnnotationController::settingsColor
  = new QgsSettingsEntryColor( QStringLiteral( "linetext-color" ), sTreeAnnotation, QColor( 0, 0, 0 ), QStringLiteral( "Last-used line-text color." ) );
const QgsSettingsEntryColor *KadasLineTextAnnotationController::settingsBufferColor
  = new QgsSettingsEntryColor( QStringLiteral( "linetext-buffer-color" ), sTreeAnnotation, QColor( 255, 255, 255, 0 ), QStringLiteral( "Last-used line-text buffer (border) color." ) );
const QgsSettingsEntryDouble *KadasLineTextAnnotationController::settingsBufferWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "linetext-buffer-width" ), sTreeAnnotation, 0.0, QStringLiteral( "Last-used line-text buffer (border) width (mm)." ) );

namespace
{
  inline QgsAnnotationLineTextItem *asLineText( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "linetext" ) );
    return static_cast<QgsAnnotationLineTextItem *>( item );
  }
  inline const QgsAnnotationLineTextItem *asLineText( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "linetext" ) );
    return static_cast<const QgsAnnotationLineTextItem *>( item );
  }

  // vidx.part sentinel for the rotation handle (real vertices use part 0).
  constexpr int kPartRotate = 1;

  //! Returns a mutable QgsLineString backing the item, cloning it via setGeometry() for in-place edits.
  QgsLineString *takeMutableLine( QgsAnnotationLineTextItem *item )
  {
    const QgsCurve *curve = item->geometry();
    if ( !curve )
    {
      auto fresh = new QgsLineString();
      item->setGeometry( fresh );
      return fresh;
    }
    QgsLineString *clone = qgsgeometry_cast<QgsLineString *>( curve->clone() );
    Q_ASSERT( clone );
    item->setGeometry( clone );
    return clone;
  }
} // namespace


QString KadasLineTextAnnotationController::itemType() const
{
  return QStringLiteral( "linetext" );
}

QString KadasLineTextAnnotationController::itemName() const
{
  return QObject::tr( "Text along Line" );
}

QgsAnnotationItem *KadasLineTextAnnotationController::createItem() const
{
  auto *item = new QgsAnnotationLineTextItem( QString(), new QgsLineString() );
  item->setZIndex( KadasAnnotationZIndex::LineText );
  return item;
}

bool KadasLineTextAnnotationController::isEmpty( const QgsAnnotationItem *item ) const
{
  return asLineText( item )->text().trimmed().isEmpty();
}

QgsGeometry KadasLineTextAnnotationController::representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext & ) const
{
  const QgsCurve *curve = asLineText( item )->geometry();
  return curve ? QgsGeometry( curve->clone() ) : QgsGeometry();
}

QList<KadasNode> KadasLineTextAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasNode> result;
  const QgsCurve *curve = asLineText( item )->geometry();
  if ( !curve )
    return result;
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    result.append( { toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) } );
  }
  // Rotation handle above the geometric centroid (needs at least a segment).
  if ( n >= 2 )
  {
    QgsPointXY handle;
    if ( mRotation.active() )
    {
      // Follow the cursor while rotating.
      handle = mRotation.handle();
    }
    else
    {
      const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
      const QgsPointXY centerMap = centroidMap( curve, ctx );
      handle = KadasAnnotationRotation::VertexRotationState::restHandle( centerMap, off );
    }
    result.append( { handle, []( QPainter *p, const QPointF &pt, int sz ) { KadasAnnotationRotation::renderHandle( p, pt, sz ); } } );
  }
  return result;
}

QList<QList<QgsPointXY>> KadasLineTextAnnotationController::editGuide( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const QgsCurve *curve = asLineText( item )->geometry();
  if ( !curve || curve->numPoints() < 2 )
    return {};
  QList<QgsPointXY> polyline;
  const int n = curve->numPoints();
  polyline.reserve( n );
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    polyline.append( toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) );
  }
  return { polyline };
}

bool KadasLineTextAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Seed with two coincident points so setCurrentPoint() can rubber-band the trailing one.
  auto *ls = new QgsLineString();
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  asLineText( item )->setGeometry( ls );
  return true;
}

bool KadasLineTextAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasLineTextAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  QgsLineString *ls = takeMutableLine( asLineText( item ) );
  const int n = ls->numPoints();
  if ( n == 0 )
    return;
  const QgsPointXY ip = toItemPos( p, ctx );
  ls->moveVertex( QgsVertexId( 0, 0, n - 1 ), QgsPoint( ip.x(), ip.y() ) );
}

void KadasLineTextAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasLineTextAnnotationController::continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext & )
{
  QgsLineString *ls = takeMutableLine( asLineText( item ) );
  const int n = ls->numPoints();
  // If current (trailing) point coincides with the previous one, drop it and finish.
  if ( n > 2 && ls->pointN( n - 1 ) == ls->pointN( n - 2 ) )
  {
    ls->deleteVertex( QgsVertexId( 0, 0, n - 1 ) );
    return false;
  }
  if ( n == 0 )
    return true;
  // Append a duplicate of the trailing point as the new rubber-band point.
  ls->addVertex( ls->pointN( n - 1 ) );
  return true;
}

void KadasLineTextAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasLineTextAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasLineTextAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasLineTextAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasLineTextAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsCurve *curve = asLineText( item )->geometry();
  if ( !curve )
    return KadasEditContext();
  // Any hover hit-test means we are no longer mid-rotation; draw the handle at
  // rest again (a drag never calls getEditContext, it goes straight to edit()).
  mRotation.deactivate();
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    const QgsPointXY mp = toMapPos( QgsPointXY( p.x(), p.y() ), ctx );
    if ( pos.sqrDist( mp ) < pickTolSqr( ctx ) )
    {
      return KadasEditContext( QgsVertexId( 0, 0, i ), mp, drawAttribs() );
    }
  }
  // Rotation handle: snapshot the line (map coords) and pivot for a drift-free drag.
  if ( n >= 2 )
  {
    const QgsPointXY centerMap = centroidMap( curve, ctx );
    const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
    const QgsPointXY handle = KadasAnnotationRotation::VertexRotationState::restHandle( centerMap, off );
    if ( pos.sqrDist( handle ) < pickTolSqr( ctx ) )
    {
      QVector<QgsPointXY> verticesMap;
      verticesMap.reserve( n );
      for ( int i = 0; i < n; ++i )
      {
        const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
        verticesMap.append( toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) );
      }
      mRotation.begin( verticesMap, centerMap, handle );
      KadasAttribDefs rot;
      rot.insert( AttrAngle, KadasNumericAttribute { "angle", KadasNumericAttribute::Type::TypeAngle } );
      return KadasEditContext( QgsVertexId( kPartRotate, 0, 0 ), handle, rot, Qt::CrossCursor );
    }
  }
  // Fall back: a hit close to any segment selects the whole line for
  // translation, anchored at the first vertex. A bbox check would be wrong
  // (a diagonal line's bbox covers vast empty space), so require a segment hit.
  if ( n >= 2 )
  {
    // Widen the pick band perpendicular to the line so the rendered text
    // (which runs along, and may be offset from, the invisible line) is itself
    // clickable, not just the hairline geometry beneath it.
    QgsRenderContext rc = QgsRenderContext::fromMapSettings( ctx.mapSettings() );
    const double dpr = std::max( 1.0, static_cast<double>( rc.devicePixelRatio() ) );
    const QgsTextFormat fmt = asLineText( item )->format();
    const double fontPx = rc.convertToPainterUnits( fmt.size(), fmt.sizeUnit() ) / dpr;
    const double offsetPx = rc.convertToPainterUnits( asLineText( item )->offsetFromLine(), asLineText( item )->offsetFromLineUnit() ) / dpr;
    const double bandTol = ( 0.5 * fontPx + std::abs( offsetPx ) ) * ctx.mapSettings().mapUnitsPerPixel();
    const double tol = std::max( std::sqrt( pickTolSqr( ctx ) ), bandTol );
    const double tolSqr = tol * tol;
    for ( int i = 1; i < n; ++i )
    {
      const QgsPoint a = curve->vertexAt( QgsVertexId( 0, 0, i - 1 ) );
      const QgsPoint b = curve->vertexAt( QgsVertexId( 0, 0, i ) );
      const QgsPointXY am = toMapPos( QgsPointXY( a.x(), a.y() ), ctx );
      const QgsPointXY bm = toMapPos( QgsPointXY( b.x(), b.y() ), ctx );
      double dx = 0;
      double dy = 0;
      const double d2 = QgsGeometryUtilsBase::sqrDistToLine( pos.x(), pos.y(), am.x(), am.y(), bm.x(), bm.y(), dx, dy, 4 * std::numeric_limits<double>::epsilon() );
      if ( d2 < tolSqr )
      {
        const QgsPoint p0 = curve->vertexAt( QgsVertexId( 0, 0, 0 ) );
        const QgsPointXY refPos = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
        // Mark Precise so the canvas picker prefers this stroke hit over a higher-z polygon body.
        KadasEditContext ec( QgsVertexId(), refPos, KadasAttribDefs(), Qt::ArrowCursor );
        ec.precision = KadasEditContext::HitPrecision::Precise;
        return ec;
      }
    }
  }
  return KadasEditContext();
}

void KadasLineTextAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineTextItem *line = asLineText( item );
  if ( editContext.vidx.part == kPartRotate )
  {
    if ( !mRotation.hasSnapshot() )
      return;
    QgsLineString *ls = takeMutableLine( line );
    if ( !ls || ls->numPoints() != mRotation.vertexCount() )
      return;
    const QVector<QgsPointXY> rotated = mRotation.dragTo( newPoint, ctx.modifiers() & Qt::ShiftModifier );
    for ( int i = 0; i < rotated.size(); ++i )
    {
      const QgsPointXY ip = toItemPos( rotated[i], ctx );
      ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( ip.x(), ip.y() ) );
    }
    return;
  }
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  if ( editContext.vidx.vertex >= 0 && editContext.vidx.vertex < n )
  {
    const QgsPointXY ip = toItemPos( newPoint, ctx );
    ls->moveVertex( QgsVertexId( 0, 0, editContext.vidx.vertex ), QgsPoint( ip.x(), ip.y() ) );
  }
  else if ( n > 0 )
  {
    // Whole-geometry move: shift every vertex by (newPoint - refMapPos).
    const QgsPoint p0 = ls->pointN( 0 );
    const QgsPointXY refMap = toMapPos( QgsPointXY( p0.x(), p0.y() ), ctx );
    const double dxMap = newPoint.x() - refMap.x();
    const double dyMap = newPoint.y() - refMap.y();
    for ( int i = 0; i < n; ++i )
    {
      const QgsPoint pi = ls->pointN( i );
      const QgsPointXY mp = toMapPos( QgsPointXY( pi.x(), pi.y() ), ctx );
      const QgsPointXY shifted = toItemPos( QgsPointXY( mp.x() + dxMap, mp.y() + dyMap ), ctx );
      ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( shifted.x(), shifted.y() ) );
    }
  }
}

void KadasLineTextAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  if ( editContext.vidx.part == kPartRotate )
  {
    if ( !mRotation.hasSnapshot() )
      return;
    QgsLineString *ls = takeMutableLine( asLineText( item ) );
    if ( !ls || ls->numPoints() != mRotation.vertexCount() )
      return;
    const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
    const QVector<QgsPointXY> rotated = mRotation.applyAngle( values[AttrAngle], off );
    for ( int i = 0; i < rotated.size(); ++i )
    {
      const QgsPointXY ip = toItemPos( rotated[i], ctx );
      ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( ip.x(), ip.y() ) );
    }
    return;
  }
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasLineTextAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.part == kPartRotate )
  {
    KadasAttribValues v;
    v.insert( AttrAngle, mRotation.angleFromCursor( pos ) );
    return v;
  }
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasLineTextAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.part == kPartRotate )
  {
    const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
    return mRotation.handleForAngle( values[AttrAngle], off );
  }
  return positionFromDrawAttribs( item, values, ctx );
}

void KadasLineTextAnnotationController::populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineTextItem *line = asLineText( item );
  const QgsCurve *curve = line->geometry();
  if ( !curve || curve->numPoints() < 2 )
    return;
  // The text follows the line direction, so it renders upside down when the line
  // runs right-to-left. Reversing the geometry flips the text upright (and is its
  // own inverse, so a second reversal restores the original).
  QPointer<QgsAnnotationLayer> layerPtr( ctx.layer() );
  QAction *reverse = menu->addAction( QObject::tr( "Reverse text direction" ) );
  QObject::connect( reverse, &QAction::triggered, reverse, [line, layerPtr]() {
    const QgsCurve *c = line->geometry();
    if ( !c )
      return;
    line->setGeometry( c->reversed() );
    if ( layerPtr )
      layerPtr->triggerRepaint();
  } );
}

QgsPointXY KadasLineTextAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsCurve *curve = asLineText( item )->geometry();
  if ( !curve || curve->numPoints() == 0 )
    return QgsPointXY();
  const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, 0 ) );
  return QgsPointXY( p.x(), p.y() );
}

void KadasLineTextAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  QgsLineString *ls = takeMutableLine( asLineText( item ) );
  const int n = ls->numPoints();
  if ( n == 0 )
    return;
  const QgsPoint p0 = ls->pointN( 0 );
  const double dx = pos.x() - p0.x();
  const double dy = pos.y() - p0.y();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ls->pointN( i );
    ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

void KadasLineTextAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsLineString *ls = takeMutableLine( asLineText( item ) );
  const int n = ls->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ls->pointN( i );
    ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

void KadasLineTextAnnotationController::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  auto *lt = dynamic_cast<QgsAnnotationLineTextItem *>( item );
  if ( !lt || !settingsColor->exists() )
    return;
  QgsTextFormat fmt = lt->format();
  fmt.setSize( settingsSize->value() );
  fmt.setSizeUnit( Qgis::RenderUnit::Points );
  fmt.setColor( settingsColor->value() );
  QgsTextBufferSettings buf = fmt.buffer();
  const double bw = settingsBufferWidth->value();
  const QColor bc = settingsBufferColor->value();
  buf.setEnabled( bw > 0.0 && bc.alpha() > 0 );
  buf.setColor( bc );
  buf.setSize( bw );
  buf.setSizeUnit( Qgis::RenderUnit::Millimeters );
  fmt.setBuffer( buf );
  lt->setFormat( fmt );
}

void KadasLineTextAnnotationController::persistStyle( const QgsAnnotationItem *item ) const
{
  const auto *lt = dynamic_cast<const QgsAnnotationLineTextItem *>( item );
  if ( !lt )
    return;
  const QgsTextFormat fmt = lt->format();
  settingsSize->setValue( fmt.size() );
  settingsColor->setValue( fmt.color() );
  const QgsTextBufferSettings buf = fmt.buffer();
  settingsBufferColor->setValue( buf.enabled() ? buf.color() : QColor( 0, 0, 0, 0 ) );
  settingsBufferWidth->setValue( buf.enabled() ? buf.size() : 0.0 );
}

KadasAnnotationStyleEditor *KadasLineTextAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasLineTextStyleEditor( parent );
}
