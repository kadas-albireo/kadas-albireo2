/***************************************************************************
    kadaspolygonitem.cpp
    --------------------
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

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadaspolygonitem.h>


KadasPolygonItem::KadasPolygonItem( const QgsCoordinateReferenceSystem &crs, bool geodesic, QObject *parent )
  : KadasGeometryItem( crs, parent )
{
  mGeodesic = geodesic;
  clear();
}

void KadasPolygonItem::setGeodesic( bool geodesic )
{
  mGeodesic = geodesic;
  update();
}

QList<KadasMapItem::Node> KadasPolygonItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> nodes;
  for ( const QList<KadasItemPos> &part : constState()->points )
  {
    for ( const KadasItemPos &pos : part )
    {
      nodes.append( {toMapPos( pos, settings )} );
    }
  }
  return nodes;
}

KadasItemPos KadasPolygonItem::position() const
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

void KadasPolygonItem::setPosition( const KadasItemPos &pos )
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
    mGeometry->transformVertices( [dx, dy]( const QgsPoint & p ) { return QgsPoint( p.x() + dx, p.y() + dy ); } );
  }
  update();
}

bool KadasPolygonItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  KadasItemPos itemPos = toItemPos( firstPoint, mapSettings );
  state()->drawStatus = State::Drawing;
  state()->points.append( QList<KadasItemPos>() );
  state()->points.last().append( itemPos );
  state()->points.last().append( itemPos );
  recomputeDerived();
  return true;
}

bool KadasPolygonItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPolygonItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  state()->points.last().last() = toItemPos( p, mapSettings );
  recomputeDerived();
}

void KadasPolygonItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  setCurrentPoint( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

bool KadasPolygonItem::continuePart( const QgsMapSettings &mapSettings )
{
  // If current point is same as last one, drop last point and end geometry
  int n = state()->points.last().size();
  if ( n > 2 && state()->points.last() [n - 1] == state()->points.last() [n - 2] )
  {
    state()->points.last().removeLast();
    recomputeDerived();
    return false;
  }
  state()->points.last().append( state()->points.last().last() );
  recomputeDerived();
  return true;
}

void KadasPolygonItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasPolygonItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x", NumericAttribute::XCooAttr} );
  attributes.insert( AttrY, NumericAttribute{"y", NumericAttribute::YCooAttr} );
  return attributes;
}

KadasMapItem::AttribValues KadasPolygonItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasPolygonItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPolygonItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  double mup = mapSettings.mapUnitsPerPixel();
  for ( int iPart = 0, nParts = constState()->points.size(); iPart < nParts; ++iPart )
  {
    const QList<KadasItemPos> &part = constState()->points[iPart];
    for ( int iVert = 0, nVerts = part.size(); iVert < nVerts; ++iVert )
    {
      KadasMapPos testPos = toMapPos( part[iVert], mapSettings );
      if ( pos.sqrDist( testPos ) < pickTol( mapSettings ) )
      {
        return EditContext( QgsVertexId( iPart, 0, iVert ), testPos, drawAttribs() );
      }
    }
  }
  return EditContext();
}

void KadasPolygonItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part >= 0 && context.vidx.part < state()->points.size()
       && context.vidx.vertex >= 0 && context.vidx.vertex < state()->points[context.vidx.part].size() )
  {
    state()->points[context.vidx.part][context.vidx.vertex] = toItemPos( newPoint, mapSettings );
    recomputeDerived();
  }
}

void KadasPolygonItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPolygonItem::populateContextMenu( QMenu *menu, const EditContext &context )
{
  if ( context.vidx.vertex >= 0 )
  {
    menu->addAction( tr( "Delete Node" ), menu, [this, context]
    {
      state()->points[context.vidx.part].removeAt( context.vidx.vertex );
      recomputeDerived();
    } );
  }
}

KadasMapItem::AttribValues KadasPolygonItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasPolygonItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

void KadasPolygonItem::addPartFromGeometry( const QgsAbstractGeometry *geom )
{
  if ( dynamic_cast<const QgsPolygon *>( geom ) )
  {
    QList<KadasItemPos> points;
    QgsVertexId vidx;
    QgsPoint p;
    const QgsCurve *ring = static_cast<const QgsPolygon *>( geom )->exteriorRing();
    while ( ring->nextVertex( vidx, p ) )
    {
      points.append( KadasItemPos( p.x(), p.y() ) );
    }
    state()->points.append( points );
    recomputeDerived();
    endPart();
  }
}

const QgsMultiPolygon *KadasPolygonItem::geometry() const
{
  return static_cast<QgsMultiPolygon *>( mGeometry );
}

QgsMultiPolygon *KadasPolygonItem::geometry()
{
  return static_cast<QgsMultiPolygon *>( mGeometry );
}

void KadasPolygonItem::measureGeometry()
{
  double totalArea = 0;
  for ( int i = 0, n = geometry()->numGeometries(); i < n; ++i )
  {
    const QgsPolygon *polygon = static_cast<QgsPolygon *>( geometry()->geometryN( i ) );

    double area = mDa.measureArea( QgsGeometry( polygon->clone() ) );
    addMeasurements( QStringList() << formatArea( area, areaBaseUnit() ), KadasItemPos::fromPoint( polygon->centroid() ) );
    totalArea += area;
  }
  mTotalMeasurement = formatArea( totalArea, areaBaseUnit() );
}

void KadasPolygonItem::recomputeDerived()
{
  QgsMultiPolygon *multiGeom = new QgsMultiPolygon();

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
        wgsPoints.append( t1.transform( part.front() ) );

        double sdist = 500000; // 500km segments
        for ( int i = 0; i < nPoints; ++i )
        {
          int ringSize = ring->vertexCount();
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i + 1].y(), wgsPoints[i + 1].x() );
          double dist = line.Distance();
          int nIntervals = qMax( 1, int ( std::ceil( dist / sdist ) ) );
          for ( int j = 0; j < nIntervals; ++j )
          {
            double lat, lon;
            line.Position( j * sdist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
          if ( i == nPoints - 1 )
          {
            double lat, lon;
            line.Position( dist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
        }
      }
      QgsPolygon *poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
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
      QgsPolygon *poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
  setInternalGeometry( multiGeom );
}
