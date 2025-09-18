/***************************************************************************
    kadaslineitem.cpp
    -----------------
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

#include <QMenu>
#include <QJsonArray>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/mapitems/kadaslineitem.h"


QJsonObject KadasLineItem::State::serialize() const
{
  QJsonArray pts;
  for ( const QList<KadasItemPos> &part : points )
  {
    QJsonArray prt;
    for ( const KadasItemPos &pos : part )
    {
      QJsonArray p;
      p.append( pos.x() );
      p.append( pos.y() );
      prt.append( p );
    }
    pts.append( prt );
  }
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  json["points"] = pts;
  return json;
}

bool KadasLineItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  points.clear();
  QJsonArray pts = json["points"].toArray();
  for ( QJsonValue prtValue : pts )
  {
    QJsonArray prt = prtValue.toArray();
    QList<KadasItemPos> part;
    for ( QJsonValue pValue : prt )
    {
      QJsonArray p = pValue.toArray();
      part.append( KadasItemPos( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() ) );
    }
    points.append( part );
  }
  return true;
}

KadasLineItem::KadasLineItem( bool geodesic )
  : KadasGeometryItem()
{
  mGeodesic = geodesic;
  clear();
}

void KadasLineItem::setGeodesic( bool geodesic )
{
  mGeodesic = geodesic;
  update();
  emit propertyChanged();
}

KadasItemPos KadasLineItem::position() const
{
  double x = 0., y = 0.;
  int n = 0;
  for ( const QList<KadasItemPos> &part : constState()->points )
  {
    for ( const KadasItemPos &point : part )
    {
      x += point.x();
      y += point.y();
    }
    n += part.size();
  }
  n = std::max( 1, n );
  return KadasItemPos( x / n, y / n );
}

void KadasLineItem::setPosition( const KadasItemPos &pos )
{
  KadasItemPos prevPos = position();
  double dx = pos.x() - prevPos.x();
  double dy = pos.y() - prevPos.y();
  for ( QList<KadasItemPos> &part : state()->points )
  {
    for ( KadasItemPos &point : part )
    {
      point.setX( point.x() + dx );
      point.setY( point.y() + dy );
    }
  }
  if ( mGeometry )
  {
    mGeometry->transformVertices( [dx, dy]( const QgsPoint &p ) { return QgsPoint( p.x() + dx, p.y() + dy ); } );
  }
  update();
}

QList<KadasMapItem::Node> KadasLineItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> nodes;
  for ( const QList<KadasItemPos> &part : constState()->points )
  {
    for ( const KadasItemPos &pos : part )
    {
      nodes.append( { toMapPos( pos, settings ) } );
    }
  }
  return nodes;
}

bool KadasLineItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  KadasItemPos itemPos = toItemPos( firstPoint, mapSettings );
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->points.append( QList<KadasItemPos>() );
  state()->points.last().append( itemPos );
  state()->points.last().append( itemPos );
  recomputeDerived();
  return true;
}

bool KadasLineItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasLineItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  state()->points.last().last() = toItemPos( p, mapSettings );
  recomputeDerived();
}

void KadasLineItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  setCurrentPoint( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

bool KadasLineItem::continuePart( const QgsMapSettings &mapSettings )
{
  // If current point is same as last one, drop last point and end geometry
  int n = state()->points.last().size();
  if ( n > 2 && state()->points.last()[n - 1] == state()->points.last()[n - 2] )
  {
    state()->points.last().removeLast();
    recomputeDerived();
    return false;
  }
  state()->points.last().append( state()->points.last().last() );
  recomputeDerived();
  return true;
}


void KadasLineItem::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasLineItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasLineItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasLineItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasLineItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  for ( int iPart = 0, nParts = constState()->points.size(); iPart < nParts; ++iPart )
  {
    const QList<KadasItemPos> &part = constState()->points[iPart];
    for ( int iVert = 0, nVerts = part.size(); iVert < nVerts; ++iVert )
    {
      KadasMapPos testPos = toMapPos( part[iVert], mapSettings );
      if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
      {
        return EditContext( QgsVertexId( iPart, 0, iVert ), testPos, drawAttribs() );
      }
    }
  }
  QgsRectangle rect = pointWithTolerance( pos, pickTol( mapSettings ) );
  if ( intersects( rect, mapSettings ) )
  {
    KadasMapPos refPos = toMapPos( constState()->points.front().front(), mapSettings );
    return EditContext( QgsVertexId(), refPos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasLineItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part >= 0 && context.vidx.part < state()->points.size()
       && context.vidx.vertex >= 0 && context.vidx.vertex < state()->points[context.vidx.part].size() )
  {
    state()->points[context.vidx.part][context.vidx.vertex] = toItemPos( newPoint, mapSettings );
    recomputeDerived();
  }
  else
  {
    // Move geometry a whole
    KadasMapPos refMapPos = toMapPos( constState()->points.front().front(), mapSettings );
    for ( QList<KadasItemPos> &part : state()->points )
    {
      for ( KadasItemPos &pos : part )
      {
        KadasMapPos mapPos = toMapPos( pos, mapSettings );
        pos = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
      }
    }
    recomputeDerived();
  }
}

void KadasLineItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasLineItem::populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.vertex == 0 )
  {
    menu->addAction( QObject::tr( "Continue line" ), menu, [this, context] {
          std::reverse( state()->points[context.vidx.part].begin(), state()->points[context.vidx.part].end() );
          recomputeDerived();
        } )
      ->setData( static_cast<int>( ContextMenuActions::EditSwitchToDrawingTool ) );
  }
  else if ( context.vidx.part >= 0 && context.vidx.vertex == state()->points[context.vidx.part].size() - 1 )
  {
    menu->addAction( QObject::tr( "Continue line" ) )->setData( static_cast<int>( ContextMenuActions::EditSwitchToDrawingTool ) );
  }
  if ( context.vidx.vertex >= 0 )
  {
    QAction *deleteNodeAction = menu->addAction( QIcon( ":/kadas/icons/delete_node" ), QObject::tr( "Delete node" ), menu, [this, context] {
      state()->points[context.vidx.part].removeAt( context.vidx.vertex );
      recomputeDerived();
    } );
    deleteNodeAction->setEnabled( constState()->points[context.vidx.part].size() > 2 );
  }
  else
  {
    menu->addAction( QIcon( ":/kadas/icons/add_node" ), QObject::tr( "Add node" ), menu, [=] {
      KadasItemPos newPos = toItemPos( clickPos, mapSettings );
      QgsVertexId insPoint = insertionPoint( constState()->points, newPos );
      state()->points[insPoint.part].insert( insPoint.vertex, newPos );
      recomputeDerived();
    } );
  }
}

KadasMapItem::AttribValues KadasLineItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasLineItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

void KadasLineItem::addPartFromGeometry( const QgsAbstractGeometry &geom )
{
  QList<const QgsLineString *> geoms;
  if ( dynamic_cast<const QgsGeometryCollection *>( &geom ) )
  {
    const QgsGeometryCollection &collection = dynamic_cast<const QgsGeometryCollection &>( geom );
    for ( int i = 0, n = collection.numGeometries(); i < n; ++i )
    {
      if ( dynamic_cast<const QgsLineString *>( collection.geometryN( i ) ) )
      {
        geoms.append( static_cast<const QgsLineString *>( collection.geometryN( i ) ) );
      }
    }
  }
  else if ( dynamic_cast<const QgsLineString *>( &geom ) )
  {
    geoms.append( static_cast<const QgsLineString *>( &geom ) );
  }
  for ( const QgsLineString *line : geoms )
  {
    QList<KadasItemPos> points;
    QgsVertexId vidx;
    QgsPoint p;
    while ( line->nextVertex( vidx, p ) )
    {
      points.append( KadasItemPos( p.x(), p.y(), p.z() ) );
    }
    state()->points.append( points );
    endPart();
  }
  recomputeDerived();
}

const QgsMultiLineString *KadasLineItem::geometry() const
{
  return static_cast<QgsMultiLineString *>( mGeometry );
}

QgsMultiLineString *KadasLineItem::geometry()
{
  return static_cast<QgsMultiLineString *>( mGeometry );
}

void KadasLineItem::setMeasurementMode( MeasurementMode measurementMode, Qgis::AngleUnit angleUnit )
{
  setMeasurementsEnabled( true );
  mMeasurementMode = measurementMode;
  mAngleUnit = angleUnit;
  emit geometryChanged(); // Trigger re-measurement
}

void KadasLineItem::measureGeometry()
{
  double totalLength = 0;
  for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
  {
    const QList<KadasItemPos> &part = state()->points[iPart];
    if ( part.size() < 2 )
    {
      continue;
    }

    switch ( mMeasurementMode )
    {
      case MeasurementMode::MeasureLineAndSegments:
      {
        double totLength = 0;
        for ( int i = 1, n = part.size(); i < n; ++i )
        {
          const KadasItemPos &p1 = part[i - 1];
          const KadasItemPos &p2 = part[i];
          double length = mDa.measureLine( p1, p2 );
          totLength += length;
          addMeasurements( QStringList() << formatLength( length, distanceBaseUnit() ), KadasItemPos( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
        }
        QString totLengthStr = QObject::tr( "Tot.: %1" ).arg( formatLength( totLength, distanceBaseUnit() ) );
        addMeasurements( QStringList() << totLengthStr, KadasItemPos::fromPoint( part.last() ), false );
        totalLength += totLength;
        break;
      }
      case MeasurementMode::MeasureAzimuthGeoNorth:
      case MeasurementMode::MeasureAzimuthMapNorth:
      {
        for ( int i = 1, n = part.size(); i < n; ++i )
        {
          const KadasItemPos &p1 = part[i - 1];
          const KadasItemPos &p2 = part[i];
          double angle = computeSegmentAzimut( p1, p2, mMeasurementMode == MeasurementMode::MeasureAzimuthGeoNorth );
          QString segmentAngle = formatAngle( angle, mAngleUnit );
          addMeasurements( QStringList() << segmentAngle, KadasItemPos( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
        }
        break;
      }
      case MeasurementMode::MeasureLineAndSegmentsAndAzimuthGeoNorth:
      case MeasurementMode::MeasureLineAndSegmentsAndAzimuthMapNorth:
      {
        double totLength = 0;
        for ( int i = 1, n = part.size(); i < n; ++i )
        {
          const KadasItemPos &p1 = part[i - 1];
          const KadasItemPos &p2 = part[i];
          double length = mDa.measureLine( p1, p2 );
          double angle = computeSegmentAzimut( p1, p2, mMeasurementMode == MeasurementMode::MeasureLineAndSegmentsAndAzimuthGeoNorth );
          totLength += length;
          QString segmentAngle = formatAngle( angle, mAngleUnit );
          addMeasurements( QStringList() << formatLength( length, distanceBaseUnit() ) << segmentAngle, KadasItemPos( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
        }
        QString totLengthStr = QObject::tr( "Tot.: %1" ).arg( formatLength( totLength, distanceBaseUnit() ) );
        addMeasurements( QStringList() << totLengthStr, KadasItemPos::fromPoint( part.last() ), false );
        totalLength += totLength;
        break;
      }
    }
  }
  mTotalMeasurement = formatLength( totalLength, distanceBaseUnit() );
}

double KadasLineItem::computeSegmentAzimut( const KadasItemPos &p1, const KadasItemPos &p2, bool geoNorth ) const
{
  double angle = 0;
  if ( geoNorth )
  {
    angle = mDa.bearing( p1, p2 );
  }
  else
  {
    angle = std::atan2( p2.x() - p1.x(), p2.y() - p1.y() );
  }
  angle = qRound( angle * 1000 ) / 1000.;
  angle = angle < 0 ? angle + 2 * M_PI : angle;
  angle = angle >= 2 * M_PI ? angle - 2 * M_PI : angle;
  return angle;
}

void KadasLineItem::recomputeDerived()
{
  QgsMultiLineString *multiGeom = new QgsMultiLineString();

  if ( mGeodesic )
  {
    QgsCoordinateTransform t1( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
    QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326" ), mCrs, QgsProject::instance() );
    GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );

    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<KadasItemPos> &part = state()->points[iPart];
      QgsLineString *ring = new QgsLineString();

      int nPoints = part.size();
      if ( nPoints >= 2 )
      {
        QList<QgsPointXY> wgsPoints;
        for ( const KadasItemPos &point : part )
        {
          wgsPoints.append( t1.transform( point ) );
        }

        double sdist = 100000; // 100km segments
        for ( int i = 0; i < nPoints - 1; ++i )
        {
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i + 1].y(), wgsPoints[i + 1].x() );
          double dist = line.Distance();
          int nIntervals = std::max( 1, int( std::ceil( dist / sdist ) ) );
          for ( int j = 0; j < nIntervals; ++j )
          {
            double lat, lon;
            line.Position( j * sdist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
          if ( i == nPoints - 2 )
          {
            double lat, lon;
            line.Position( dist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
        }
      }
      multiGeom->addGeometry( ring );
    }
  }
  else
  {
    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<KadasItemPos> &part = state()->points[iPart];
      QgsLineString *ring = new QgsLineString();
      for ( const KadasItemPos &point : part )
      {
        ring->addVertex( QgsPoint( point ) );
      }
      multiGeom->addGeometry( ring );
    }
  }
  setInternalGeometry( multiGeom );
}
