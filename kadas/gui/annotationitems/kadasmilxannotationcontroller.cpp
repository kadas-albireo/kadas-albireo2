/***************************************************************************
    kadasmilxannotationcontroller.cpp
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

#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QTextStream>
#include <QUuid>
#include <limits>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsunittypes.h>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <quazip/quazipnewinfo.h>

#include "kadas/gui/annotationitems/kadasmilxannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "kadas/gui/milx/kadasmilxclient.h"


namespace
{
  // Mirrors the legacy KadasMilxItem node renderers so the on-canvas
  // appearance of MilX nodes does not change when the controller takes over.
  void posPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
  {
    painter->setPen( QPen( Qt::black, 1 ) );
    painter->setBrush( Qt::yellow );
    painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
  }

  void ctrlPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
  {
    painter->setPen( QPen( Qt::black, 1 ) );
    painter->setBrush( Qt::red );
    painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
  }
} // namespace


QString KadasMilxAnnotationController::itemType() const
{
  return KadasMilxAnnotationItem::itemTypeId();
}

QString KadasMilxAnnotationController::itemName() const
{
  return QObject::tr( "MilX Symbol" );
}

QgsAnnotationItem *KadasMilxAnnotationController::createItem() const
{
  return new KadasMilxAnnotationItem();
}

QList<KadasNode> KadasMilxAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
  const QList<QgsPointXY> &pts = milx->points();
  if ( pts.isEmpty() || milx->mssString().isEmpty() )
    return {};

  // Ask libmss which point indices are draggable control points; the rest
  // are rendered as plain position nodes (matches legacy KadasMilxItem::nodes).
  QList<int> controlIndices;
  KadasMilxClient::getControlPointIndices( milx->mssString(), pts.size(), KadasMilxClient::globalSymbolSettings(), controlIndices );

  // MilX points are stored in EPSG:4326; project them to the map CRS for display.
  const QgsCoordinateTransform xform( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );

  QList<KadasNode> result;
  result.reserve( pts.size() );
  for ( int i = 0; i < pts.size(); ++i )
  {
    QgsPointXY mapPt = pts[i];
    try
    {
      mapPt = xform.transform( pts[i] );
    }
    catch ( const QgsCsException & )
    {
      continue;
    }
    const auto renderer = controlIndices.contains( i ) ? &ctrlPointNodeRenderer : &posPointNodeRenderer;
    result.append( { mapPt, renderer } );
  }
  return result;
}

bool KadasMilxAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( milx->mssString().isEmpty() )
    return false;

  // First click: append the user point in item CRS (4326), bump the
  // pressed-points counter, then ask libmss to materialize the symbol
  // (which usually back-fills control points up to mMinNumPoints).
  milx->setDrawStatus( KadasMilxAnnotationItem::DrawStatus::Drawing );
  QList<QgsPointXY> pts = milx->points();
  pts.append( toItemPos( firstPoint, ctx ) );
  milx->setPoints( pts );
  milx->setPressedPoints( 1 );

  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
  KadasMilxClient::NPointSymbolGraphic result;
  KadasMilxClient::updateSymbol(
    KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() ),
    ctx.mapSettings().outputDpi(),
    symbol,
    KadasMilxClient::globalSymbolSettings(),
    result,
    /* returnPoints */ true
  );
  milx->applySymbolResult( ctx.mapSettings(), result );

  return milx->pressedPoints() < milx->minNumPoints() || milx->hasVariablePoints();
}

bool KadasMilxAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, positionFromDrawAttribs( item, values, ctx ), ctx );
}

void KadasMilxAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( milx->mssString().isEmpty() || milx->drawStatus() != KadasMilxAnnotationItem::DrawStatus::Drawing )
    return;

  // Live preview: ask libmss to move the next non-pressed slot to the
  // current cursor position. The slot index equals pressedPoints (matches
  // the legacy KadasMilxItem semantics).
  const QPoint screenPoint = ctx.mapSettings().mapToPixel().transform( p ).toQPointF().toPoint();
  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
  KadasMilxClient::NPointSymbolGraphic result;
  if ( KadasMilxClient::movePoint( KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() ), ctx.mapSettings().outputDpi(), symbol, milx->pressedPoints(), screenPoint, KadasMilxClient::globalSymbolSettings(), result ) )
  {
    milx->applySymbolResult( ctx.mapSettings(), result );
  }
}

void KadasMilxAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, positionFromDrawAttribs( item, values, ctx ), ctx );
}

bool KadasMilxAnnotationController::continuePart( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  milx->setPressedPoints( milx->pressedPoints() + 1 );

  // Once the minimum number of points has been clicked, additional clicks
  // append a new point to the symbol (only meaningful for variable-point
  // symbols; libmss otherwise stops at minNumPoints).
  if ( milx->pressedPoints() >= milx->minNumPoints() && milx->hasVariablePoints() )
  {
    const QList<QgsPointXY> pts = milx->points();
    const QList<int> ctrl = milx->controlPoints();
    int index = pts.size() - 1;
    while ( index >= 0 && ctrl.contains( index ) )
      --index;
    if ( index >= 0 )
    {
      const QgsCoordinateTransform xform( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
      try
      {
        const QPoint screenPoint = ctx.mapSettings().mapToPixel().transform( xform.transform( pts[index] ) ).toQPointF().toPoint();
        KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
        KadasMilxClient::NPointSymbolGraphic result;
        if ( KadasMilxClient::appendPoint( KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() ), ctx.mapSettings().outputDpi(), symbol, screenPoint, KadasMilxClient::globalSymbolSettings(), result ) )
        {
          milx->applySymbolResult( ctx.mapSettings(), result );
        }
      }
      catch ( const QgsCsException & )
      {}
    }
  }
  return milx->pressedPoints() < milx->minNumPoints() || milx->hasVariablePoints();
}

void KadasMilxAnnotationController::endPart( QgsAnnotationItem *item )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( !milx->mssString().isEmpty() )
  {
    milx->setDrawStatus( KadasMilxAnnotationItem::DrawStatus::Finished );
  }
}

KadasAttribDefs KadasMilxAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasMilxAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasMilxAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasMilxAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
  const QgsCoordinateTransform xform( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() );
  const double tolSqr = pickTolSqr( ctx );

  // 1) Geometry node hit test (ring 0, vertex = point index).
  const QList<QgsPointXY> &pts = milx->points();
  for ( int i = 0; i < pts.size(); ++i )
  {
    QgsPointXY mapPos;
    try
    {
      mapPos = xform.transform( pts[i] );
    }
    catch ( const QgsCsException & )
    {
      continue;
    }
    if ( pos.sqrDist( mapPos ) < tolSqr )
    {
      return KadasEditContext( QgsVertexId( 0, 0, i ), mapPos, drawAttribs() );
    }
  }

  // 2) Attribute control point hit test (ring 1, vertex = libmss attr id).
  const QMap<KadasMilxAttrType, QgsPointXY> attrPts = milx->attributePoints();
  for ( auto it = attrPts.cbegin(), itEnd = attrPts.cend(); it != itEnd; ++it )
  {
    QgsPointXY mapPos;
    try
    {
      mapPos = xform.transform( it.value() );
    }
    catch ( const QgsCsException & )
    {
      continue;
    }
    if ( pos.sqrDist( mapPos ) < tolSqr )
    {
      const double min = it.key() == MilxAttributeAttitude ? std::numeric_limits<double>::lowest() : 0;
      const double max = std::numeric_limits<double>::max();
      const int decimals = it.key() == MilxAttributeAttitude ? 1 : 0;
      KadasNumericAttribute::Type type = KadasNumericAttribute::Type::TypeOther;
      if ( it.key() == MilxAttributeLength || it.key() == MilxAttributeWidth || it.key() == MilxAttributeRadius )
        type = KadasNumericAttribute::Type::TypeDistance;
      else if ( it.key() == MilxAttributeAttitude )
        type = KadasNumericAttribute::Type::TypeAngle;
      KadasAttribDefs attrs;
      attrs.insert( it.key(), KadasNumericAttribute { KadasMilxClient::attributeName( it.key() ), type, min, max, decimals } );
      return KadasEditContext( QgsVertexId( 0, 1, it.key() ), mapPos, attrs );
    }
  }

  // 3) Whole-symbol drag fallback (vidx invalid). Anchor on the first
  //    point — for single-point symbols, shift the anchor by the user
  //    offset so the drag visually starts on the rendered glyph.
  if ( hitTest( item, pos, ctx ) && !pts.isEmpty() )
  {
    QgsPointXY refMap;
    try
    {
      refMap = xform.transform( pts.front() );
    }
    catch ( const QgsCsException & )
    {
      return KadasEditContext();
    }
    if ( !milx->isMultiPoint() )
    {
      const QPoint anchorScreen = ctx.mapSettings().mapToPixel().transform( refMap ).toQPointF().toPoint();
      refMap = ctx.mapSettings().mapToPixel().toMapCoordinates( anchorScreen + milx->userOffset() );
    }
    return KadasEditContext( QgsVertexId(), refMap, KadasAttribDefs(), Qt::ArrowCursor );
  }
  return KadasEditContext();
}

void KadasMilxAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );

  if ( editContext.vidx.isValid() )
  {
    // Move a single node.
    if ( milx->isMultiPoint() )
    {
      const QPoint screenPoint = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF().toPoint();
      KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
      KadasMilxClient::NPointSymbolGraphic result;
      const QRect screenRect = KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() );
      const int dpi = ctx.mapSettings().outputDpi();
      if ( editContext.vidx.ring == 0 )
      {
        if ( KadasMilxClient::movePoint( screenRect, dpi, symbol, editContext.vidx.vertex, screenPoint, KadasMilxClient::globalSymbolSettings(), result ) )
          milx->applySymbolResult( ctx.mapSettings(), result );
      }
      else if ( editContext.vidx.ring == 1 )
      {
        if ( KadasMilxClient::moveAttributePoint( screenRect, dpi, symbol, editContext.vidx.vertex, screenPoint, KadasMilxClient::globalSymbolSettings(), result ) )
          milx->applySymbolResult( ctx.mapSettings(), result );
      }
    }
    else
    {
      // Single-point symbols: bypass libmss and update the geometry directly.
      const QgsPointXY itemPos = toItemPos( newPoint, ctx );
      milx->setPoints( { itemPos } );
    }
  }
  else if ( milx->isMultiPoint() )
  {
    // Whole-symbol drag (multipoint): translate every geometry / attribute
    // point by the same map-space delta. No libmss round-trip needed.
    if ( milx->points().isEmpty() )
      return;
    const QgsPointXY refMap = toMapPos( milx->points().front(), ctx );
    const double mdx = newPoint.x() - refMap.x();
    const double mdy = newPoint.y() - refMap.y();
    QList<QgsPointXY> pts = milx->points();
    for ( QgsPointXY &p : pts )
    {
      const QgsPointXY mp = toMapPos( p, ctx );
      p = toItemPos( QgsPointXY( mp.x() + mdx, mp.y() + mdy ), ctx );
    }
    milx->setPoints( pts );
    QMap<KadasMilxAttrType, QgsPointXY> aps = milx->attributePoints();
    for ( auto it = aps.begin(), itEnd = aps.end(); it != itEnd; ++it )
    {
      const QgsPointXY mp = toMapPos( it.value(), ctx );
      it.value() = toItemPos( QgsPointXY( mp.x() + mdx, mp.y() + mdy ), ctx );
    }
    milx->setAttributePoints( aps );
  }
  else
  {
    // Whole-symbol drag (single-point): record screen-space user offset.
    if ( milx->points().isEmpty() )
      return;
    const QPoint anchorScreen = ctx.mapSettings().mapToPixel().transform( toMapPos( milx->points().front(), ctx ) ).toQPointF().toPoint();
    const QPoint newScreen = ctx.mapSettings().mapToPixel().transform( newPoint ).toQPointF().toPoint();
    milx->setUserOffset( newScreen - anchorScreen );
  }
}

void KadasMilxAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( values.size() == 1 )
  {
    // Single shape attribute (length / width / radius / attitude): replace
    // the stored value, then ask libmss to recompute the symbol graphic.
    const KadasMilxAttrType attr = static_cast<KadasMilxAttrType>( values.firstKey() );
    QMap<KadasMilxAttrType, double> attrs = milx->attributes();
    attrs[attr] = values[values.firstKey()];
    milx->setAttributes( attrs );

    KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
    KadasMilxClient::NPointSymbolGraphic result;
    KadasMilxClient::updateSymbol(
      KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() ),
      ctx.mapSettings().outputDpi(),
      symbol,
      KadasMilxClient::globalSymbolSettings(),
      result,
      /* returnPoints */ true
    );
    milx->applySymbolResult( ctx.mapSettings(), result );
  }
  else
  {
    edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
  }
}

KadasAttribValues KadasMilxAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  if ( editContext.attributes.size() == 1 )
  {
    // Single shape attribute: report the current stored value, not a
    // position-derived one (the input dialog binds to the attribute id key).
    const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
    const KadasMilxAttrType attr = static_cast<KadasMilxAttrType>( editContext.attributes.firstKey() );
    KadasAttribValues values;
    values.insert( attr, milx->attributes().value( attr, 0.0 ) );
    return values;
  }
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasMilxAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  if ( values.size() == 1 )
  {
    const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
    const KadasMilxAttrType attr = static_cast<KadasMilxAttrType>( values.firstKey() );
    const QgsPointXY itemPos = milx->attributePoints().value( attr );
    return toMapPos( itemPos, ctx );
  }
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasMilxAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
  if ( milx->points().isEmpty() )
    return QgsPointXY();
  return milx->points().front();
}

void KadasMilxAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  QList<QgsPointXY> pts = milx->points();
  if ( pts.isEmpty() )
  {
    milx->setPoints( { pos } );
    return;
  }
  const QgsPointXY anchor = pts.front();
  const double dx = pos.x() - anchor.x();
  const double dy = pos.y() - anchor.y();
  translate( milx, dx, dy );
}

void KadasMilxAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  QList<QgsPointXY> pts = milx->points();
  for ( QgsPointXY &p : pts )
    p.set( p.x() + dx, p.y() + dy );
  milx->setPoints( pts );
  // Attribute points (libmss-managed handles for width/length/etc.) live in
  // the same CRS as the geometry points; translate them in lockstep.
  QMap<KadasMilxAttrType, QgsPointXY> aps = milx->attributePoints();
  for ( auto it = aps.begin(), itEnd = aps.end(); it != itEnd; ++it )
  {
    it.value().set( it.value().x() + dx, it.value().y() + dy );
  }
  milx->setAttributePoints( aps );
}

QString KadasMilxAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &renderContext, QuaZip *kmzZip ) const
{
  // Mirrors the legacy KadasMilxItem::asKml: render the symbol at a fixed
  // world-wide extent, store the PNG inside the KMZ, then emit either a
  // <Placemark>+<StyleMap> (single-point) or a <GroundOverlay> (multipoint).
  if ( !kmzZip )
    return QString();

  const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
  if ( milx->mssString().isEmpty() || milx->points().isEmpty() )
    return QString();

  // Build a transient QgsMapSettings that mimics the legacy export render
  // context (world extent in EPSG:4326). Reusing item-level helpers keeps
  // the libmss IPC path identical to the on-canvas one.
  const QgsRectangle worldExtent( -180., -90., 180., 90. );
  const double factor = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Degrees, Qgis::DistanceUnit::Meters ) * renderContext.scaleFactor() * 1000 / renderContext.rendererScale();
  QgsMapSettings ms;
  ms.setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ) );
  ms.setExtent( worldExtent );
  ms.setOutputSize( QSize( static_cast<int>( worldExtent.width() * factor ), static_cast<int>( worldExtent.height() * factor ) ) );
  ms.setOutputDpi( renderContext.painter() && renderContext.painter()->device() ? renderContext.painter()->device()->logicalDpiX() : 96 );
  ms.setTransformContext( renderContext.transformContext() );

  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ms );
  KadasMilxClient::NPointSymbolGraphic result;
  if ( !KadasMilxClient::updateSymbol( KadasMilxAnnotationItem::computeScreenExtent( ms ), ms.outputDpi(), symbol, KadasMilxClient::globalSymbolSettings(), result, /* returnPoints */ true ) )
  {
    return QString();
  }

  QString fileName = QUuid::createUuid().toString();
  fileName = fileName.mid( 1, fileName.length() - 2 ) + QStringLiteral( ".png" );
  QuaZipFile outputFile( kmzZip );
  QuaZipNewInfo info( fileName );
  info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
  if ( !outputFile.open( QIODevice::WriteOnly, info ) || !result.graphic.save( &outputFile, "PNG" ) )
  {
    return QString();
  }

  // Item geometry is always EPSG:4326; the itemCrs argument is informational
  // here (and matches itemCrs of the parent annotation layer in practice).
  QgsPoint pos( milx->points().front() );
  if ( itemCrs.isValid() && itemCrs != QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ) )
  {
    pos.transform( QgsCoordinateTransform( itemCrs, QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ), QgsProject::instance() ) );
  }

  QString outString;
  QTextStream outStream( &outString );

  if ( !milx->isMultiPoint() )
  {
    const double hotSpotX = -result.offset.x();
    const double hotSpotY = -result.offset.y();
    QString id = QUuid::createUuid().toString();
    id = id.mid( 1, id.length() - 2 );
    outStream << "<StyleMap id=\"" << id << "\">\n";
    for ( const QString &key : { QStringLiteral( "normal" ), QStringLiteral( "highlight" ) } )
    {
      outStream << "  <Pair>\n    <key>" << key << "</key>\n    <Style>\n      <IconStyle>\n";
      outStream << "        <scale>1.0</scale>\n";
      outStream << "        <Icon><href>" << fileName << "</href></Icon>\n";
      outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />\n";
      outStream << "      </IconStyle>\n    </Style>\n  </Pair>\n";
    }
    outStream << "</StyleMap>\n";
    outStream << "<Placemark>\n";
    outStream << "  <name>" << milx->militaryName() << "</name>\n";
    outStream << "  <styleUrl>#" << id << "</styleUrl>\n";
    outStream << "  <Point>\n";
    outStream << "    <coordinates>" << QString::number( pos.x(), 'f', 10 ) << "," << QString::number( pos.y(), 'f', 10 ) << ",0</coordinates>\n";
    outStream << "  </Point>\n";
    outStream << "</Placemark>\n";
  }
  else
  {
    const QPoint offset = result.adjustedPoints.front() + result.offset;
    const QgsPointXY pNW = ms.mapToPixel().toMapCoordinates( offset.x(), offset.y() );
    const QgsPointXY pSE = ms.mapToPixel().toMapCoordinates( offset.x() + result.graphic.width(), offset.y() + result.graphic.height() );
    outStream << "<GroundOverlay>\n";
    outStream << "<name>" << milx->militaryName() << "</name>\n";
    outStream << "<Icon><href>" << fileName << "</href></Icon>\n";
    outStream << "<LatLonBox>\n";
    outStream << "<north>" << pNW.y() << "</north>\n";
    outStream << "<south>" << pSE.y() << "</south>\n";
    outStream << "<east>" << pSE.x() << "</east>\n";
    outStream << "<west>" << pNW.x() << "</west>\n";
    outStream << "</LatLonBox>\n";
    outStream << "</GroundOverlay>\n";
  }
  outStream.flush();
  return outString;
}

namespace
{
  // Pick the topmost main window, used as parent for libmss' modal symbol
  // editor dialog (mirrors the legacy KadasMilxItem behavior).
  WId mainWindowWid()
  {
    for ( QWidget *widget : QApplication::topLevelWidgets() )
    {
      if ( qobject_cast<QMainWindow *>( widget ) )
        return widget->effectiveWinId();
    }
    return 0;
  }
} // namespace

void KadasMilxAnnotationController::populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &editContext, const QgsPointXY &clickPos, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( milx->mssString().isEmpty() )
    return;

  const QRect screenRect = KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() );
  const int dpi = ctx.mapSettings().outputDpi();
  const QPoint screenPos = ctx.mapSettings().mapToPixel().transform( clickPos ).toQPointF().toPoint();
  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );

  // Symbol editor (libmss modal dialog) — also wired up on double-click.
  menu->addAction( QIcon( QStringLiteral( ":/kadas/icons/editor" ) ), QObject::tr( "Symbol editor..." ), [milx, &ctxRef = ctx, screenRect, dpi, symbol]() mutable {
    KadasMilxClient::NPointSymbolGraphic result;
    QString newMssString = milx->mssString();
    QString newMilitaryName = milx->militaryName();
    if ( KadasMilxClient::editSymbol( screenRect, dpi, symbol, newMssString, newMilitaryName, KadasMilxClient::globalSymbolSettings(), result, mainWindowWid() ) )
    {
      milx->setMssString( newMssString );
      milx->setMilitaryName( newMilitaryName );
      milx->applySymbolResult( ctxRef.mapSettings(), result );
    }
  } );

  if ( milx->isMultiPoint() )
  {
    if ( editContext.vidx.vertex >= 0 )
    {
      // Right-click on an existing node → delete (if libmss says it's removable).
      QAction *actionDeletePoint = menu->addAction( QIcon( QStringLiteral( ":/kadas/icons/delete_node" ) ), QObject::tr( "Delete node" ), [milx, &ctxRef = ctx, screenRect, dpi, symbol, editContext]() mutable {
        KadasMilxClient::NPointSymbolGraphic result;
        if ( KadasMilxClient::deletePoint( screenRect, dpi, symbol, editContext.vidx.vertex, KadasMilxClient::globalSymbolSettings(), result ) )
          milx->applySymbolResult( ctxRef.mapSettings(), result );
      } );
      bool canDelete = false;
      actionDeletePoint->setEnabled( KadasMilxClient::canDeletePoint( symbol, KadasMilxClient::globalSymbolSettings(), editContext.vidx.vertex, canDelete ) && canDelete );
    }
    else
    {
      // Right-click on the symbol body → insert a node at the click position.
      menu->addAction( QIcon( QStringLiteral( ":/kadas/icons/add_node" ) ), QObject::tr( "Add node" ), [milx, &ctxRef = ctx, screenRect, dpi, symbol, screenPos]() mutable {
        KadasMilxClient::NPointSymbolGraphic result;
        if ( KadasMilxClient::insertPoint( screenRect, dpi, symbol, screenPos, KadasMilxClient::globalSymbolSettings(), result ) )
          milx->applySymbolResult( ctxRef.mapSettings(), result );
      } );
    }
  }
  else
  {
    // Single-point symbols carry a screen-space user offset; offer to reset.
    QAction *action = menu->addAction( QObject::tr( "Reset offset" ), [milx]() { milx->setUserOffset( QPoint() ); } );
    action->setEnabled( !milx->userOffset().isNull() );
  }
}

void KadasMilxAnnotationController::onDoubleClick( QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  if ( milx->mssString().isEmpty() )
    return;

  const QRect screenRect = KadasMilxAnnotationItem::computeScreenExtent( ctx.mapSettings() );
  const int dpi = ctx.mapSettings().outputDpi();
  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );

  KadasMilxClient::NPointSymbolGraphic result;
  QString newMssString = milx->mssString();
  QString newMilitaryName = milx->militaryName();
  if ( KadasMilxClient::editSymbol( screenRect, dpi, symbol, newMssString, newMilitaryName, KadasMilxClient::globalSymbolSettings(), result, mainWindowWid() ) )
  {
    milx->setMssString( newMssString );
    milx->setMilitaryName( newMilitaryName );
    milx->applySymbolResult( ctx.mapSettings(), result );
  }
}

bool KadasMilxAnnotationController::hitTest( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const auto *milx = static_cast<const KadasMilxAnnotationItem *>( item );
  if ( milx->points().isEmpty() || milx->mssString().isEmpty() )
    return false;

  // Defer the precise hit test to libmss — the rendered MilX glyph is
  // typically much larger than the convex hull of its control points,
  // so the default bounding-box test would miss most of the symbol.
  KadasMilxClient::NPointSymbol symbol = milx->toSymbol( ctx.mapSettings() );
  // toSymbol() doesn't throw — but a degenerate (empty xml/points) state is
  // still possible if controller is invoked on a half-built item.
  if ( symbol.xml.isEmpty() || symbol.points.isEmpty() )
    return false;
  // Account for the user-applied screen-space offset before asking libmss.
  for ( QPoint &p : symbol.points )
    p += milx->userOffset();

  const QPoint screenPos = ctx.mapSettings().mapToPixel().transform( pos ).toQPointF().toPoint();
  QList<KadasMilxClient::NPointSymbol> symbols { symbol };
  int selectedSymbol = -1;
  QRect bbox;
  return KadasMilxClient::pickSymbol( symbols, screenPos, KadasMilxClient::globalSymbolSettings(), selectedSymbol, bbox ) && selectedSymbol >= 0;
}
