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

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgsgeometry.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadaspolygonitem.h>


KadasPolygonItem::KadasPolygonItem(const QgsCoordinateReferenceSystem &crs, bool geodesic, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  mGeodesic = geodesic;
  clear();
}

bool KadasPolygonItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->points.append(QList<QgsPointXY>());
  state()->points.last().append(firstPoint);
  state()->points.last().append(firstPoint);
  recomputeDerived();
  return true;
}

bool KadasPolygonItem::startPart(const QList<double>& attributeValues)
{
  QgsPoint point(attributeValues[AttrX], attributeValues[AttrY]);
  return startPart(point);
}

void KadasPolygonItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  state()->points.last().last() = p;
  recomputeDerived();
}

void KadasPolygonItem::setCurrentAttributes(const QList<double>& values)
{
  state()->points.last().last().setX(values[AttrX]);
  state()->points.last().last().setY(values[AttrY]);
  recomputeDerived();
}

bool KadasPolygonItem::continuePart()
{
  // If current point is same as last one, drop last point and end geometry
  int n = state()->points.last().size();
  if(n > 2 && state()->points.last()[n - 1] == state()->points.last()[n - 2]) {
    state()->points.last().removeLast();
    recomputeDerived();
    return false;
  }
  state()->points.last().append(state()->points.last().last());
  recomputeDerived();
  return true;
}

void KadasPolygonItem::endPart()
{
  state()->drawStatus = State::Finished;
}

const QgsMultiPolygon* KadasPolygonItem::geometry() const
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

QgsMultiPolygon* KadasPolygonItem::geometry()
{
  return static_cast<QgsMultiPolygon*>(mGeometry);
}

void KadasPolygonItem::measureGeometry()
{
  double totalArea = 0;
  for(int i = 0, n = geometry()->numGeometries(); i < n; ++i) {
    const QgsPolygon* polygon = static_cast<QgsPolygon*>(geometry()->geometryN(i));

    double area = mDa.measureArea( QgsGeometry( polygon->clone() ) );
    addMeasurements( QStringList() << formatArea(area, areaBaseUnit()), polygon->centroid() );
    totalArea += area;
  }
  mTotalMeasurement = formatArea(totalArea, areaBaseUnit());
}

void KadasPolygonItem::recomputeDerived()
{
  QgsMultiPolygon* multiGeom = new QgsMultiPolygon();

  if ( mGeodesic )
  {
    QgsCoordinateTransform t1( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326"), QgsProject::instance() );
    QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326"), mCrs, QgsProject::instance() );
    GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );

    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<QgsPointXY>& part = state()->points[iPart];
      QgsLineString* ring = new QgsLineString();

      int nPoints = part.size();
      if ( nPoints >= 2 )
      {
        QList<QgsPointXY> wgsPoints;
        for ( const QgsPointXY& point : part )
        {
          wgsPoints.append( t1.transform( point ) );
        }

        double sdist = 500000; // 500km segments
        for ( int i = 0; i < nPoints - 1; ++i )
        {
          int ringSize = ring->vertexCount();
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i+1].y(), wgsPoints[i+1].x() );
          double dist = line.Distance();
          int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
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
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
  else
  {
    for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
    {
      const QList<QgsPointXY>& part = state()->points[iPart];
      QgsLineString* ring = new QgsLineString();
      for ( const QgsPointXY& point : part )
      {
        ring->addVertex( QgsPoint(point) );
      }
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
  setGeometry(multiGeom);
}

QList<KadasMapItem::NumericAttribute> KadasPolygonItem::attributes() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  QList<KadasMapItem::NumericAttribute> attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  return attributes;
}

QList<double> KadasPolygonItem::attributesFromPosition(const QgsPointXY& pos) const
{
  QList<double> values;
  values.insert(AttrX, pos.x());
  values.insert(AttrY, pos.y());
  return values;
}

QgsPointXY KadasPolygonItem::positionFromAttributes(const QList<double>& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}
