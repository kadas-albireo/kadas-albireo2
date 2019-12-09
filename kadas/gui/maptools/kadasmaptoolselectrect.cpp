/***************************************************************************
    kadasmaptoolselectrect.cpp
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

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/maptools/kadasmaptoolselectrect.h>


KadasMapToolSelectRect::KadasMapToolSelectRect( QgsMapCanvas *mapCanvas )
  : QgsMapTool( mapCanvas )
{

}

void KadasMapToolSelectRect::setRect( const QgsRectangle &rect )
{
  clear();
  mRect = rect;
  mRubberband = new QgsRubberBand( canvas(), QgsWkbTypes::PolygonGeometry );
  mRubberband->setToCanvasRectangle( canvasRect( mRect ) );
  mRubberband->setColor( QColor( 127, 127, 255, 127 ) );
  emit rectChanged( mRect );
}

void KadasMapToolSelectRect::clear()
{
  if ( mRubberband )
  {
    canvas()->scene()->removeItem( mRubberband );
    delete mRubberband;
    mRubberband = nullptr;
  }
  mRect = QgsRectangle();
  emit rectChanged( mRect );
}

void KadasMapToolSelectRect::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mRubberband )
  {
    return;
  }
  double tol = QgsSettings().value( "/kadas/snapping_radius", 10 ).toInt();
  if ( mInteraction == InteractionNone )
  {

    // Determine cursor
    QRectF r = canvasRect( mRect );
    bool left = qAbs( r.left() - e->x() ) < tol;
    bool right = qAbs( r.right() - e->x() ) < tol;
    bool top = qAbs( r.top() - e->y() ) < tol;
    bool bottom = qAbs( r.bottom() - e->y() ) < tol;

    if ( mResizeAllowed )
    {
      if ( ( bottom && left ) || ( top && right ) )
      {
        canvas()->setCursor( Qt::SizeBDiagCursor );
      }
      else if ( ( bottom && right ) || ( top && left ) )
      {
        canvas()->setCursor( Qt::SizeFDiagCursor );
      }
      else if ( top || bottom )
      {
        canvas()->setCursor( Qt::SizeVerCursor );
      }
      else if ( left || right )
      {
        canvas()->setCursor( Qt::SizeHorCursor );
      }
      else if ( r.contains( e->pos() ) )
      {
        canvas()->setCursor( Qt::OpenHandCursor );
      }
      else
      {
        canvas()->unsetCursor();
      }
    }
    else if ( r.contains( e->pos() ) )
    {
      canvas()->setCursor( Qt::OpenHandCursor );
    }
    else
    {
      canvas()->unsetCursor();
    }
  }
  else
  {
    const QgsMapToPixel &mtp = canvas()->mapSettings().mapToPixel();
    QgsPointXY p = mtp.toMapCoordinates( e->pos() );
    p.setX( p.x() - mResizeMoveOffset.x() );
    p.setY( p.y() - mResizeMoveOffset.y() );

    if ( mInteraction == InteractionMoving )
    {
      double x = p.x();
      double y = p.y();
      if ( mShowReferenceWhenMoving )
      {
        double snaptol = tol * canvas()->mapSettings().mapUnitsPerPixel();
        if ( qAbs( x - ( mOldRect.xMinimum() + mOldRect.width() ) ) < snaptol )
        {
          // Left edge matches with old right
          x = mOldRect.xMinimum() + mOldRect.width();
        }
        else if ( qAbs( x + mRect.width() - mOldRect.xMinimum() ) < snaptol )
        {
          // Right edge matches with old left
          x = mOldRect.xMinimum() - mRect.width();
        }
        else if ( qAbs( x - mOldRect.xMinimum() ) < snaptol )
        {
          // Left edge matches with old left
          x = mOldRect.xMinimum();
        }
        if ( qAbs( y - ( mOldRect.yMinimum() + mOldRect.height() ) ) < snaptol )
        {
          // Bottom edge matches with old top
          y = mOldRect.yMinimum() + mOldRect.height();
        }
        else if ( qAbs( y + mRect.height() - mOldRect.yMinimum() ) < snaptol )
        {
          // Top edge matches with old bottom
          y = mOldRect.yMinimum() - mRect.height();
        }
        else if ( qAbs( y - mOldRect.yMinimum() ) < snaptol )
        {
          // Bottom edge matches with old bottom
          y = mOldRect.yMinimum();
        }
      }

      mRect = QRectF( x, y, mRect.width(), mRect.height() );
    }
    else if ( mInteraction == InteractionResizing )
    {
      for ( const auto &handler : mResizeHandlers )
      {
        handler( p );
      }
      mRect = QgsRectangle( mResizePoints[0], mResizePoints[1] );
    }
    emit rectChanged( mRect );
    mRubberband->setToCanvasRectangle( canvasRect( mRect ) );
  }
}

void KadasMapToolSelectRect::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( mRubberband && e->button() == Qt::LeftButton )
  {
    QRect r = canvasRect( mRect );
    if ( mResizeAllowed )
    {
      QPoint p1 = r.topLeft();
      QPoint p2 = r.bottomRight();
      double mup = canvas()->mapSettings().mapUnitsPerPixel();
      double tol = QgsSettings().value( "/kadas/snapping_radius", 10 ).toInt();
      mResizePoints = QList<QgsPointXY>() << QgsPointXY( mRect.xMinimum(), mRect.yMinimum() ) << QgsPointXY( mRect.xMaximum(), mRect.yMaximum() );
      mResizeMoveOffset = QgsPointXY();

      if ( qAbs( p1.x() - e->x() ) < tol )
      {
        mResizeHandlers.append( [this]( const QgsPointXY & p ) { mResizePoints[0].setX( p.x() ); } );
        mResizeMoveOffset.setX( ( e->x() - p1.x() ) * mup );
      }
      else if ( qAbs( p2.x() - e->x() ) < tol )
      {
        mResizeHandlers.append( [this]( const QgsPointXY & p ) { mResizePoints[1].setX( p.x() ); } );
        mResizeMoveOffset.setX( ( e->x() - p2.x() ) * mup );
      }
      if ( qAbs( p1.y() - e->y() ) < tol )
      {
        mResizeHandlers.append( [this]( const QgsPointXY & p ) { mResizePoints[1].setY( p.y() ); } );
        mResizeMoveOffset.setY( -( e->y() - p1.y() ) * mup );
      }
      else if ( qAbs( p2.y() - e->y() ) < tol )
      {
        mResizeHandlers.append( [this]( const QgsPointXY & p ) { mResizePoints[0].setY( p.y() ); } );
        mResizeMoveOffset.setY( -( e->y() - p2.y() ) * mup );
      }
    }

    if ( !mResizeHandlers.isEmpty() )
    {
      mInteraction = InteractionResizing;
    }
    else if ( r.contains( e->pos() ) )
    {
      if ( mShowReferenceWhenMoving )
      {
        mOldRect = mRect;
        mOldRubberband = new QgsRubberBand( canvas(), QgsWkbTypes::PolygonGeometry );
        mOldRubberband->setToCanvasRectangle( canvasRect( mOldRect ) );
        mOldRubberband->setColor( QColor( 127, 127, 255, 31 ) );
      }

      mInteraction = InteractionMoving;
      const QgsMapToPixel &mtp = canvas()->mapSettings().mapToPixel();
      QgsPointXY p = mtp.toMapCoordinates( e->pos() );
      mResizeMoveOffset = QgsPointXY( p.x() - mRect.xMinimum(), p.y() - mRect.yMinimum() );
    }
  }
}

void KadasMapToolSelectRect::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  mInteraction = InteractionNone;
  if ( mOldRubberband )
  {
    canvas()->scene()->removeItem( mOldRubberband );
    delete mOldRubberband;
    mOldRubberband = nullptr;
  }
  mResizeHandlers.clear();
  mResizePoints.clear();
  mResizeMoveOffset = QgsPointXY();
  emit rectChangeComplete( mRect );
}

void KadasMapToolSelectRect::deactivate()
{
  QgsMapTool::deactivate();
  clear();
}

QRect KadasMapToolSelectRect::canvasRect( const QgsRectangle &rect ) const
{
  const QgsMapToPixel &mtp = canvas()->mapSettings().mapToPixel();
  QgsPointXY p1 = mtp.transform( rect.xMinimum(), rect.yMaximum() );
  QgsPointXY p2 = mtp.transform( rect.xMaximum(), rect.yMinimum() );
  return QRect( p1.x(), p1.y(), p2.x() - p1.x(), p2.y() - p1.y() );
}
