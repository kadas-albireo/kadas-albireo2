/***************************************************************************
    kadasmapitem.cpp
    ----------------
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

#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadasmapitem.h>


KadasMapItem::KadasMapItem ( const QgsCoordinateReferenceSystem& crs, QObject* parent )
  : QObject ( parent ), mCrs ( crs )
{
}

KadasMapItem::~KadasMapItem()
{
  emit aboutToBeDestroyed();
  if ( mAssociatedLayer ) {
    QgsProject::instance()->removeMapLayer ( mAssociatedLayer->id() );
  }
}

void KadasMapItem::associateToLayer ( QgsMapLayer* layer )
{
  mAssociatedLayer = layer;
  setParent ( layer );
}

void KadasMapItem::setSelected ( bool selected )
{
  mSelected = selected;
  emit changed();
}

void KadasMapItem::setZIndex ( int zIndex )
{
  mZIndex = zIndex;
  emit changed();
}

void KadasMapItem::setState ( const State* state )
{
  mState->assign ( state );
  recomputeDerived();
}

void KadasMapItem::clear()
{
  delete mState;
  mState = createEmptyState();
  recomputeDerived();
}

void KadasMapItem::defaultNodeRenderer ( QPainter* painter, const QgsPointXY& screenPoint, int nodeSize )
{
  painter->setPen ( QPen ( Qt::red, 2 ) );
  painter->setBrush ( Qt::white );
  painter->drawRect ( QRectF ( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize ) );
}

void KadasMapItem::anchorNodeRenderer ( QPainter* painter, const QgsPointXY& screenPoint, int nodeSize )
{
  painter->setPen ( QPen ( Qt::black, 1 ) );
  painter->setBrush ( Qt::red );
  painter->drawEllipse ( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}
