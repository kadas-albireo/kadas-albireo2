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

#include <QObject>
#include <QPainter>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgspointxy.h>

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

KadasEditContext KadasMilxAnnotationController::getEditContext( const QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & ) const
{
  // TODO slice 37: libmss hit-test on control points.
  return KadasEditContext();
}

void KadasMilxAnnotationController::edit( QgsAnnotationItem *, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext & )
{}

void KadasMilxAnnotationController::edit( QgsAnnotationItem *, const KadasEditContext &, const KadasAttribValues &, const KadasAnnotationItemContext & )
{}

KadasAttribValues KadasMilxAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext & ) const
{
  return {};
}

QgsPointXY KadasMilxAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *, const KadasEditContext &, const KadasAttribValues &, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY();
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

QString KadasMilxAnnotationController::asKml( const QgsAnnotationItem *, const QgsCoordinateReferenceSystem &, const QgsRenderContext &, QuaZip * ) const
{
  // TODO slice 38: render symbol → KMZ image placemark.
  return QString();
}
