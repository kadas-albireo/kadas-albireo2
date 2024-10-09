/***************************************************************************
    kadasmaptoolheightprofile.cpp
    -----------------------------
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

#include <qgis/qgsgeometryutils.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/kadasmapcanvasitemmanager.h"
#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/kadasheightprofiledialog.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/maptools/kadasmaptoolheightprofile.h"

KadasMapItem* KadasMapToolHeightProfileItemInterface::createItem() const
{
  KadasLineItem *item = new KadasLineItem( mCanvas->mapSettings().destinationCrs() );
  item->setIconType( KadasGeometryItem::IconType::ICON_CIRCLE );
  return item;
}


KadasMapToolHeightProfile::KadasMapToolHeightProfile( QgsMapCanvas *canvas )
  : KadasMapToolCreateItem( canvas, std::move( std::make_unique<KadasMapToolHeightProfileItemInterface>( KadasMapToolHeightProfileItemInterface( canvas ) ) ) )
{
  setSelectItems( false );
  setToolLabel( tr( "Measure height profile" ) );

  mPosMarker = new KadasPointItem( canvas->mapSettings().destinationCrs(), KadasPointItem::IconType::ICON_CIRCLE );
  mPosMarker->setIconFill( Qt::blue );
  mPosMarker->setIconOutline( QPen( Qt::blue ) );
  mPosMarker->setZIndex( 100 );
  KadasMapCanvasItemManager::instance()->addItem( mPosMarker );


  mDialog = new KadasHeightProfileDialog( this, 0, Qt::WindowStaysOnTopHint );
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolHeightProfile::drawFinished );
  connect( this, &KadasMapToolCreateItem::cleared, this, &KadasMapToolHeightProfile::drawCleared );
}

void KadasMapToolHeightProfile::activate()
{
  mPicking = false;
  setCursor( Qt::ArrowCursor );
  mDialog->show();
  KadasMapToolCreateItem::activate();
}

void KadasMapToolHeightProfile::deactivate()
{
  mDialog->close();
  mDialog->setPoints( QList<QgsPointXY>(), mCanvas->mapSettings().destinationCrs() );
  KadasMapToolCreateItem::deactivate();
  delete mPosMarker;
  mPosMarker = nullptr;
}

void KadasMapToolHeightProfile::setGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs )
{
  clear();
  addPartFromGeometry( geom, crs );
}

void KadasMapToolHeightProfile::setMarkerPos( double distance )
{
  const KadasLineItem *lineItem = dynamic_cast<const KadasLineItem *>( currentItem() );
  if ( !mPosMarker || lineItem->constState()->points.isEmpty() )
  {
    return;
  }
  const QList<KadasItemPos> &points = lineItem->constState()->points.front();
  for ( int i = 0, n = points.size() - 1; i < n; ++i )
  {
    double segDist = std::sqrt( points[i + 1].sqrDist( points[i] ) );
    if ( distance < segDist )
    {
      double k = distance / segDist;
      double x = points[i].x() + ( points[i + 1].x() - points[i].x() ) * k;
      double y = points[i].y() + ( points[i + 1].y() - points[i].y() ) * k;
      mPosMarker->setPosition( KadasItemPos( x, y ) );
      return;
    }
    else
    {
      distance -= segDist;
    }
  }
  mPosMarker->setPosition( points.last() );
}

void KadasMapToolHeightProfile::pickLine()
{
  mPicking = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void KadasMapToolHeightProfile::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( !mPicking )
  {
    KadasMapToolCreateItem::canvasPressEvent( e );
  }
}

void KadasMapToolHeightProfile::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mPicking )
  {
    const KadasLineItem *lineItem = dynamic_cast<const KadasLineItem *>( currentItem() );
    if ( lineItem && lineItem->constState()->drawStatus == KadasMapItem::State::DrawStatus::Finished && !lineItem->constState()->points.isEmpty() )
    {
      QgsPointXY p = toMapCoordinates( e->pos() );
      const QList<KadasItemPos> &points = lineItem->constState()->points.front();
      double minDist = std::numeric_limits<double>::max();
      int minIdx = 0;
      QgsPoint minPos;
      QgsCoordinateTransform crst( lineItem->crs(), canvas()->mapSettings().destinationCrs(), canvas()->mapSettings().transformContext() );
      for ( int i = 0, nPoints = points.size(); i < nPoints - 1; ++i )
      {
        QgsPointXY p1 = crst.transform( points[i] );
        QgsPointXY p2 = crst.transform( points[i + 1] );
        QgsPoint pProj = QgsGeometryUtils::projectPointOnSegment( QgsPoint( p ), QgsPoint( p1 ), QgsPoint( p2 ) );
        double dist = pProj.distanceSquared( p.x(), p.y() );
        if ( dist < minDist )
        {
          minDist = dist;
          minPos = pProj;
          minIdx = i;
        }
      }
      if ( std::sqrt( minDist ) / mCanvas->mapSettings().mapUnitsPerPixel() < 30. )
      {
        mPosMarker->setPosition( KadasItemPos::fromPoint( minPos ) );
        mDialog->setMarkerPos( minIdx, minPos, mCanvas->mapSettings().destinationCrs() );
      }
    }
    KadasMapToolCreateItem::canvasMoveEvent( e );
  }
}

void KadasMapToolHeightProfile::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPicking )
  {
    KadasMapToolCreateItem::canvasReleaseEvent( e );
  }
  else
  {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), Qgis::GeometryType::Line );
    if ( pickResult.geom )
    {
      setGeometry( *pickResult.geom, pickResult.crs );
    }
    mPicking = false;
    setCursor( Qt::ArrowCursor );
  }
}

void KadasMapToolHeightProfile::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPicking && e->key() == Qt::Key_Escape )
  {
    mPicking = false;
    setCursor( Qt::ArrowCursor );
  }
  else
  {
    KadasMapToolCreateItem::keyReleaseEvent( e );
  }
}

void KadasMapToolHeightProfile::drawCleared()
{
  mPosMarker->clear();
  mDialog->clear();
}

void KadasMapToolHeightProfile::drawFinished()
{
  const KadasLineItem *lineItem = dynamic_cast<const KadasLineItem *>( currentItem() );
  if ( lineItem )
  {
    if ( !lineItem->constState()->points.isEmpty() && !lineItem->constState()->points.front().isEmpty() )
    {
      QList<QgsPointXY> points;
      for ( const KadasItemPos &pos : lineItem->constState()->points.front() )
      {
        points.append( pos );
      }
      mDialog->setPoints( points, lineItem->crs() );
      QgsPoint markerPos( points[0] );
      mPosMarker->clear();
      mPosMarker->addPartFromGeometry( markerPos );
    }
  }
}
