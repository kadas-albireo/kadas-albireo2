/***************************************************************************
    kadasmaptoolpan.cpp
    -------------------
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

#include <QBitmap>
#include <QCursor>
#include <QMouseEvent>
#include <QPinchGesture>
#include <QToolTip>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/maptools/kadasmaptooldeleteitems.h>
#include <kadas/gui/maptools/kadasmaptoolpan.h>

KadasMapToolPan::KadasMapToolPan( QgsMapCanvas *canvas, bool allowItemInteraction )
  : QgsMapTool( canvas )
  , mAllowItemInteraction( allowItemInteraction )
  , mDragging( false )
  , mPinching( false )
  , mExtentRubberBand( 0 )
  , mPickClick( false )
{
  mToolName = tr( "Pan" );
  setCursor( QCursor( Qt::ArrowCursor ) );
}

KadasMapToolPan::~KadasMapToolPan()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
}

void KadasMapToolPan::activate()
{
  mCanvas->grabGesture( Qt::PinchGesture );
  QgsMapTool::activate();
}

void KadasMapToolPan::deactivate()
{
  mCanvas->ungrabGesture( Qt::PinchGesture );
  QgsMapTool::deactivate();
}

void KadasMapToolPan::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( e->modifiers() == Qt::ShiftModifier || e->modifiers() == Qt::ControlModifier )
    {
      mExtentRubberBand = new QgsRubberBand( mCanvas, QgsWkbTypes::PolygonGeometry );
      if ( e->modifiers() == Qt::ShiftModifier )
      {
        mExtentRubberBand->setColor( QColor( 0, 0, 255, 63 ) );
      }
      else if ( e->modifiers() == Qt::ControlModifier )
      {
        mExtentRubberBand->setFillColor( Qt::transparent );
        mExtentRubberBand->setStrokeColor( Qt::black );
        mExtentRubberBand->setWidth( 2 );
        mExtentRubberBand->setLineStyle( Qt::DashLine );
      }
      mExtentRect.setTopLeft( e->pos() );
      mExtentRect.setBottomRight( e->pos() );
      mExtentRubberBand->setToCanvasRectangle( mExtentRect );
      mExtentRubberBand->show();
    }
    else if ( mAllowItemInteraction )
    {
      mPickClick = true;
    }
  }
  else if ( e->button() == Qt::RightButton && mAllowItemInteraction )
  {
    emit contextMenuRequested( e->pos(), toMapCoordinates( e->pos() ) );
  }
}

void KadasMapToolPan::canvasMoveEvent( QgsMapMouseEvent *e )
{
  mPickClick = false;

  if ( ( e->buttons() & Qt::LeftButton ) )
  {
    if ( mExtentRubberBand )
    {
      mExtentRect.setBottomRight( e->pos() );
      mExtentRubberBand->setToCanvasRectangle( mExtentRect );
    }
    else
    {
      mDragging = true;
      mCanvas->panAction( e );
      mCanvas->setCursor( QCursor( Qt::ClosedHandCursor ) );
    }
  }
  else
  {
    mCanvas->setCursor( mCursor );
  }
}

void KadasMapToolPan::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mExtentRubberBand )
    {
      if ( e->modifiers() == Qt::ShiftModifier )
      {
        // set center and zoom
        QSize zoomRectSize = mExtentRect.normalized().size();
        QSize canvasSize = mCanvas->mapSettings().outputSize();
        double sfx = ( double ) zoomRectSize.width() / canvasSize.width();
        double sfy = ( double ) zoomRectSize.height() / canvasSize.height();
        double scaleFactor = qMax( sfx, sfy );

        QgsPointXY c = mCanvas->getCoordinateTransform()->toMapCoordinates( mExtentRect.center() );
        QgsRectangle oe = mCanvas->mapSettings().extent();
        QgsRectangle e(
          c.x() - oe.width() / 2.0, c.y() - oe.height() / 2.0,
          c.x() + oe.width() / 2.0, c.y() + oe.height() / 2.0
        );
        e.scale( scaleFactor, &c );
        mCanvas->setExtent( e, true );
        mCanvas->refresh();
      }
      else if ( e->modifiers() == Qt::ControlModifier )
      {
        KadasMapToolDeleteItems( canvas() ).deleteItems( mExtentRubberBand->rect(), canvas()->mapSettings().destinationCrs() );
      }

      delete mExtentRubberBand;
      mExtentRubberBand = 0;
    }
    else if ( mDragging )
    {
      mCanvas->panActionEnd( e->pos() );
      mDragging = false;
      mCanvas->setCursor( mCursor );
    }
    else if ( mAllowItemInteraction && mPickClick )
    {
      KadasFeaturePicker::PickResult result = KadasFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), QgsWkbTypes::UnknownGeometry );
      if ( !result.isEmpty() )
      {
        emit itemPicked( result );
      }
    }
  }
}

bool KadasMapToolPan::gestureEvent( QGestureEvent *event )
{
  qDebug() << "gesture " << event;
  if ( QGesture *gesture = event->gesture( Qt::PinchGesture ) )
  {
    mPinching = true;
    pinchTriggered( static_cast<QPinchGesture *>( gesture ) );
  }
  return true;
}

bool KadasMapToolPan::canvasToolTipEvent( QHelpEvent *e )
{
  QPoint canvasPos = e->pos();
  KadasFeaturePicker::PickResult result = KadasFeaturePicker::pick( mCanvas, canvasPos, mCanvas->getCoordinateTransform()->toMapCoordinates( canvasPos ) );
  if ( result.itemId != KadasItemLayer::ITEM_ID_NULL )
  {
    QString tooltip = static_cast<KadasItemLayer *>( result.layer )->items()[result.itemId]->tooltip();
    if ( !tooltip.isEmpty() )
    {
      QToolTip::showText( e->globalPos(), tooltip, mCanvas );
      return true;
    }
  }
  return false;
}

void KadasMapToolPan::pinchTriggered( QPinchGesture *gesture )
{
  if ( gesture->state() == Qt::GestureFinished )
  {
    //a very small totalScaleFactor indicates a two finger tap (pinch gesture without pinching)
    if ( 0.98 < gesture->totalScaleFactor()  && gesture->totalScaleFactor() < 1.02 )
    {
      mCanvas->zoomOut();
    }
    else
    {
      //Transfor global coordinates to widget coordinates
      QPoint pos = gesture->centerPoint().toPoint();
      pos = mCanvas->mapFromGlobal( pos );
      // transform the mouse pos to map coordinates
      QgsPointXY center  = mCanvas->getCoordinateTransform()->toMapCoordinates( pos.x(), pos.y() );
      QgsRectangle r = mCanvas->extent();
      r.scale( 1 / gesture->totalScaleFactor(), &center );
      mCanvas->setExtent( r );
      mCanvas->refresh();
    }
    mPinching = false;
  }
}
