/***************************************************************************
    kadasselectionrectitem.cpp
    --------------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QImageReader>

#include <exiv2/exiv2.hpp>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

#include "kadas/gui/mapitems/kadasselectionrectitem.h"


QJsonObject KadasSelectionRectItem::State::serialize() const
{
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  return json;
}

bool KadasSelectionRectItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  return true;
}


KadasSelectionRectItem::KadasSelectionRectItem( const QgsCoordinateReferenceSystem &crs )
  : KadasMapItem( crs )
{
  clear();
}

void KadasSelectionRectItem::setSelectedItems( const QList<KadasMapItem *> &items )
{
  mItems = items;
  update();
}

QgsRectangle KadasSelectionRectItem::boundingBox() const
{
  if ( mItems.isEmpty() )
  {
    return QgsRectangle();
  }
  QgsCoordinateTransformContext tctx = QgsProject::instance()->transformContext();
  QgsRectangle rect = QgsCoordinateTransform( mItems.front()->crs(), crs(), tctx ).transformBoundingBox( mItems.front()->boundingBox() );
  for ( int i = 1, n = mItems.size(); i < n; ++i )
  {
    QgsRectangle itemRect = QgsCoordinateTransform( mItems[i]->crs(), crs(), tctx ).transformBoundingBox( mItems[i]->boundingBox() );
    rect.setXMinimum( std::min( rect.xMinimum(), itemRect.xMinimum() ) );
    rect.setYMinimum( std::min( rect.yMinimum(), itemRect.yMinimum() ) );
    rect.setXMaximum( std::max( rect.xMaximum(), itemRect.xMaximum() ) );
    rect.setYMaximum( std::max( rect.yMaximum(), itemRect.yMaximum() ) );
  }
  return rect;
}

KadasMapItem::Margin KadasSelectionRectItem::margin() const
{
  if ( mItems.isEmpty() )
  {
    return Margin();
  }
  Margin m = mItems.front()->margin();
  for ( int i = 1, n = mItems.size(); i < n; ++i )
  {
    Margin im = mItems[i]->margin();
    m.left = std::max( m.left, im.left );
    m.right = std::max( m.right, im.right );
    m.top = std::max( m.top, im.top );
    m.bottom = std::max( m.bottom, im.bottom );
  }
  return m;
}

QgsRectangle KadasSelectionRectItem::itemsRect( const QgsCoordinateReferenceSystem &mapCrs, double mup ) const
{
  if ( mItems.isEmpty() )
  {
    return KadasMapRect();
  }
  bool empty = true;
  KadasMapRect rect;
  QgsCoordinateTransformContext tctx = QgsProject::instance()->transformContext();
  for ( const KadasMapItem *item : mItems )
  {
    QgsRectangle itemRect = QgsCoordinateTransform( item->crs(), mapCrs, tctx ).transformBoundingBox( item->boundingBox() );
    Margin m = item->margin();
    if ( empty )
    {
      rect = KadasMapRect( itemRect.xMaximum(), itemRect.yMinimum(), itemRect.xMaximum(), itemRect.yMaximum() );
      empty = false;
    }
    rect.setXMinimum( std::min( rect.xMinimum(), itemRect.xMinimum() - m.left * mup ) );
    rect.setYMinimum( std::min( rect.yMinimum(), itemRect.yMinimum() - m.bottom * mup ) );
    rect.setXMaximum( std::max( rect.xMaximum(), itemRect.xMaximum() + m.right * mup ) );
    rect.setYMaximum( std::max( rect.yMaximum(), itemRect.yMaximum() + m.top * mup ) );
  }
  return rect;
}

bool KadasSelectionRectItem::intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains ) const
{
  if ( mItems.isEmpty() )
  {
    return false;
  }
  QgsRectangle mapRect = itemsRect( settings.destinationCrs(), settings.mapUnitsPerPixel() );

  QgsPolygon itemRect;
  QgsLineString *points = new QgsLineString();
  points->setPoints( QgsPointSequence() << QgsPoint( mapRect.xMinimum(), mapRect.yMinimum() ) << QgsPoint( mapRect.xMaximum(), mapRect.yMinimum() ) << QgsPoint( mapRect.xMaximum(), mapRect.yMaximum() ) << QgsPoint( mapRect.xMinimum(), mapRect.yMaximum() ) << QgsPoint( mapRect.xMinimum(), mapRect.yMinimum() ) );
  itemRect.setExteriorRing( points );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence() << QgsPoint( rect.xMinimum(), rect.yMinimum() ) << QgsPoint( rect.xMaximum(), rect.yMinimum() ) << QgsPoint( rect.xMaximum(), rect.yMaximum() ) << QgsPoint( rect.xMinimum(), rect.yMaximum() ) << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &filterRect );
  bool intersects = contains ? geomEngine->contains( &itemRect ) : geomEngine->intersects( &itemRect );
  delete geomEngine;
  return intersects;
}

void KadasSelectionRectItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  if ( mItems.isEmpty() )
  {
    return;
  }

  QgsRectangle mapRect = itemsRect( context.coordinateTransform().destinationCrs(), context.mapToPixel().mapUnitsPerPixel() );
  QList<QgsPoint> points = QList<QgsPoint>()
                           << QgsPoint( mapRect.xMinimum(), mapRect.yMinimum() )
                           << QgsPoint( mapRect.xMaximum(), mapRect.yMinimum() )
                           << QgsPoint( mapRect.xMaximum(), mapRect.yMaximum() )
                           << QgsPoint( mapRect.xMinimum(), mapRect.yMaximum() )
                           << QgsPoint( mapRect.xMinimum(), mapRect.yMinimum() );

  QPolygonF poly;
  for ( QgsPoint p : points )
  {
    p.transform( context.coordinateTransform() );
    p.transform( context.mapToPixel().transform() );
    poly.append( p.toQPointF() );
  }
  QPainterPath path;
  path.addPolygon( poly );

  context.painter()->setPen( QPen( Qt::black, 2, Qt::DashLine ) );
  context.painter()->setBrush( Qt::NoBrush );
  context.painter()->drawPath( path );
}
