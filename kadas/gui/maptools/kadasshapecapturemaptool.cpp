/***************************************************************************
    kadasshapecapturemaptool.cpp
    ----------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>
#include <limits>

#include <QKeyEvent>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsrubberband.h>

#include "kadas/gui/maptools/kadasshapecapturemaptool.h"


KadasShapeCaptureMapTool::KadasShapeCaptureMapTool( QgsMapCanvas *canvas, Shape shape )
  : QgsMapTool( canvas )
  , mShape( shape )
{}

KadasShapeCaptureMapTool::~KadasShapeCaptureMapTool()
{
  delete mRubberBand;
}

void KadasShapeCaptureMapTool::setShape( Shape shape )
{
  if ( mShape == shape )
    return;
  mShape = shape;
  clear();
}

void KadasShapeCaptureMapTool::clear()
{
  resetRubberBand();
  mDragging = false;
  mCapturing = false;
  mVertices.clear();
  mCircleRadius = 0.0;
  mSectorStage = SectorStage::None;
  mSectorStartAngle = 0.0;
  mSectorStopAngle = 0.0;
  emit cleared();
}

void KadasShapeCaptureMapTool::deactivate()
{
  clear();
  QgsMapTool::deactivate();
}

void KadasShapeCaptureMapTool::setCircleRadius( double radius )
{
  if ( mShape == Shape::Circle )
  {
    mCircleRadius = radius;
    updateCircleRubberBand();
  }
  else if ( mShape == Shape::Sector )
  {
    mCircleRadius = radius;
    updateSectorRubberBand();
  }
}

void KadasShapeCaptureMapTool::setCapturedPolyline( const QVector<QgsPointXY> &vertices )
{
  if ( mShape != Shape::Polyline && mShape != Shape::Polygon )
    return;
  mVertices = vertices;
  mCapturing = false;
  mDragging = false;
  updatePolyRubberBand( QgsPointXY(), false );
}

void KadasShapeCaptureMapTool::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() != Qt::LeftButton )
  {
    if ( e->button() == Qt::RightButton && ( mShape == Shape::Polyline || mShape == Shape::Polygon ) && mCapturing )
    {
      // Right click finishes polyline/polygon capture
      const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      mCapturing = false;
    }
    return;
  }

  switch ( mShape )
  {
    case Shape::Rectangle:
    case Shape::Circle:
      mAnchor = toMapCoordinates( e->pos() );
      mCurrent = mAnchor;
      mDragging = true;
      break;

    case Shape::Sector:
      switch ( mSectorStage )
      {
        case SectorStage::None:
          // First click: place the center
          resetRubberBand();
          mAnchor = toMapCoordinates( e->pos() );
          mCircleRadius = 0.0;
          mSectorStartAngle = 0.0;
          mSectorStopAngle = 2 * M_PI;
          mSectorStage = SectorStage::HaveCenter;
          break;

        case SectorStage::HaveCenter:
          // Second click: fix radius + start angle, begin sweeping
          mSectorStage = SectorStage::HaveRadius;
          break;

        case SectorStage::HaveRadius:
        {
          // Third click: finish the sector
          mSectorStage = SectorStage::None;
          updateSectorRubberBand();
          const QgsGeometry geom = buildSectorGeometry();
          if ( !geom.isEmpty() )
            emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
          break;
        }
      }
      break;

    case Shape::Polyline:
    case Shape::Polygon:
      if ( !mCapturing )
      {
        mVertices.clear();
        mCapturing = true;
      }
      mVertices.append( toMapCoordinates( e->pos() ) );
      updatePolyRubberBand( mVertices.last(), false );
      break;
  }
}

void KadasShapeCaptureMapTool::canvasMoveEvent( QgsMapMouseEvent *e )
{
  switch ( mShape )
  {
    case Shape::Rectangle:
      if ( !mDragging )
        return;
      mCurrent = toMapCoordinates( e->pos() );
      updateRectRubberBand();
      break;

    case Shape::Circle:
      if ( !mDragging )
        return;
      mCurrent = toMapCoordinates( e->pos() );
      mCircleRadius = std::sqrt( mCurrent.sqrDist( mAnchor ) );
      updateCircleRubberBand();
      break;

    case Shape::Sector:
    {
      const QgsPointXY p = toMapCoordinates( e->pos() );
      if ( mSectorStage == SectorStage::HaveCenter )
      {
        // Radius + start angle follow the cursor; full circle until the sweep starts
        mCircleRadius = std::sqrt( p.sqrDist( mAnchor ) );
        mSectorStartAngle = std::atan2( p.y() - mAnchor.y(), p.x() - mAnchor.x() );
        if ( mSectorStartAngle < 0 )
          mSectorStartAngle += 2 * M_PI;
        mSectorStopAngle = mSectorStartAngle + 2 * M_PI;
        updateSectorRubberBand();
      }
      else if ( mSectorStage == SectorStage::HaveRadius )
      {
        mSectorStopAngle = std::atan2( p.y() - mAnchor.y(), p.x() - mAnchor.x() );
        while ( mSectorStopAngle <= mSectorStartAngle )
          mSectorStopAngle += 2 * M_PI;

        // Snap to full circle when the sweep end is within pick tolerance of its start
        const QgsPointXY pStart( mAnchor.x() + mCircleRadius * std::cos( mSectorStartAngle ), mAnchor.y() + mCircleRadius * std::sin( mSectorStartAngle ) );
        const QgsPointXY pEnd( mAnchor.x() + mCircleRadius * std::cos( mSectorStopAngle ), mAnchor.y() + mCircleRadius * std::sin( mSectorStopAngle ) );
        const double tol = searchRadiusMU( canvas() );
        if ( pStart.sqrDist( pEnd ) < tol * tol )
          mSectorStopAngle = mSectorStartAngle + 2 * M_PI;
        updateSectorRubberBand();
      }
      break;
    }

    case Shape::Polyline:
    case Shape::Polygon:
      if ( mCapturing )
        updatePolyRubberBand( toMapCoordinates( e->pos() ), true );
      break;
  }
}

void KadasShapeCaptureMapTool::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( e->button() != Qt::LeftButton )
    return;

  switch ( mShape )
  {
    case Shape::Rectangle:
    {
      if ( !mDragging )
        return;
      mDragging = false;
      mCurrent = toMapCoordinates( e->pos() );
      updateRectRubberBand();
      const QgsGeometry geom = buildRectGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      break;
    }

    case Shape::Circle:
    {
      if ( !mDragging )
        return;
      mDragging = false;
      mCurrent = toMapCoordinates( e->pos() );
      mCircleRadius = std::sqrt( mCurrent.sqrDist( mAnchor ) );
      updateCircleRubberBand();
      const QgsGeometry geom = buildCircleGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      break;
    }

    case Shape::Sector:
    case Shape::Polyline:
    case Shape::Polygon:
      // Sector advances on press; vertex addition is handled in press; release does nothing extra.
      break;
  }
}

void KadasShapeCaptureMapTool::canvasDoubleClickEvent( QgsMapMouseEvent *e )
{
  if ( mShape != Shape::Polyline && mShape != Shape::Polygon )
    return;
  if ( !mCapturing )
    return;
  // The first click of the double-click already added a vertex via canvasPressEvent.
  Q_UNUSED( e );
  const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
  if ( !geom.isEmpty() )
    emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
  mCapturing = false;
}

void KadasShapeCaptureMapTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    clear();
    e->accept();
    return;
  }
  if ( ( mShape == Shape::Polyline || mShape == Shape::Polygon ) && mCapturing )
  {
    if ( e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete )
    {
      if ( !mVertices.isEmpty() )
      {
        mVertices.removeLast();
        updatePolyRubberBand( QgsPointXY(), false );
      }
      e->accept();
      return;
    }
    if ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter )
    {
      const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      mCapturing = false;
      e->accept();
      return;
    }
  }
  QgsMapTool::keyPressEvent( e );
}

void KadasShapeCaptureMapTool::resetRubberBand()
{
  delete mRubberBand;
  mRubberBand = nullptr;
}

void KadasShapeCaptureMapTool::updateRectRubberBand()
{
  if ( !mRubberBand )
  {
    mRubberBand = new QgsRubberBand( canvas(), Qgis::GeometryType::Polygon );
    mRubberBand->setStrokeColor( QColor( 227, 22, 28, 255 ) );
    mRubberBand->setFillColor( QColor( 227, 22, 28, 63 ) );
    mRubberBand->setWidth( 2 );
  }
  mRubberBand->setToGeometry( buildRectGeometry(), nullptr );
}

void KadasShapeCaptureMapTool::updateCircleRubberBand()
{
  if ( !mRubberBand )
  {
    mRubberBand = new QgsRubberBand( canvas(), Qgis::GeometryType::Polygon );
    mRubberBand->setStrokeColor( QColor( 227, 22, 28, 255 ) );
    mRubberBand->setFillColor( QColor( 227, 22, 28, 63 ) );
    mRubberBand->setWidth( 2 );
  }
  mRubberBand->setToGeometry( buildCircleGeometry(), nullptr );
}

void KadasShapeCaptureMapTool::updateSectorRubberBand()
{
  if ( !mRubberBand )
  {
    mRubberBand = new QgsRubberBand( canvas(), Qgis::GeometryType::Polygon );
    mRubberBand->setStrokeColor( QColor( 227, 22, 28, 255 ) );
    mRubberBand->setFillColor( QColor( 227, 22, 28, 63 ) );
    mRubberBand->setWidth( 2 );
  }
  mRubberBand->setToGeometry( buildSectorGeometry(), nullptr );
}

void KadasShapeCaptureMapTool::updatePolyRubberBand( const QgsPointXY &cursor, bool hasCursor )
{
  const Qgis::GeometryType bandType = ( mShape == Shape::Polygon ) ? Qgis::GeometryType::Polygon : Qgis::GeometryType::Line;
  if ( !mRubberBand )
  {
    mRubberBand = new QgsRubberBand( canvas(), bandType );
    mRubberBand->setStrokeColor( QColor( 227, 22, 28, 255 ) );
    mRubberBand->setFillColor( QColor( 227, 22, 28, 63 ) );
    mRubberBand->setWidth( 2 );
  }
  mRubberBand->reset( bandType );
  for ( const QgsPointXY &v : mVertices )
    mRubberBand->addPoint( v, false );
  if ( hasCursor )
    mRubberBand->addPoint( cursor, true );
  else
    mRubberBand->updatePosition();
  mRubberBand->update();
}

QgsGeometry KadasShapeCaptureMapTool::buildRectGeometry() const
{
  const double xmin = std::min( mAnchor.x(), mCurrent.x() );
  const double xmax = std::max( mAnchor.x(), mCurrent.x() );
  const double ymin = std::min( mAnchor.y(), mCurrent.y() );
  const double ymax = std::max( mAnchor.y(), mCurrent.y() );
  if ( xmin == xmax || ymin == ymax )
    return QgsGeometry();
  return QgsGeometry::fromRect( QgsRectangle( xmin, ymin, xmax, ymax ) );
}

QgsGeometry KadasShapeCaptureMapTool::buildCircleGeometry() const
{
  if ( mCircleRadius <= 0 )
    return QgsGeometry();
  return circlePolygon( mAnchor, mCircleRadius );
}

QgsGeometry KadasShapeCaptureMapTool::buildSectorGeometry() const
{
  if ( mCircleRadius <= 0 )
    return QgsGeometry();
  return sectorPolygon( mAnchor, mCircleRadius, mSectorStartAngle, mSectorStopAngle );
}

QgsGeometry KadasShapeCaptureMapTool::buildPolylineGeometry() const
{
  if ( mVertices.size() < 2 )
    return QgsGeometry();
  return QgsGeometry::fromPolylineXY( mVertices.toList() );
}

QgsGeometry KadasShapeCaptureMapTool::buildPolygonGeometry() const
{
  if ( mVertices.size() < 3 )
    return QgsGeometry();
  return QgsGeometry::fromPolygonXY( QVector<QVector<QgsPointXY>>() << mVertices );
}

QgsGeometry KadasShapeCaptureMapTool::circlePolygon( const QgsPointXY &center, double radius, int segments )
{
  QVector<QgsPointXY> ring;
  ring.reserve( segments + 1 );
  for ( int i = 0; i < segments; ++i )
  {
    const double a = 2.0 * M_PI * static_cast<double>( i ) / static_cast<double>( segments );
    ring.append( QgsPointXY( center.x() + radius * std::cos( a ), center.y() + radius * std::sin( a ) ) );
  }
  ring.append( ring.first() );
  return QgsGeometry::fromPolygonXY( QVector<QVector<QgsPointXY>>() << ring );
}

QgsGeometry KadasShapeCaptureMapTool::sectorPolygon( const QgsPointXY &center, double radius, double startAngle, double stopAngle, int segments )
{
  const double sweep = stopAngle - startAngle;
  if ( sweep >= 2 * M_PI - std::numeric_limits<float>::epsilon() )
    return circlePolygon( center, radius, segments );

  const int arcSegments = std::max( 2, static_cast<int>( std::ceil( segments * sweep / ( 2 * M_PI ) ) ) );
  QVector<QgsPointXY> ring;
  ring.reserve( arcSegments + 3 );
  ring.append( center );
  for ( int i = 0; i <= arcSegments; ++i )
  {
    const double a = startAngle + sweep * static_cast<double>( i ) / static_cast<double>( arcSegments );
    ring.append( QgsPointXY( center.x() + radius * std::cos( a ), center.y() + radius * std::sin( a ) ) );
  }
  ring.append( center );
  return QgsGeometry::fromPolygonXY( QVector<QVector<QgsPointXY>>() << ring );
}
