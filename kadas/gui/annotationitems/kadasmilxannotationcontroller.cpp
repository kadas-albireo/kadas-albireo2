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

#include <qgis/qgspointxy.h>

#include "kadas/gui/annotationitems/kadasmilxannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"


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

QList<KadasNode> KadasMilxAnnotationController::nodes( const QgsAnnotationItem *, const KadasAnnotationItemContext & ) const
{
  // TODO slice 35: query KadasMilxClient for control points.
  return {};
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
