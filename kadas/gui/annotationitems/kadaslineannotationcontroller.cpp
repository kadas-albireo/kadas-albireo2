/***************************************************************************
    kadaslineannotationcontroller.cpp
    ---------------------------------
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

#include <QObject>
#include <QTextStream>
#include <memory>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsdistancearea.h>
#include <qgis/qgsgeometryutils_base.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettingsentryimpl.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"


// ----- Persisted last-used style ----------------------------------------

const QgsSettingsEntryDouble *KadasLineAnnotationController::settingsWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "line-width" ), sTreeAnnotation, 0.5, QStringLiteral( "Last-used line width (mm)." ) );
const QgsSettingsEntryColor *KadasLineAnnotationController::settingsColor
  = new QgsSettingsEntryColor( QStringLiteral( "line-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used line color." ) );
const QgsSettingsEntryInteger *KadasLineAnnotationController::settingsStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "line-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidLine ), QStringLiteral( "Last-used line style." ) );

namespace
{
  inline QgsAnnotationLineItem *asLine( QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<QgsAnnotationLineItem *>( item ) );
    return static_cast<QgsAnnotationLineItem *>( item );
  }
  inline const QgsAnnotationLineItem *asLine( const QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<const QgsAnnotationLineItem *>( item ) );
    return static_cast<const QgsAnnotationLineItem *>( item );
  }

  //! Returns a mutable QgsLineString backing the item, cloning it via setGeometry() for in-place edits.
  QgsLineString *takeMutableLine( QgsAnnotationLineItem *item )
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


QString KadasLineAnnotationController::itemType() const
{
  return QStringLiteral( "linestring" );
}

QString KadasLineAnnotationController::itemName() const
{
  return QObject::tr( "Line" );
}

QgsAnnotationItem *KadasLineAnnotationController::createItem() const
{
  auto *item = new QgsAnnotationLineItem( new QgsLineString() );
  item->setZIndex( KadasAnnotationZIndex::Line );
  return item;
}

QList<KadasNode> KadasLineAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasNode> result;
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve )
    return result;
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    result.append( { toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) } );
  }
  return result;
}

bool KadasLineAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Seed with two coincident points so setCurrentPoint() can rubber-band the trailing one.
  auto *ls = new QgsLineString();
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  ls->addVertex( QgsPoint( ip.x(), ip.y() ) );
  asLine( item )->setGeometry( ls );
  return true;
}

bool KadasLineAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasLineAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  if ( n == 0 )
    return;
  const QgsPointXY ip = toItemPos( p, ctx );
  ls->moveVertex( QgsVertexId( 0, 0, n - 1 ), QgsPoint( ip.x(), ip.y() ) );
}

void KadasLineAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasLineAnnotationController::continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext & )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
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

void KadasLineAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasLineAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasLineAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasLineAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasLineAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve )
    return KadasEditContext();
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
  // Fall back: a hit close to any segment selects the whole line for
  // translation, anchored at the first vertex. A bbox check would be wrong
  // (a diagonal line's bbox covers vast empty space), so require a segment hit.
  if ( n >= 2 )
  {
    const double tolSqr = pickTolSqr( ctx );
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

void KadasLineAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationLineItem *line = asLine( item );
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

void KadasLineAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasLineAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasLineAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasLineAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve || curve->numPoints() == 0 )
    return QgsPointXY();
  const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, 0 ) );
  return QgsPointXY( p.x(), p.y() );
}

void KadasLineAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
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

void KadasLineAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsAnnotationLineItem *line = asLine( item );
  QgsLineString *ls = takeMutableLine( line );
  const int n = ls->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    const QgsPoint pi = ls->pointN( i );
    ls->moveVertex( QgsVertexId( 0, 0, i ), QgsPoint( pi.x() + dx, pi.y() + dy ) );
  }
}

QString KadasLineAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve || curve->numPoints() == 0 )
    return QString();

  QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<LineString>\n<coordinates>";
  const int n = curve->numPoints();
  for ( int i = 0; i < n; ++i )
  {
    QgsPointXY p( curve->vertexAt( QgsVertexId( 0, 0, i ) ) );
    p = ct.transform( p );
    if ( i > 0 )
      outStream << " ";
    outStream << QString::number( p.x(), 'f', 10 ) << "," << QString::number( p.y(), 'f', 10 );
  }
  outStream << "</coordinates>\n</LineString>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}

QList<KadasAnnotationMeasurementLabel> KadasLineAnnotationController::measurementLabels( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  QList<KadasAnnotationMeasurementLabel> labels;
  const QgsCurve *curve = asLine( item )->geometry();
  if ( !curve )
    return labels;
  const int n = curve->numPoints();
  if ( n < 2 )
    return labels;

  // Measurement uses the project ellipsoid in the item CRS; anchors are
  // transformed to the map CRS for on-canvas placement.
  QgsDistanceArea da;
  da.setSourceCrs( ctx.itemCrs(), ctx.mapSettings().transformContext() );
  da.setEllipsoid( QgsProject::instance()->ellipsoid() );

  double total = 0.0;
  for ( int i = 1; i < n; ++i )
  {
    const QgsPoint a = curve->vertexAt( QgsVertexId( 0, 0, i - 1 ) );
    const QgsPoint b = curve->vertexAt( QgsVertexId( 0, 0, i ) );
    const QgsPointXY ai( a.x(), a.y() );
    const QgsPointXY bi( b.x(), b.y() );
    if ( ai == bi )
      continue; // skip rubber-band trailing duplicate

    const double seg = da.measureLine( ai, bi );
    total += seg;

    const QgsPointXY midItem( 0.5 * ( a.x() + b.x() ), 0.5 * ( a.y() + b.y() ) );
    labels.append( { toMapPos( midItem, ctx ), formatLengthMeters( seg ), true } );
  }

  const QgsPoint last = curve->vertexAt( QgsVertexId( 0, 0, n - 1 ) );
  labels.append( { toMapPos( QgsPointXY( last.x(), last.y() ), ctx ), QObject::tr( "Tot.: %1" ).arg( formatLengthMeters( total ) ), false } );
  return labels;
}

void KadasLineAnnotationController::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  auto *line = dynamic_cast<QgsAnnotationLineItem *>( item );
  if ( !line || !settingsColor->exists() )
    return;
  std::unique_ptr<QgsLineSymbol> sym( line->symbol() ? line->symbol()->clone() : new QgsLineSymbol() );
  if ( sym->symbolLayerCount() == 0 )
    sym->appendSymbolLayer( new QgsSimpleLineSymbolLayer() );
  auto *sl = dynamic_cast<QgsSimpleLineSymbolLayer *>( sym->symbolLayer( 0 ) );
  if ( !sl )
  {
    auto *replacement = new QgsSimpleLineSymbolLayer();
    sym->changeSymbolLayer( 0, replacement );
    sl = replacement;
  }
  sl->setWidth( settingsWidth->value() );
  sl->setColor( settingsColor->value() );
  sl->setPenStyle( static_cast<Qt::PenStyle>( settingsStyle->value() ) );
  line->setSymbol( sym.release() );
}

void KadasLineAnnotationController::persistStyle( const QgsAnnotationItem *item ) const
{
  const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( item );
  if ( !line || !line->symbol() || line->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSimpleLineSymbolLayer *>( line->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  settingsWidth->setValue( sl->width() );
  settingsColor->setValue( sl->color() );
  settingsStyle->setValue( static_cast<int>( sl->penStyle() ) );
}

KadasAnnotationStyleEditor *KadasLineAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasLineStyleEditor( parent );
}
