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

#include <limits>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsgeometryutils.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsrubberband.h>

#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/kadasheightprofiledialog.h"
#include "kadas/gui/maptools/kadasmaptoolheightprofile.h"


KadasMapToolHeightProfile::KadasMapToolHeightProfile( QgsMapCanvas *canvas )
  : KadasShapeCaptureMapTool( canvas, KadasShapeCaptureMapTool::Shape::Polyline )
{
  mPosMarker = new QgsRubberBand( canvas, Qgis::GeometryType::Point );
  mPosMarker->setColor( Qt::blue );
  mPosMarker->setStrokeColor( Qt::blue );
  mPosMarker->setFillColor( Qt::blue );
  mPosMarker->setIcon( QgsRubberBand::ICON_CIRCLE );
  mPosMarker->setIconSize( 8 );

  mDialog = new KadasHeightProfileDialog( this, nullptr, Qt::WindowStaysOnTopHint );

  connect( this, &KadasShapeCaptureMapTool::shapeCaptured, this, &KadasMapToolHeightProfile::onShapeCaptured );
  connect( this, &KadasShapeCaptureMapTool::cleared, this, &KadasMapToolHeightProfile::onCleared );
}

void KadasMapToolHeightProfile::activate()
{
  mPicking = false;
  setCursor( Qt::ArrowCursor );
  mDialog->show();
  KadasShapeCaptureMapTool::activate();
}

void KadasMapToolHeightProfile::deactivate()
{
  mDialog->close();
  mDialog->setPoints( QList<QgsPointXY>(), canvas()->mapSettings().destinationCrs() );
  KadasShapeCaptureMapTool::deactivate();
  delete mPosMarker;
  mPosMarker = nullptr;
}

void KadasMapToolHeightProfile::setGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs )
{
  QgsGeometry g( geom.clone() );
  if ( g.isEmpty() )
    return;
  // Reduce to a flat polyline in canvas CRS
  QVector<QgsPointXY> pts;
  const QgsCoordinateReferenceSystem destCrs = canvas()->mapSettings().destinationCrs();
  QgsCoordinateTransform ct( crs, destCrs, canvas()->mapSettings().transformContext() );
  for ( auto it = g.vertices_begin(); it != g.vertices_end(); ++it )
  {
    QgsPointXY p( ( *it ).x(), ( *it ).y() );
    if ( ct.isValid() && !ct.isShortCircuited() )
    {
      try
      {
        p = ct.transform( p );
      }
      catch ( ... )
      {
        continue;
      }
    }
    pts.append( p );
  }
  if ( pts.size() < 2 )
    return;
  setCapturedPolyline( pts );
  onShapeCaptured( QgsGeometry::fromPolylineXY( pts ), destCrs );
}

void KadasMapToolHeightProfile::setMarkerPos( double distance )
{
  if ( !mPosMarker || mCapturedPoints.isEmpty() )
    return;

  for ( int i = 0, n = mCapturedPoints.size() - 1; i < n; ++i )
  {
    const QgsPointXY &a = mCapturedPoints[i];
    const QgsPointXY &b = mCapturedPoints[i + 1];
    const double segDist = std::sqrt( a.sqrDist( b ) );
    if ( distance < segDist )
    {
      const double k = distance / segDist;
      const double x = a.x() + ( b.x() - a.x() ) * k;
      const double y = a.y() + ( b.y() - a.y() ) * k;
      mPosMarker->reset( Qgis::GeometryType::Point );
      mPosMarker->addPoint( QgsPointXY( x, y ), true );
      return;
    }
    distance -= segDist;
  }
  mPosMarker->reset( Qgis::GeometryType::Point );
  mPosMarker->addPoint( mCapturedPoints.last(), true );
}

void KadasMapToolHeightProfile::pickLine()
{
  mPicking = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void KadasMapToolHeightProfile::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( mPicking )
    return;

  // While the user is still drawing the line, just delegate to the helper.
  if ( isCapturing() || mCapturedPoints.isEmpty() )
  {
    KadasShapeCaptureMapTool::canvasMoveEvent( e );
    return;
  }

  // Captured: project cursor onto the polyline and move the position marker.
  const QgsPointXY p = toMapCoordinates( e->pos() );
  double minDist = std::numeric_limits<double>::max();
  int minIdx = 0;
  QgsPoint minPos;
  QgsCoordinateTransform crst( mCapturedCrs, canvas()->mapSettings().destinationCrs(), canvas()->mapSettings().transformContext() );
  for ( int i = 0, nPoints = mCapturedPoints.size(); i < nPoints - 1; ++i )
  {
    QgsPointXY p1 = crst.transform( mCapturedPoints[i] );
    QgsPointXY p2 = crst.transform( mCapturedPoints[i + 1] );
    QgsPoint pProj = QgsGeometryUtils::projectPointOnSegment( QgsPoint( p ), QgsPoint( p1 ), QgsPoint( p2 ) );
    const double dist = pProj.distanceSquared( p.x(), p.y() );
    if ( dist < minDist )
    {
      minDist = dist;
      minPos = pProj;
      minIdx = i;
    }
  }
  if ( std::sqrt( minDist ) / canvas()->mapSettings().mapUnitsPerPixel() < 30. )
  {
    mPosMarker->reset( Qgis::GeometryType::Point );
    mPosMarker->addPoint( minPos, true );
    mDialog->setMarkerPos( minIdx, minPos, canvas()->mapSettings().destinationCrs() );
  }
}

void KadasMapToolHeightProfile::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPicking )
  {
    KadasShapeCaptureMapTool::canvasReleaseEvent( e );
    return;
  }
  KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( canvas(), toMapCoordinates( e->pos() ), Qgis::GeometryType::Line );
  if ( pickResult.geom )
    setGeometry( *pickResult.geom, pickResult.crs );
  mPicking = false;
  setCursor( Qt::ArrowCursor );
}

void KadasMapToolHeightProfile::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPicking && e->key() == Qt::Key_Escape )
  {
    mPicking = false;
    setCursor( Qt::ArrowCursor );
    return;
  }
  KadasShapeCaptureMapTool::keyReleaseEvent( e );
}

void KadasMapToolHeightProfile::onCleared()
{
  mCapturedPoints.clear();
  mCapturedCrs = QgsCoordinateReferenceSystem();
  if ( mPosMarker )
    mPosMarker->reset( Qgis::GeometryType::Point );
  mDialog->clear();
}

void KadasMapToolHeightProfile::onShapeCaptured( const QgsGeometry &geom, const QgsCoordinateReferenceSystem &crs )
{
  mCapturedPoints.clear();
  for ( auto it = geom.vertices_begin(); it != geom.vertices_end(); ++it )
    mCapturedPoints.append( QgsPointXY( ( *it ).x(), ( *it ).y() ) );
  if ( mCapturedPoints.isEmpty() )
    return;
  mCapturedCrs = crs;
  mDialog->setPoints( mCapturedPoints, crs );
  if ( mPosMarker )
  {
    mPosMarker->reset( Qgis::GeometryType::Point );
    mPosMarker->addPoint( mCapturedPoints.first(), true );
  }
}
/***************************************************************************
    kadasmaptoolheightprofile.cpp
    -----------------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/
