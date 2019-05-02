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
#include <qgis/qgsrubberband.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/kadasheightprofiledialog.h>
#include <kadas/gui/kadasgeometryrubberband.h>
#include <kadas/gui/maptools/kadasmaptooldrawshape.h>
#include <kadas/gui/maptools/kadasmaptoolheightprofile.h>

KadasMapToolHeightProfile::KadasMapToolHeightProfile( QgsMapCanvas *canvas )
    : QgsMapTool( canvas ), mPicking( false )
{
  setCursor( Qt::ArrowCursor );

  mDrawTool = new KadasMapToolDrawPolyLine( canvas, false );
  mDrawTool->getRubberBand()->setIconType( KadasGeometryRubberBand::ICON_CIRCLE );
  mDrawTool->setParentTool( this );

  QgsSettings settings;
  int red = settings.value( "/Qgis/default_measure_color_red", 255 ).toInt();
  int green = settings.value( "/Qgis/default_measure_color_green", 0 ).toInt();
  int blue = settings.value( "/Qgis/default_measure_color_blue", 0 ).toInt();

  mPosMarker = new QgsRubberBand( canvas, QgsWkbTypes::PointGeometry );
  mPosMarker->setIcon( QgsRubberBand::ICON_CIRCLE );
  mPosMarker->setIconSize( 10 );
  mPosMarker->setFillColor( Qt::white );
  mPosMarker->setStrokeColor( QColor( red, green, blue ) );
  mPosMarker->setWidth( 2 );

  mDialog = new KadasHeightProfileDialog( this, 0, Qt::WindowStaysOnTopHint );
  connect( mDrawTool, &KadasMapToolDrawShape::finished, this, &KadasMapToolHeightProfile::drawFinished );
  connect( mDrawTool, &KadasMapToolDrawShape::cleared, this, &KadasMapToolHeightProfile::drawCleared );
}

KadasMapToolHeightProfile::~KadasMapToolHeightProfile()
{
  delete mDrawTool;
  delete mPosMarker;
}

void KadasMapToolHeightProfile::activate()
{
  mPicking = false;
  mDialog->show();
  mDrawTool->setShowInputWidget( QgsSettings().value( "/Qgis/showNumericInput", false ).toBool() );
  mDrawTool->activate();
  QgsMapTool::activate();
}

void KadasMapToolHeightProfile::deactivate()
{
  mDrawTool->deactivate();
  mDialog->close();
  mDialog->setPoints( QList<QgsPointXY>(), mCanvas->mapSettings().destinationCrs() );
  QgsMapTool::deactivate();
}

void KadasMapToolHeightProfile::setGeometry( const QgsGeometry& geometry, QgsVectorLayer *layer )
{
  mDrawTool->reset();
  mDrawTool->addGeometry( geometry.constGet(), layer->crs() );
  drawFinished();
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
    mDrawTool->canvasPressEvent( e );
  }
}

void KadasMapToolHeightProfile::canvasMoveEvent( QgsMapMouseEvent * e )
{
  if ( !mPicking )
  {
    if ( mDrawTool->getStatus() == KadasMapToolDrawShape::StatusFinished && mDrawTool->getPartCount() > 0 )
    {
      QgsPointXY p = toMapCoordinates( e->pos() );
      QList<QgsPointXY> points;
      mDrawTool->getPart( 0, points );
      double minDist = std::numeric_limits<double>::max();
      int minIdx = 0;
      QgsPoint minPos;
      for ( int i = 0, nPoints = points.size(); i < nPoints - 1; ++i )
      {
        const QgsPointXY& p1 = points[i];
        const QgsPointXY& p2 = points[i + 1];
        QgsPoint pProj = QgsGeometryUtils::projectPointOnSegment( QgsPoint( p ), QgsPoint( p1 ), QgsPoint( p2 ) );
        double dist = pProj.distanceSquared( p.x(), p.y() );
        if ( dist < minDist )
        {
          minDist = dist;
          minPos = pProj;
          minIdx = i;
        }
      }
      if ( qSqrt( minDist ) / mCanvas->mapSettings().mapUnitsPerPixel() < 30. )
      {
        mPosMarker->movePoint( 0, minPos );
        mDialog->setMarkerPos( minIdx, minPos );
      }
    }
    mDrawTool->canvasMoveEvent( e );
  }
}

void KadasMapToolHeightProfile::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPicking )
  {
    mDrawTool->canvasReleaseEvent( e );
  }
  else
  {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), QgsWkbTypes::LineGeometry );
    if ( pickResult.feature.isValid() )
    {
      setGeometry( pickResult.feature.geometry(), static_cast<QgsVectorLayer*>( pickResult.layer ) );
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
  else if ( e->key() == Qt::Key_Escape && mDrawTool->getStatus() == KadasMapToolDrawShape::StatusReady )
  {
    canvas()->unsetMapTool( this );
  }
  else
  {
    mDrawTool->keyReleaseEvent( e );
  }
}


void KadasMapToolHeightProfile::drawCleared()
{
  mPosMarker->reset( QgsWkbTypes::PointGeometry );
  mDialog->clear();
}

void KadasMapToolHeightProfile::drawFinished()
{
  QList<QgsPointXY> points;
  mDrawTool->getPart( 0, points );
  if ( points.size() > 0 )
  {
    mDialog->setPoints( points, mCanvas->mapSettings().destinationCrs() );
    mPosMarker->addPoint( points[0] );
  }
}
