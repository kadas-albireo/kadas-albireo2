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

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/kadasfloatinginputwidget.h"
#include "kadas/gui/maptools/kadasshapecapturemaptool.h"


//! Converts an angle in radians CCW from east to a geographic angle in degrees CW from north.
static double toGeoAngle( double arad )
{
  double ageo = -arad / M_PI * 180 + 90.;
  while ( ageo < 0 )
    ageo += 360.;
  while ( ageo >= 360 )
    ageo -= 360.;
  return ageo;
}

//! Converts a geographic angle in degrees CW from north to radians CCW from east.
static double toRadAngle( double ageo )
{
  double arad = -( ageo - 90. ) / 180. * M_PI;
  while ( arad < 0 )
    arad += 2 * M_PI;
  while ( arad >= 2 * M_PI )
    arad -= 2 * M_PI;
  return arad;
}


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
  if ( mInputWidget )
    setupNumericInput();
}

void KadasShapeCaptureMapTool::clear()
{
  resetRubberBand();
  mDragging = false;
  mCapturing = false;
  mVertices.clear();
  mPolyHasCursor = false;
  mCircleRadius = 0.0;
  mSectorStage = SectorStage::None;
  mSectorStartAngle = 0.0;
  mSectorStopAngle = 0.0;
  emit cleared();
}

void KadasShapeCaptureMapTool::activate()
{
  QgsMapTool::activate();
  setupNumericInput();
}

void KadasShapeCaptureMapTool::deactivate()
{
  clearNumericInput();
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

void KadasShapeCaptureMapTool::addPoint( const QgsPointXY &pos )
{
  switch ( mShape )
  {
    case Shape::Rectangle:
    case Shape::Circle:
      mAnchor = pos;
      mCurrent = pos;
      mDragging = true;
      break;

    case Shape::Sector:
      if ( mSectorStage == SectorStage::None )
      {
        resetRubberBand();
        mAnchor = pos;
        mCircleRadius = 0.0;
        mSectorStartAngle = 0.0;
        mSectorStopAngle = 2 * M_PI;
        mSectorStage = SectorStage::HaveCenter;
      }
      break;

    case Shape::Polyline:
    case Shape::Polygon:
      if ( !mCapturing )
      {
        mVertices.clear();
        mCapturing = true;
      }
      mVertices.append( pos );
      updatePolyRubberBand( pos, false );
      break;
  }
}

void KadasShapeCaptureMapTool::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( e->button() != Qt::LeftButton )
  {
    if ( e->button() == Qt::RightButton && ( mShape == Shape::Polyline || mShape == Shape::Polygon ) && mCapturing )
    {
      // Right click finishes polyline/polygon capture
      mCapturing = false;
      const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
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
  if ( mIgnoreNextMoveEvent )
  {
    // Spurious move event triggered by KadasFloatingInputWidget::adjustCursorAndExtent;
    // processing it would clobber the user-entered values with the (less precise) warped
    // cursor position.
    mIgnoreNextMoveEvent = false;
    return;
  }
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

  updateNumericInput( e );
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
  mCapturing = false;
  const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
  if ( !geom.isEmpty() )
    emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
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
      mCapturing = false;
      const QgsGeometry geom = ( mShape == Shape::Polygon ) ? buildPolygonGeometry() : buildPolylineGeometry();
      if ( !geom.isEmpty() )
        emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
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

void KadasShapeCaptureMapTool::setupNumericInput()
{
  clearNumericInput();
  if ( !QgsSettings().value( "/kadas/showNumericInput", false ).toBool() )
    return;

  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  if ( mShape == Shape::Circle || mShape == Shape::Sector )
    attributes.insert( AttrR, KadasNumericAttribute { "r", KadasNumericAttribute::Type::TypeDistance, 0 } );
  if ( mShape == Shape::Sector )
  {
    attributes.insert( AttrA1, KadasNumericAttribute { QString( QChar( 0x03B1 ) ) + "1", KadasNumericAttribute::Type::TypeAngle, 0 } );
    attributes.insert( AttrA2, KadasNumericAttribute { QString( QChar( 0x03B1 ) ) + "2", KadasNumericAttribute::Type::TypeAngle, 0 } );
  }

  mInputWidget = new KadasFloatingInputWidget( canvas() );
  for ( auto it = attributes.constBegin(), itEnd = attributes.constEnd(); it != itEnd; ++it )
  {
    const KadasNumericAttribute &attribute = it.value();
    KadasFloatingInputWidgetField *attrEdit = new KadasFloatingInputWidgetField( it.key(), attribute.precision( canvas()->mapSettings() ), attribute.min, attribute.max );
    connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasShapeCaptureMapTool::numericInputChanged );
    connect( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasShapeCaptureMapTool::acceptNumericInput );
    mInputWidget->addInputField( attribute.name + ":", attrEdit, attribute.suffix( canvas()->mapSettings() ) );
  }
  mInputWidget->setFocusedInputField( mInputWidget->inputField( AttrX ) );
}

void KadasShapeCaptureMapTool::clearNumericInput()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasShapeCaptureMapTool::updateNumericInput( QgsMapMouseEvent *e )
{
  if ( !mInputWidget )
    return;
  mInputWidget->ensureFocus();
  const KadasAttribValues values = attribsFromState( toMapCoordinates( e->pos() ) );
  for ( auto it = values.constBegin(), itEnd = values.constEnd(); it != itEnd; ++it )
  {
    if ( KadasFloatingInputWidgetField *field = mInputWidget->inputField( it.key() ) )
      field->setValue( it.value() );
  }
  mInputWidget->move( e->pos().x(), e->pos().y() + 20 );
  mInputWidget->show();
  if ( mInputWidget->focusedInputField() )
  {
    mInputWidget->focusedInputField()->setFocus();
    mInputWidget->focusedInputField()->selectAll();
  }
}

KadasAttribValues KadasShapeCaptureMapTool::collectAttributeValues() const
{
  KadasAttribValues values;
  if ( !mInputWidget )
    return values;
  for ( const KadasFloatingInputWidgetField *field : mInputWidget->inputFields() )
    values.insert( field->id(), field->text().toDouble() );
  return values;
}

KadasAttribValues KadasShapeCaptureMapTool::attribsFromState( const QgsPointXY &cursorPos ) const
{
  KadasAttribValues values;
  switch ( mShape )
  {
    case Shape::Rectangle:
    case Shape::Polyline:
    case Shape::Polygon:
      values.insert( AttrX, cursorPos.x() );
      values.insert( AttrY, cursorPos.y() );
      break;

    case Shape::Circle:
      values.insert( AttrX, mDragging ? mAnchor.x() : cursorPos.x() );
      values.insert( AttrY, mDragging ? mAnchor.y() : cursorPos.y() );
      values.insert( AttrR, mDragging ? mCircleRadius : 0 );
      break;

    case Shape::Sector:
    {
      const bool started = mSectorStage != SectorStage::None;
      values.insert( AttrX, started ? mAnchor.x() : cursorPos.x() );
      values.insert( AttrY, started ? mAnchor.y() : cursorPos.y() );
      values.insert( AttrR, started ? mCircleRadius : 0 );
      if ( mSectorStage == SectorStage::HaveRadius )
      {
        values.insert( AttrA1, toGeoAngle( mSectorStartAngle ) );
        values.insert( AttrA2, toGeoAngle( mSectorStopAngle ) );
      }
      else
      {
        values.insert( AttrA1, 0 );
        values.insert( AttrA2, 0 );
      }
      break;
    }
  }
  return values;
}

void KadasShapeCaptureMapTool::numericInputChanged()
{
  if ( !mInputWidget )
    return;
  const KadasAttribValues values = collectAttributeValues();
  const QgsPointXY pos( values[AttrX], values[AttrY] );

  switch ( mShape )
  {
    case Shape::Rectangle:
      if ( mDragging )
      {
        mCurrent = pos;
        updateRectRubberBand();
      }
      break;

    case Shape::Circle:
      if ( mDragging )
      {
        mAnchor = pos;
        mCircleRadius = values[AttrR];
        updateCircleRubberBand();
      }
      break;

    case Shape::Sector:
      if ( mSectorStage != SectorStage::None )
      {
        mAnchor = pos;
        mCircleRadius = values[AttrR];
        mSectorStage = mCircleRadius > 0 ? SectorStage::HaveRadius : SectorStage::HaveCenter;
        mSectorStartAngle = toRadAngle( values[AttrA1] );
        mSectorStopAngle = toRadAngle( values[AttrA2] );
        if ( mSectorStopAngle <= mSectorStartAngle )
          mSectorStopAngle += 2 * M_PI;
        updateSectorRubberBand();
      }
      break;

    case Shape::Polyline:
    case Shape::Polygon:
      if ( mCapturing )
        updatePolyRubberBand( pos, true );
      break;
  }

  // Suppress the spurious move event triggered by adjustCursorAndExtent.
  mIgnoreNextMoveEvent = true;
  mInputWidget->adjustCursorAndExtent( pos );
}

void KadasShapeCaptureMapTool::acceptNumericInput()
{
  if ( !mInputWidget )
    return;
  const KadasAttribValues values = collectAttributeValues();
  const QgsPointXY pos( values[AttrX], values[AttrY] );

  switch ( mShape )
  {
    case Shape::Rectangle:
      if ( !mDragging )
      {
        mAnchor = pos;
        mCurrent = pos;
        mDragging = true;
      }
      else
      {
        mDragging = false;
        mCurrent = pos;
        updateRectRubberBand();
        const QgsGeometry geom = buildRectGeometry();
        if ( !geom.isEmpty() )
          emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      }
      break;

    case Shape::Circle:
      if ( !mDragging )
      {
        mAnchor = pos;
        mCurrent = pos;
        mCircleRadius = values[AttrR];
        mDragging = true;
        if ( mCircleRadius > 0 )
          updateCircleRubberBand();
      }
      else
      {
        mDragging = false;
        mAnchor = pos;
        mCircleRadius = values[AttrR];
        updateCircleRubberBand();
        const QgsGeometry geom = buildCircleGeometry();
        if ( !geom.isEmpty() )
          emit shapeCaptured( geom, canvas()->mapSettings().destinationCrs() );
      }
      break;

    case Shape::Sector:
      switch ( mSectorStage )
      {
        case SectorStage::None:
          resetRubberBand();
          mAnchor = pos;
          mCircleRadius = values[AttrR];
          mSectorStartAngle = 0.0;
          mSectorStopAngle = 2 * M_PI;
          mSectorStage = mCircleRadius > 0 ? SectorStage::HaveRadius : SectorStage::HaveCenter;
          if ( mSectorStage == SectorStage::HaveRadius )
          {
            mSectorStartAngle = toRadAngle( values[AttrA1] );
            mSectorStopAngle = toRadAngle( values[AttrA2] );
            if ( mSectorStopAngle <= mSectorStartAngle )
              mSectorStopAngle += 2 * M_PI;
            updateSectorRubberBand();
          }
          break;

        case SectorStage::HaveCenter:
          mSectorStage = SectorStage::HaveRadius;
          break;

        case SectorStage::HaveRadius:
        {
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
      mVertices.append( pos );
      updatePolyRubberBand( pos, false );
      break;
  }
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
  emit previewChanged();
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
  emit previewChanged();
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
  emit previewChanged();
}

void KadasShapeCaptureMapTool::updatePolyRubberBand( const QgsPointXY &cursor, bool hasCursor )
{
  const Qgis::GeometryType bandType = ( mShape == Shape::Polygon ) ? Qgis::GeometryType::Polygon : Qgis::GeometryType::Line;
  mPolyCursor = cursor;
  mPolyHasCursor = hasCursor;
  if ( !mRubberBand )
  {
    mRubberBand = new QgsRubberBand( canvas(), bandType );
    mRubberBand->setStrokeColor( QColor( 227, 22, 28, 255 ) );
    mRubberBand->setFillColor( QColor( 227, 22, 28, 63 ) );
    mRubberBand->setWidth( 2 );
  }
  QVector<QgsPointXY> displayPoints = mVertices;
  if ( hasCursor )
    displayPoints.append( cursor );
  if ( mGeodesicPreview && displayPoints.size() >= 2 )
    displayPoints = geodesicDisplayPoints( displayPoints );

  mRubberBand->reset( bandType );
  for ( const QgsPointXY &v : displayPoints )
    mRubberBand->addPoint( v, false );
  mRubberBand->updatePosition();
  mRubberBand->update();
  emit previewChanged();
}

QVector<QgsPointXY> KadasShapeCaptureMapTool::geodesicDisplayPoints( const QVector<QgsPointXY> &points ) const
{
  QgsDistanceArea da;
  da.setSourceCrs( canvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  da.setEllipsoid( QStringLiteral( "WGS84" ) );
  if ( !da.willUseEllipsoid() )
    return points;

  constexpr double interval = 100000; // 100km segments
  QVector<QgsPointXY> out;
  out.append( points.first() );
  for ( int i = 0, n = points.size(); i < n - 1; ++i )
  {
    const QVector<QVector<QgsPointXY>> segments = da.geodesicLine( points[i], points[i + 1], interval, false );
    if ( segments.isEmpty() || segments.first().size() < 2 )
    {
      out.append( points[i + 1] );
      continue;
    }
    for ( const QVector<QgsPointXY> &segment : segments )
    {
      for ( int j = 1, m = segment.size(); j < m; ++j )
        out.append( segment[j] );
    }
  }
  return out;
}

QgsGeometry KadasShapeCaptureMapTool::previewGeometry() const
{
  switch ( mShape )
  {
    case Shape::Rectangle:
      return buildRectGeometry();
    case Shape::Circle:
      return buildCircleGeometry();
    case Shape::Sector:
      return buildSectorGeometry();
    case Shape::Polyline:
    case Shape::Polygon:
    {
      QVector<QgsPointXY> pts = mVertices;
      if ( mCapturing && mPolyHasCursor && ( pts.isEmpty() || pts.last() != mPolyCursor ) )
        pts.append( mPolyCursor );
      if ( mShape == Shape::Polygon )
      {
        if ( pts.size() < 3 )
          return QgsGeometry();
        return QgsGeometry::fromPolygonXY( QVector<QVector<QgsPointXY>>() << pts );
      }
      if ( pts.size() < 2 )
        return QgsGeometry();
      return QgsGeometry::fromPolylineXY( pts.toList() );
    }
  }
  return QgsGeometry();
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
