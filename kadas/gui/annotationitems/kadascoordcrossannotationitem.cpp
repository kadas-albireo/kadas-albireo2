/***************************************************************************
    kadascoordcrossannotationitem.cpp
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

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsrendercontext.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"


KadasCoordCrossAnnotationItem::KadasCoordCrossAnnotationItem( const QgsPoint &point )
  : QgsAnnotationMarkerItem( point )
{
  setZIndex( KadasAnnotationZIndex::CoordCross );
  installDefaultSymbol();
}

QString KadasCoordCrossAnnotationItem::type() const
{
  return itemTypeId();
}

void KadasCoordCrossAnnotationItem::installDefaultSymbol()
{
  // The cross and labels are drawn manually in render(); the marker symbol
  // is kept fully transparent so the base QgsAnnotationMarkerItem painter
  // does not draw anything on top of it.
  auto *layer = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle );
  layer->setColor( QColor( 0, 0, 0, 0 ) );
  layer->setStrokeColor( QColor( 0, 0, 0, 0 ) );
  layer->setSize( 0.1 );
  setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << layer ) );
}

QgsRectangle KadasCoordCrossAnnotationItem::boundingBox() const
{
  const QgsPointXY p = geometry();
  return QgsRectangle( p.x(), p.y(), p.x(), p.y() );
}

QgsRectangle KadasCoordCrossAnnotationItem::boundingBox( QgsRenderContext &context ) const
{
  // Inflate the point bounds by the cross half-extent, expressed in map units,
  // so the canvas chunk loader rasters the labels correctly when the cross
  // sits near a tile edge.
  const double mupp = context.mapToPixel().mapUnitsPerPixel();
  const double crossMu = sCrossSizePx * context.scaleFactor() * mupp;
  const QgsPointXY p = geometry();
  return QgsRectangle( p.x() - crossMu, p.y() - crossMu, p.x() + crossMu, p.y() + crossMu );
}

void KadasCoordCrossAnnotationItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  // Draw nothing through the base symbol (it is transparent), but allow it
  // to update any cached state.
  QgsAnnotationMarkerItem::render( context, feedback );

  const double crossSize = sCrossSizePx * context.scaleFactor();
  const QgsPointXY mapPos = context.coordinateTransform().transform( geometry() );
  const QPointF screenPos = context.mapToPixel().transform( mapPos ).toQPointF();

  QPainter *painter = context.painter();
  painter->save();
  painter->setPen( QPen( Qt::white, 10 ) );
  painter->drawLine( QLineF( screenPos.x() - crossSize, screenPos.y(), screenPos.x() + crossSize, screenPos.y() ) );
  painter->drawLine( QLineF( screenPos.x(), screenPos.y() - crossSize, screenPos.x(), screenPos.y() + crossSize ) );
  painter->setPen( QPen( Qt::black, 3 ) );
  painter->drawLine( QLineF( screenPos.x() - crossSize, screenPos.y(), screenPos.x() + crossSize, screenPos.y() ) );
  painter->drawLine( QLineF( screenPos.x(), screenPos.y() - crossSize, screenPos.x(), screenPos.y() + crossSize ) );

  struct LabelData
  {
      double x, y;
      double mapCoord;
      double angle;
  };
  const QList<LabelData> labels = {
    { screenPos.x() - crossSize, screenPos.y() - 12, geometry().y(), 0 },
    { screenPos.x() - 12, screenPos.y() + crossSize, geometry().x(), -90 },
  };

  QFont font = painter->font();
  font.setPixelSize( sFontSizePx * context.scaleFactor() );

  for ( const LabelData &label : labels )
  {
    QPainterPath path;
    path.addText( 0, 0, font, QString::number( label.mapCoord / 1000., 'f', 0 ) );
    painter->save();
    painter->translate( label.x, label.y );
    painter->rotate( label.angle );
    painter->setBrush( Qt::black );
    painter->setPen( QPen( Qt::white, qRound( sFontSizePx / 3. ) ) );
    painter->drawPath( path );
    painter->setPen( Qt::NoPen );
    painter->drawPath( path );
    painter->restore();
  }
  painter->restore();
}

KadasCoordCrossAnnotationItem *KadasCoordCrossAnnotationItem::clone() const
{
  auto *item = new KadasCoordCrossAnnotationItem( QgsPoint( geometry().x(), geometry().y() ) );
  if ( symbol() )
    item->setSymbol( symbol()->clone() );
  item->copyCommonProperties( this );
  return item;
}

KadasCoordCrossAnnotationItem *KadasCoordCrossAnnotationItem::create()
{
  return new KadasCoordCrossAnnotationItem();
}
