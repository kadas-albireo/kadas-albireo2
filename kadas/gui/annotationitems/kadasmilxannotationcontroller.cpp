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

bool KadasMilxAnnotationController::startPart( QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & )
{
  // TODO slice 36: libmss IPC for new-symbol creation.
  return false;
}

bool KadasMilxAnnotationController::startPart( QgsAnnotationItem *, const KadasAttribValues &, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasMilxAnnotationController::setCurrentPoint( QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & )
{}

void KadasMilxAnnotationController::setCurrentAttributes( QgsAnnotationItem *, const KadasAttribValues &, const KadasAnnotationItemContext & )
{}

bool KadasMilxAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasMilxAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasMilxAnnotationController::drawAttribs() const
{
  return {};
}

KadasAttribValues KadasMilxAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & ) const
{
  return {};
}

QgsPointXY KadasMilxAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY();
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
  for ( QgsPointXY &p : pts )
    p.set( p.x() + dx, p.y() + dy );
  milx->setPoints( pts );
}

void KadasMilxAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  auto *milx = static_cast<KadasMilxAnnotationItem *>( item );
  QList<QgsPointXY> pts = milx->points();
  for ( QgsPointXY &p : pts )
    p.set( p.x() + dx, p.y() + dy );
  milx->setPoints( pts );
}

QString KadasMilxAnnotationController::asKml( const QgsAnnotationItem *, const QgsCoordinateReferenceSystem &, const QgsRenderContext &, QuaZip * ) const
{
  // TODO slice 38: render symbol → KMZ image placemark.
  return QString();
}
