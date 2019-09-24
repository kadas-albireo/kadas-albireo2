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

#include <kadas/gui/mapitems/kadasselectionrectitem.h>


KadasSelectionRectItem::KadasSelectionRectItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : KadasMapItem( crs, parent )
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
  QgsRectangle rect = mItems.front()->boundingBox();
  for ( int i = 1, n = mItems.size(); i < n; ++i )
  {
    QgsRectangle itemRect = mItems[i]->boundingBox();
    rect.setXMinimum( qMin( rect.xMinimum(), itemRect.xMinimum() ) );
    rect.setYMinimum( qMin( rect.yMinimum(), itemRect.yMinimum() ) );
    rect.setXMaximum( qMin( rect.xMaximum(), itemRect.xMaximum() ) );
    rect.setYMaximum( qMin( rect.yMaximum(), itemRect.yMaximum() ) );
  }
  return rect;
}

QRect KadasSelectionRectItem::margin() const
{
  if ( mItems.isEmpty() )
  {
    return QRect();
  }
  QRect rect = mItems.front()->margin();
  for ( int i = 1, n = mItems.size(); i < n; ++i )
  {
    rect = rect.united( mItems[i]->margin() );
  }
  return rect;
}

QgsRectangle KadasSelectionRectItem::itemsRect( double mup ) const
{
  QgsRectangle selRect;
  for ( const KadasMapItem *item : mItems )
  {
    QgsRectangle itemRect = item->boundingBox();
    QRectF itemMargin = item->margin();
    if ( selRect.isEmpty() )
    {
      selRect.setXMinimum( itemRect.xMinimum() - itemMargin.left() * mup );
      selRect.setXMaximum( itemRect.xMaximum() + itemMargin.right() * mup );
      selRect.setYMinimum( itemRect.yMinimum() - itemMargin.bottom() * mup );
      selRect.setYMaximum( itemRect.yMaximum() + itemMargin.top() * mup );
    }
    else
    {
      selRect.setXMinimum( qMin( itemRect.xMinimum() - itemMargin.left() * mup, selRect.xMinimum() ) );
      selRect.setXMaximum( qMax( itemRect.xMaximum() + itemMargin.right() * mup, selRect.xMaximum() ) );
      selRect.setYMinimum( qMin( itemRect.yMinimum() - itemMargin.bottom() * mup, selRect.yMinimum() ) );
      selRect.setYMaximum( qMax( itemRect.yMaximum() + itemMargin.top() * mup, selRect.yMaximum() ) );
    }
  }
  return selRect;
}

bool KadasSelectionRectItem::intersects( const QgsRectangle &rect, const QgsMapSettings &settings ) const
{
  if ( mItems.isEmpty() )
  {
    return false;
  }
  QgsRectangle selRect = itemsRect( settings.mapUnitsPerPixel() );

  QgsPolygon itemRect;
  QgsLineString *points = new QgsLineString();
  points->setPoints( QgsPointSequence()
                     << QgsPoint( selRect.xMinimum(), selRect.yMinimum() )
                     << QgsPoint( selRect.xMaximum(), selRect.yMinimum() )
                     << QgsPoint( selRect.xMaximum(), selRect.yMaximum() )
                     << QgsPoint( selRect.xMinimum(), selRect.yMaximum() )
                     << QgsPoint( selRect.xMinimum(), selRect.yMinimum() ) );
  itemRect.setExteriorRing( points );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &itemRect );
  bool intersects = geomEngine->intersects( &filterRect );
  delete geomEngine;
  return intersects;
}

void KadasSelectionRectItem::render( QgsRenderContext &context ) const
{
  if ( mItems.isEmpty() )
  {
    return;
  }

  double mup = context.mapToPixel().mapUnitsPerPixel();
  QgsRectangle selRect = itemsRect( context.mapToPixel().mapUnitsPerPixel() );

  QList<QgsPoint> points = QList<QgsPoint>()
                           << QgsPoint( selRect.xMinimum(), selRect.yMinimum() )
                           << QgsPoint( selRect.xMaximum(), selRect.yMinimum() )
                           << QgsPoint( selRect.xMaximum(), selRect.yMaximum() )
                           << QgsPoint( selRect.xMinimum(), selRect.yMaximum() )
                           << QgsPoint( selRect.xMinimum(), selRect.yMinimum() );

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
