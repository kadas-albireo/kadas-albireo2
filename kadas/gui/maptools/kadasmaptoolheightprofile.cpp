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

#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/kadasheightprofiledialog.h>
#include <kadas/gui/mapitems/kadaspointitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/maptools/kadasmaptoolheightprofile.h>

KadasMapToolHeightProfile::KadasMapToolHeightProfile ( QgsMapCanvas* canvas )
  : KadasMapToolCreateItem ( canvas, lineFactory ( canvas ) )
{
  QgsSettings settings;

  mPosMarker = new KadasPointItem ( canvas->mapSettings().destinationCrs(), KadasPointItem::ICON_CIRCLE, this );
  mPosMarker->setZIndex ( 100 );
  KadasMapCanvasItemManager::instance()->addItem ( mPosMarker );


  mDialog = new KadasHeightProfileDialog ( this, 0, Qt::WindowStaysOnTopHint );
  connect ( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolHeightProfile::drawFinished );
  connect ( this, &KadasMapToolCreateItem::cleared, this, &KadasMapToolHeightProfile::drawCleared );
}

KadasMapToolCreateItem::ItemFactory KadasMapToolHeightProfile::lineFactory ( QgsMapCanvas* canvas )
{
  return [ = ] {
    KadasLineItem* item = new KadasLineItem ( canvas->mapSettings().destinationCrs() );
    item->setIconType ( KadasGeometryItem::ICON_CIRCLE );
    return item;
  };
}

KadasMapToolHeightProfile::~KadasMapToolHeightProfile()
{
  delete mPosMarker;
}

void KadasMapToolHeightProfile::activate()
{
  mPicking = false;
  setCursor ( Qt::ArrowCursor );
  mDialog->show();
  KadasMapToolCreateItem::activate();
}

void KadasMapToolHeightProfile::deactivate()
{
  mDialog->close();
  mDialog->setPoints ( QList<QgsPointXY>(), mCanvas->mapSettings().destinationCrs() );
  KadasMapToolCreateItem::deactivate();
}

void KadasMapToolHeightProfile::setGeometry ( const QgsAbstractGeometry* geom, const QgsCoordinateReferenceSystem& crs )
{
  clear();
  addPartFromGeometry ( geom, crs );
}

void KadasMapToolHeightProfile::pickLine()
{
  mPicking = true;
  setCursor ( QCursor ( Qt::CrossCursor ) );
}

void KadasMapToolHeightProfile::canvasPressEvent ( QgsMapMouseEvent* e )
{
  if ( !mPicking ) {
    KadasMapToolCreateItem::canvasPressEvent ( e );
  }
}

void KadasMapToolHeightProfile::canvasMoveEvent ( QgsMapMouseEvent* e )
{
  if ( !mPicking ) {
    KadasLineItem* lineItem = dynamic_cast<KadasLineItem*> ( currentItem() );
    if ( lineItem && lineItem->constState()->drawStatus == KadasMapItem::State::Finished && !lineItem->constState()->points.isEmpty() ) {
      QgsPointXY p = toMapCoordinates ( e->pos() );
      const QList<QgsPointXY>& points = lineItem->constState()->points.front();
      double minDist = std::numeric_limits<double>::max();
      int minIdx = 0;
      QgsPoint minPos;
      QgsCoordinateTransform crst ( lineItem->crs(), canvas()->mapSettings().destinationCrs(), canvas()->mapSettings().transformContext() );
      for ( int i = 0, nPoints = points.size(); i < nPoints - 1; ++i ) {
        QgsPointXY p1 = crst.transform ( points[i] );
        QgsPointXY p2 = crst.transform ( points[i + 1] );
        QgsPoint pProj = QgsGeometryUtils::projectPointOnSegment ( QgsPoint ( p ), QgsPoint ( p1 ), QgsPoint ( p2 ) );
        double dist = pProj.distanceSquared ( p.x(), p.y() );
        if ( dist < minDist ) {
          minDist = dist;
          minPos = pProj;
          minIdx = i;
        }
      }
      if ( qSqrt ( minDist ) / mCanvas->mapSettings().mapUnitsPerPixel() < 30. ) {
        mPosMarker->clear();
        mPosMarker->addPartFromGeometry ( &minPos );
        mDialog->setMarkerPos ( minIdx, minPos );
      }
    }
    KadasMapToolCreateItem::canvasMoveEvent ( e );
  }
}

void KadasMapToolHeightProfile::canvasReleaseEvent ( QgsMapMouseEvent* e )
{
  if ( !mPicking ) {
    KadasMapToolCreateItem::canvasReleaseEvent ( e );
  } else {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick ( mCanvas, e->pos(), toMapCoordinates ( e->pos() ), QgsWkbTypes::LineGeometry );
    if ( pickResult.feature.isValid() ) {
      setGeometry ( pickResult.feature.geometry().constGet(), pickResult.layer->crs() );
    }
    mPicking = false;
    setCursor ( Qt::ArrowCursor );
  }
}

void KadasMapToolHeightProfile::keyReleaseEvent ( QKeyEvent* e )
{
  if ( mPicking && e->key() == Qt::Key_Escape ) {
    mPicking = false;
    setCursor ( Qt::ArrowCursor );
  } else {
    KadasMapToolCreateItem::keyReleaseEvent ( e );
  }
}

void KadasMapToolHeightProfile::drawCleared()
{
  mPosMarker->clear();
  mDialog->clear();
}

void KadasMapToolHeightProfile::drawFinished()
{
  KadasLineItem* lineItem = dynamic_cast<KadasLineItem*> ( currentItem() );
  if ( lineItem ) {
    if ( !lineItem->constState()->points.isEmpty() && !lineItem->constState()->points.front().isEmpty() ) {
      const QList<QgsPointXY>& line = lineItem->constState()->points.front();
      mDialog->setPoints ( line, lineItem->crs() );
      QgsPoint markerPos ( line[0] );
      mPosMarker->addPartFromGeometry ( &markerPos );
    }
  }
}
