/***************************************************************************
    kadasgeometryrubberband.cpp
    ---------------------------
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

#include <QPainter>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgscompoundcurve.h>
#include <qgis/qgscurve.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsgeometrycollection.h>
#include <qgis/qgsgeometryengine.h>
#include <qgis/qgsgeometryutils.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include "kadasgeometryrubberband.h"

KadasGeometryRubberBand::KadasGeometryRubberBand( QgsMapCanvas* mapCanvas, QgsWkbTypes::GeometryType geomType )
    : QgsMapCanvasItem( mapCanvas )
    , mGeometry( 0 )
    , mPen( Qt::red )
    , mBrush( Qt::red )
    , mIconSize( 5 )
    , mIconType( ICON_BOX )
    , mIconPen( Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin )
    , mIconBrush( Qt::transparent )
    , mGeometryType( geomType )
    , mMeasurementMode( MEASURE_NONE )
    , mMeasurer( 0 )
{
  mTranslationOffset[0] = 0.;
  mTranslationOffset[1] = 0.;

  connect( mapCanvas, &QgsMapCanvas::mapCanvasRefreshed, this, &KadasGeometryRubberBand::redrawMeasurements );
  connect( mapCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasGeometryRubberBand::configureDistanceArea );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasGeometryRubberBand::configureDistanceArea );
  configureDistanceArea();
}

KadasGeometryRubberBand::~KadasGeometryRubberBand()
{
  qDeleteAll( mMeasurementLabels );
  delete mGeometry;
  delete mMeasurer;
}

void KadasGeometryRubberBand::updatePosition()
{
  setRect( rubberBandRectangle() );
  QgsMapCanvasItem::updatePosition();
}

void KadasGeometryRubberBand::paint( QPainter* painter )
{
  if ( !mGeometry || !painter )
  {
    return;
  }

  painter->save();
  painter->translate( -pos() );

  if ( mGeometryType == QgsWkbTypes::PolygonGeometry )
  {
    painter->setBrush( mBrush );
  }
  else
  {
    painter->setBrush( Qt::NoBrush );
  }
  painter->setPen( mPen );


  QgsAbstractGeometry* paintGeom = mGeometry->clone();

  QTransform mapToCanvas = mMapCanvas->getCoordinateTransform()->transform();
  QTransform translationOffset = QTransform::fromTranslate( mTranslationOffset[0], mTranslationOffset[1] );

  paintGeom->transform( translationOffset * mapToCanvas );
  paintGeom->draw( *painter );

  //draw vertices
  QgsVertexId vertexId;
  QgsPoint vertex;
  while ( paintGeom->nextVertex( vertexId, vertex ) )
  {
    if ( !mHiddenNodes.contains( vertexId ) )
    {
      drawVertex( painter, vertex.x(), vertex.y() );
    }
  }

  delete paintGeom;
  painter->restore();
}

void KadasGeometryRubberBand::drawVertex( QPainter* p, double x, double y )
{
  qreal s = ( mIconSize - 1 ) / 2;
  p->save();
  p->setPen( mIconPen );
  p->setBrush( mIconBrush );

  switch ( mIconType )
  {
    case ICON_NONE:
      break;

    case ICON_CROSS:
      p->drawLine( QLineF( x - s, y, x + s, y ) );
      p->drawLine( QLineF( x, y - s, x, y + s ) );
      break;

    case ICON_X:
      p->drawLine( QLineF( x - s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x + s, y - s ) );
      break;

    case ICON_BOX:
      p->drawLine( QLineF( x - s, y - s, x + s, y - s ) );
      p->drawLine( QLineF( x + s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x - s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x - s, y - s ) );
      break;

    case ICON_FULL_BOX:
      p->drawRect( x - s, y - s, mIconSize, mIconSize );
      break;

    case ICON_CIRCLE:
      p->drawEllipse( x - s, y - s, mIconSize, mIconSize );
      break;

    case ICON_TRIANGLE:
      p->drawLine( QLineF( x - s, y + s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x, y - s ) );
      p->drawLine( QLineF( x, y - s, x - s, y + s ) );
      break;

    case ICON_FULL_TRIANGLE:
      p->drawPolygon( QPolygonF() <<
                      QPointF( x - s, y + s ) <<
                      QPointF( x + s, y + s ) <<
                      QPointF( x, y - s ) );
      break;
  }
  p->restore();
}

void KadasGeometryRubberBand::configureDistanceArea()
{
  mDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
  mDa.setSourceCrs( mMapCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
}

void KadasGeometryRubberBand::redrawMeasurements()
{
  qDeleteAll( mMeasurementLabels );
  mMeasurementLabels.clear();
  mPartMeasurements.clear();
  if ( mGeometry )
  {
    if ( mMeasurementMode != MEASURE_NONE )
    {
      if ( dynamic_cast<QgsGeometryCollection*>( mGeometry ) )
      {
        QgsGeometryCollection* collection = static_cast<QgsGeometryCollection*>( mGeometry );
        for ( int i = 0, n = collection->numGeometries(); i < n; ++i )
        {
          measureGeometry( collection->geometryN( i ), i );
        }
      }
      else
      {
        measureGeometry( mGeometry, 0 );
      }
    }
  }
}

void KadasGeometryRubberBand::setGeometry( QgsAbstractGeometry* geom , const QList<QgsVertexId> &hiddenNodes )
{
  mHiddenNodes.clear();
  delete mGeometry;
  mGeometry = geom;
  qDeleteAll( mMeasurementLabels );
  mMeasurementLabels.clear();
  mPartMeasurements.clear();

  if ( !mGeometry )
  {
    setRect( QgsRectangle() );
    return;
  }
  mHiddenNodes = hiddenNodes;

  setRect( rubberBandRectangle() );

  if ( mMeasurementMode != MEASURE_NONE )
  {
    if ( dynamic_cast<QgsGeometryCollection*>( mGeometry ) )
    {
      QgsGeometryCollection* collection = static_cast<QgsGeometryCollection*>( mGeometry );
      for ( int i = 0, n = collection->numGeometries(); i < n; ++i )
      {
        measureGeometry( collection->geometryN( i ), i );
      }
    }
    else
    {
      measureGeometry( mGeometry, 0 );
    }
  }
}

bool KadasGeometryRubberBand::contains( const QgsPoint& p, double tol ) const
{
  if ( !mGeometry )
  {
    return false;
  }

  QgsPolygon filterRect;
  QgsLineString* exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( p.x() - tol, p.y() - tol )
                       << QgsPoint( p.x() + tol, p.y() - tol )
                       << QgsPoint( p.x() + tol, p.y() + tol )
                       << QgsPoint( p.x() - tol, p.y() + tol )
                       << QgsPoint( p.x() - tol, p.y() - tol ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine( mGeometry );
  bool intersects = geomEngine->intersects( &filterRect );
  delete geomEngine;
  return intersects;
}

void KadasGeometryRubberBand::setTranslationOffset( double dx, double dy )
{
  mTranslationOffset[0] = dx;
  mTranslationOffset[1] = dy;
  if ( mGeometry )
  {
    setRect( rubberBandRectangle() );
  }
}

void KadasGeometryRubberBand::moveVertex( const QgsVertexId& id, const QgsPoint& newPos )
{
  if ( mGeometry )
  {
    mGeometry->moveVertex( id, newPos );
    setRect( rubberBandRectangle() );
  }
}

void KadasGeometryRubberBand::setFillColor( const QColor& c )
{
  mBrush.setColor( c );
}

QColor KadasGeometryRubberBand::fillColor() const
{
  return mBrush.color();
}

void KadasGeometryRubberBand::setOutlineColor( const QColor& c )
{
  mPen.setColor( c );
}

QColor KadasGeometryRubberBand::outlineColor() const
{
  return mPen.color();
}

void KadasGeometryRubberBand::setOutlineWidth( int width )
{
  mPen.setWidth( width );
  setRect( rubberBandRectangle() );
}

int KadasGeometryRubberBand::outlineWidth() const
{
  return mPen.width();
}

void KadasGeometryRubberBand::setLineStyle( Qt::PenStyle penStyle )
{
  mPen.setStyle( penStyle );
}

Qt::PenStyle KadasGeometryRubberBand::lineStyle() const
{
  return mPen.style();
}

void KadasGeometryRubberBand::setBrushStyle( Qt::BrushStyle brushStyle )
{
  mBrush.setStyle( brushStyle );
}

Qt::BrushStyle KadasGeometryRubberBand::brushStyle() const
{
  return mBrush.style();
}

void KadasGeometryRubberBand::setIconSize( int iconSize )
{
  mIconSize = iconSize;
  setRect( rubberBandRectangle() );
}

void KadasGeometryRubberBand::setIconFillColor( const QColor& c )
{
  mIconBrush.setColor( c );
}

void KadasGeometryRubberBand::setIconOutlineColor( const QColor& c )
{
  mIconPen.setColor( c );
}

void KadasGeometryRubberBand::setIconOutlineWidth( int width )
{
  mIconPen.setWidth( width );
}

void KadasGeometryRubberBand::setIconLineStyle( Qt::PenStyle penStyle )
{
  mIconPen.setStyle( penStyle );
}

void KadasGeometryRubberBand::setIconBrushStyle( Qt::BrushStyle brushStyle )
{
  mIconBrush.setStyle( brushStyle );
}

void KadasGeometryRubberBand::setMeasurementMode(MeasurementMode measurementMode, QgsUnitTypes::DistanceUnit distanceUnit, QgsUnitTypes::AreaUnit areaUnit, QgsUnitTypes::AngleUnit angleUnit, AzimuthNorth azimuthNorth)
{
  mMeasurementMode = measurementMode;
  mDistanceUnit = distanceUnit;
  mAreaUnit = areaUnit;
  mAngleUnit = angleUnit;
  mAzimuthNorth = azimuthNorth;
  redrawMeasurements();
}

QString KadasGeometryRubberBand::getTotalMeasurement() const
{
  if ( mMeasurementMode == MEASURE_ANGLE || mMeasurementMode == MEASURE_AZIMUTH )
  {
    return ""; /* Does not make sense for angles */
  }
  double total = 0;
  for ( double value : mPartMeasurements )
  {
    total += value;
  }
  if ( mMeasurementMode == MEASURE_LINE || mMeasurementMode == MEASURE_LINE_AND_SEGMENTS )
  {
    return formatMeasurement( total, false );
  }
  else
  {
    return formatMeasurement( total, true );
  }
}

QgsRectangle KadasGeometryRubberBand::rubberBandRectangle() const
{
  if ( !mGeometry )
  {
    return QgsRectangle();
  }
  double scale = mMapCanvas->mapUnitsPerPixel();
  double s = ( mIconSize - 1 ) / 2.0 * scale;
  double p = mPen.width() * scale;
  double d = s + p;
  QgsRectangle bbox = mGeometry->boundingBox();
  bbox.setXMinimum( bbox.xMinimum() - d + mTranslationOffset[0] );
  bbox.setYMinimum( bbox.yMinimum() - d + mTranslationOffset[1] );
  bbox.setXMaximum( bbox.xMaximum() + d + mTranslationOffset[0] );
  bbox.setYMaximum( bbox.yMaximum() + d + mTranslationOffset[1] );
  return bbox;
}

void KadasGeometryRubberBand::measureGeometry( QgsAbstractGeometry *geometry, int part )
{
  QStringList measurements;
  if ( mMeasurer )
  {
    for ( const Measurer::Measurement& measurement : mMeasurer->measure( mMeasurementMode, part, geometry, mPartMeasurements ) )
    {
      QString value = "";
      if ( measurement.type == Measurer::Measurement::Length )
      {
        value = formatMeasurement( measurement.value, false );
      }
      else if ( measurement.type == Measurer::Measurement::Area )
      {
        value = formatMeasurement( measurement.value, true );
      }
      else if ( measurement.type == Measurer::Measurement::Angle )
      {
        value = formatAngle( measurement.value );
      }
      if ( !measurement.label.isEmpty() )
      {
        value = QString( "%1: %2" ).arg( measurement.label ).arg( value );
      }
      measurements.append( value );
    }
  }
  else
  {
    switch ( mMeasurementMode )
    {
      case MEASURE_LINE:
        if ( dynamic_cast<QgsCurve*>( geometry ) )
        {
          double length = mDa.measureLength( QgsGeometry( geometry->clone() ) );
          mPartMeasurements.append( length );
          measurements.append( formatMeasurement( length, false ) );
        }
        break;
      case MEASURE_LINE_AND_SEGMENTS:
        if ( dynamic_cast<QgsCurve*>( geometry->clone() ) )
        {
          QgsVertexId vid;
          QgsPoint p;
          QList<QgsPoint> points;
          while ( geometry->nextVertex( vid, p ) )
          {
            if ( !mHiddenNodes.contains( vid ) )
            {
              points.append( p );
            }
          }
          double totLength = 0;
          for ( int i = 0, n = points.size() - 1; i < n; ++i )
          {
            QgsPoint p1( points[i].x(), points[i].y() );
            QgsPoint p2( points[i+1].x(), points[i+1].y() );
            double segmentLength = mDa.measureLine( p1, p2 );
            totLength += segmentLength;
            if ( n > 1 )
            {
              addMeasurements( QStringList() << formatMeasurement( segmentLength, false ), QgsPoint( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
            }
          }
          if ( !points.isEmpty() )
          {
            mPartMeasurements.append( totLength );
            QString totLengthStr = QObject::tr( "KadasGeometryRubberBand", "Tot.: %1" ).arg( formatMeasurement( totLength, false ) );
            addMeasurements( QStringList() << totLengthStr, points.last() );
          }
        }
        break;
      case MEASURE_AZIMUTH:
        if ( dynamic_cast<QgsCurve*>( geometry ) )
        {
          QgsVertexId vid;
          QgsPoint p;
          QList<QgsPoint> points;
          while ( geometry->nextVertex( vid, p ) )
          {
            if ( !mHiddenNodes.contains( vid ) )
            {
              points.append( p );
            }
          }
          for ( int i = 0, n = points.size() - 1; i < n; ++i )
          {
            QgsPoint p1( points[i].x(), points[i].y() );
            QgsPoint p2( points[i+1].x(), points[i+1].y() );

            double angle = 0;
            if ( mAzimuthNorth == AZIMUTH_NORTH_GEOGRAPHIC )
            {
              angle = mDa.bearing( p1, p2 );
            }
            else
            {
              angle = qAtan2( p2.x() - p1.x(), p2.y() - p1.y() );
            }
            angle = qRound( angle *  1000 ) / 1000.;
            angle = angle < 0 ? angle + 2 * M_PI : angle;
            angle = angle >= 2 * M_PI ? angle - 2 * M_PI : angle;
            mPartMeasurements.append( angle );
            QString segmentLength = formatAngle( angle );
            addMeasurements( QStringList() << segmentLength, QgsPoint( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
          }
        }
        break;
      case MEASURE_ANGLE:
        // Note: only works with circular sector geometry
        if ( dynamic_cast<QgsCurvePolygon*>( geometry ) && dynamic_cast<const QgsCompoundCurve*>( static_cast<QgsCurvePolygon*>( geometry )->exteriorRing() ) )
        {
          const QgsCompoundCurve* curve = static_cast<const QgsCompoundCurve*>( static_cast<QgsCurvePolygon*>( geometry )->exteriorRing() );
          if ( !curve->isEmpty() )
          {
            if ( dynamic_cast<const QgsCircularString*>( curve->curveAt( 0 ) ) )
            {
              const QgsCircularString* circularString = static_cast<const QgsCircularString*>( curve->curveAt( 0 ) );
              if ( circularString->vertexCount() == 3 )
              {
                QgsPoint p1 = circularString->pointN( 0 );
                QgsPoint p2 = circularString->pointN( 1 );
                QgsPoint p3 = circularString->pointN( 2 );
                double angle;
                if ( p1 == p3 )
                {
                  angle = 2 * M_PI;
                }
                else
                {
                  double radius, cx, cy;
                  QgsGeometryUtils::circleCenterRadius( p1, p2, p3, radius, cx, cy );

                  double azimuthOne = mDa.bearing( QgsPoint( cx, cy ), QgsPoint( p1.x(), p1.y() ) );
                  double azimuthTwo = mDa.bearing( QgsPoint( cx, cy ), QgsPoint( p3.x(), p3.y() ) );
                  azimuthOne = azimuthOne < 0 ? azimuthOne + 2 * M_PI : azimuthOne;
                  azimuthTwo = azimuthTwo < 0 ? azimuthTwo + 2 * M_PI : azimuthTwo;
                  azimuthTwo = azimuthTwo < azimuthOne ? azimuthTwo + 2 * M_PI : azimuthTwo;
                  angle = azimuthTwo - azimuthOne;
                }
                mPartMeasurements.append( angle );
                QString segmentLength = formatAngle( angle );
                addMeasurements( QStringList() << segmentLength, p2 );
              }
            }
            else if ( dynamic_cast<const QgsLineString*>( curve->curveAt( 0 ) ) )
            {
              const QgsLineString* lineString = static_cast<const QgsLineString*>( curve->curveAt( 0 ) );
              if ( lineString->vertexCount() == 3 )
              {
                mPartMeasurements.append( 0 );
                addMeasurements( QStringList() << formatAngle( 0 ), lineString->pointN( 1 ) );
              }
            }
          }
        }
        break;
      case MEASURE_POLYGON:
        if ( dynamic_cast<QgsCurvePolygon*>( geometry ) )
        {
          double area = mDa.measureArea( QgsGeometry( static_cast<QgsCurvePolygon*>( geometry )->exteriorRing()->clone() ) );
          mPartMeasurements.append( area );
          measurements.append( formatMeasurement( area, true ) );
        }
        break;
      case MEASURE_RECTANGLE:
        if ( dynamic_cast<QgsPolygon*>( geometry ) )
        {
          double area = mDa.measureArea( QgsGeometry( static_cast<QgsCurvePolygon*>( geometry )->exteriorRing()->clone() ) );
          mPartMeasurements.append( area );
          measurements.append( formatMeasurement( area, true ) );
          const QgsCurve* ring = static_cast<QgsPolygon*>( geometry )->exteriorRing();
          if ( ring->vertexCount() >= 4 )
          {
            QgsPointSequence points;
            ring->points( points );
            QgsPoint p1( points[0].x(), points[0].y() );
            QgsPoint p2( points[2].x(), points[2].y() );
            QString width = formatMeasurement( mDa.measureLine( p1, QgsPoint( p2.x(), p1.y() ) ), false );
            QString height = formatMeasurement( mDa.measureLine( p1, QgsPoint( p1.x(), p2.y() ) ), false );
            measurements.append( QString( "(%1 x %2)" ).arg( width ).arg( height ) );
          }
        }
        break;
      case MEASURE_CIRCLE:
        if ( dynamic_cast<QgsCurvePolygon*>( geometry ) )
        {
          const QgsCurve* ring = static_cast<QgsCurvePolygon*>( geometry )->exteriorRing();
          QgsLineString* polyline = ring->curveToLine();
          double area = mDa.measureArea( QgsGeometry( polyline ) );
          mPartMeasurements.append( area );
          measurements.append( formatMeasurement( area, true ) );
          QgsPoint p1 = ring->vertexAt( QgsVertexId( 0, 0, 0 ) );
          QgsPoint p2 = ring->vertexAt( QgsVertexId( 0, 0, 1 ) );
          measurements.append( QObject::tr( "KadasGeometryRubberBand", "Radius: %1" ).arg( formatMeasurement( mDa.measureLine( QgsPoint( p1.x(), p1.y() ), QgsPoint( p2.x(), p2.y() ) ), false ) ) );
        }
        break;
      case MEASURE_NONE:
        break;
    }
  }
  if ( !measurements.isEmpty() )
  {
    addMeasurements( measurements, geometry->centroid() );
  }
}

QString KadasGeometryRubberBand::formatMeasurement( double value, bool isArea ) const
{
  int decimals = QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt();
  if(isArea) {
    value = mDa.convertAreaMeasurement(value, mAreaUnit);
    return QgsUnitTypes::formatArea(value, decimals, mAreaUnit);
  } else {
    value = mDa.convertLengthMeasurement(value, mDistanceUnit);
    return QgsUnitTypes::formatDistance(value, decimals, mDistanceUnit);
  }
}

QString KadasGeometryRubberBand::formatAngle( double value ) const
{
  int decimals = QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt();
  value = QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::AngleRadians, mAngleUnit);
  return QgsUnitTypes::formatAngle(value, decimals, mAngleUnit);
}

void KadasGeometryRubberBand::addMeasurements( const QStringList& measurements, const QgsPoint& mapPos )
{
  if ( measurements.isEmpty() )
  {
    return;
  }
  QGraphicsTextItem* label = new QGraphicsTextItem();
  mMapCanvas->scene()->addItem( label );
  int red = QSettings().value( "/Qgis/default_measure_color_red", 222 ).toInt();
  int green = QSettings().value( "/Qgis/default_measure_color_green", 155 ).toInt();
  int blue = QSettings().value( "/Qgis/default_measure_color_blue", 67 ).toInt();
  label->setDefaultTextColor( QColor( red, green, blue ) );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );
  label->setPos( toCanvasCoordinates( QgsPoint( mapPos.x(), mapPos.y() ) ) );
  QString html = QString( "<div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">" ) + measurements.join( "</div><div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">" ) + QString( "</div>" );
  label->setHtml( html );
  label->setZValue( zValue() + 1 );
  mMeasurementLabels.append( label );
}
