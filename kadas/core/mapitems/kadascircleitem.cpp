/***************************************************************************
    kadascircleitem.cpp
    ----------------------
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


#include <qgis/qgsgeometry.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadascircleitem.h>

KadasCircleItem::KadasCircleItem(const QgsCoordinateReferenceSystem &crs, bool geodesic, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  mGeodesic = geodesic;
  clear();
}

QList<QgsPointXY> KadasCircleItem::nodes() const
{
  QList<QgsPointXY> points;
  for(int i = 0, n = state()->centers.size(); i < n; ++i) {
    const QgsPointXY& center = state()->centers[i];
    points.append(center);
    points.append(QgsPointXY(center.x() + state()->radii[i], center.y()));
  }
  return points;
}

bool KadasCircleItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->centers.append(firstPoint);
  state()->radii.append(0);
  recomputeDerived();
  return true;
}

bool KadasCircleItem::startPart(const QList<double>& attributeValues)
{
  QgsPoint point(attributeValues[AttrX], attributeValues[AttrY]);
  return startPart(point);
}

void KadasCircleItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  const QgsPointXY& center = state()->centers.last();
  state()->radii.last() = qSqrt(p.sqrDist(center));
  recomputeDerived();
}

void KadasCircleItem::setCurrentAttributes(const QList<double>& values)
{
  state()->centers.last().setX(values[AttrX]);
  state()->centers.last().setY(values[AttrY]);
  state()->radii.last() = values[AttrR];
  recomputeDerived();
}

bool KadasCircleItem::continuePart()
{
  // No further action allowed
  return false;
}

void KadasCircleItem::endPart()
{
  state()->drawStatus = State::Finished;
}

const QgsMultiSurface* KadasCircleItem::geometry() const
{
  return static_cast<QgsMultiSurface*>(mGeometry);
}

QgsMultiSurface* KadasCircleItem::geometry()
{
  return static_cast<QgsMultiSurface*>(mGeometry);
}

void KadasCircleItem::measureGeometry()
{
  double totalArea = 0;
  for(int i = 0, n = state()->centers.size(); i < n; ++i) {
    double radius = state()->radii[i];

    double area = radius * radius * M_PI;

    QStringList measurements;
    measurements.append( formatArea( area, areaBaseUnit() ) );
    measurements.append( tr( "Radius: %1" ).arg( formatLength( radius, distanceBaseUnit() ) ) );
    addMeasurements( QStringList() << measurements, state()->centers[i] );
    totalArea += area;
  }
  mTotalMeasurement = formatArea(totalArea, areaBaseUnit());
}

void KadasCircleItem::recomputeDerived()
{
  QgsMultiSurface* multiGeom = new QgsMultiSurface();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    if(mGeodesic) {
      computeGeoCircle(state()->centers[i], state()->radii[i], multiGeom);
    } else {
      computeCircle(state()->centers[i], state()->radii[i], multiGeom);
    }
  }
  setGeometry(multiGeom);
}

void KadasCircleItem::computeCircle(const QgsPointXY &center, double radius, QgsMultiSurface *multiGeom)
{
  QgsCircularString* string = new QgsCircularString();
  string->setPoints(QgsPointSequence()
                    << QgsPoint(center.x(), center.y() + radius)
                    << QgsPoint(center.x() + radius, center.y())
                    << QgsPoint(center.x(), center.y() - radius)
                    << QgsPoint(center.x() - radius, center.y())
                    << QgsPoint(center.x(), center.y() + radius)
  );
  QgsCompoundCurve* curve = new QgsCompoundCurve();
  curve->addCurve(string);
  QgsCurvePolygon* poly = new QgsCurvePolygon();
  poly->setExteriorRing(curve);
  multiGeom->addGeometry(poly);
}

void KadasCircleItem::computeGeoCircle(const QgsPointXY& center, double radius, QgsMultiSurface* multiGeom)
{
  // 1 deg segmentized circle around center
  QgsCoordinateTransform t1( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326"), QgsProject::instance() );
  QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326"), mCrs, QgsProject::instance() );
  QgsPointXY p1 = t1.transform( center );
  QgsPointXY p2 = t1.transform( QgsPointXY(center.x() + radius, center.y()) );
  double clampLatitude = mCrs.authid() == "EPSG:3857" ? 85 : 90;
  if ( p2.y() > 90 )
  {
    p2.setY( 90. - ( p2.y() - 90. ) );
  }
  if ( p2.y() < -90 )
  {
    p2.setY( -90. - ( p2.y() + 90. ) );
  }
  QList<QgsPointXY> wgsPoints;
  for ( int a = 0; a < 360; ++a )
  {
    wgsPoints.append( mDa.destination( p1, radius, a ) );
  }
  // Check if area would cross north or south pole
  // -> Check if destination point at bearing 0 / 180 with given radius would flip longitude
  // -> If crosses north/south pole, add points at lat 90 resp. -90 between points with max resp. min latitude
  QgsPointXY pn = mDa.destination( p1, radius, 0 );
  QgsPointXY ps = mDa.destination( p1, radius, 180 );
  int shift = 0;
  int nPoints = wgsPoints.size();
  if ( qFuzzyCompare( qAbs( pn.x() - p1.x() ), 180 ) )   // crosses north pole
  {
    wgsPoints[nPoints-1].setX( p1.x() - 179.999 );
    wgsPoints[1].setX( p1.x() + 179.999 );
    wgsPoints.append( QgsPoint( p1.x() - 179.999, clampLatitude ) );
    wgsPoints[0] = QgsPoint( p1.x() + 179.999, clampLatitude );
    wgsPoints.prepend( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure first point does not overflow in longitude below
    wgsPoints.append( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure last point does not overflow in longitude below
    shift = 3;
  }
  if ( qFuzzyCompare( qAbs( ps.x() - p1.x() ), 180 ) )   // crosses south pole
  {
    wgsPoints[181 + shift].setX( p1.x() - 179.999 );
    wgsPoints[179 + shift].setX( p1.x() + 179.999 );
    wgsPoints[180 + shift] = QgsPoint( p1.x() - 179.999, -clampLatitude );
    wgsPoints.insert( 180 + shift, QgsPoint( p1.x() + 179.999, -clampLatitude ) );
  }
  // Check if area overflows in longitude
  // 0: left-overflow, 1: center, 2: right-overflow
  QList<QgsPointXY> poly[3];
  int current = 1;
  poly[1].append( wgsPoints[0] ); // First point is always in center region
  nPoints = wgsPoints.size();
  for ( int j = 1; j < nPoints; ++j )
  {
    const QgsPointXY& p = wgsPoints[j];
    if ( p.x() > 180. )
    {
      // Right-overflow
      if ( current == 1 )
      {
        poly[1].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
        poly[2].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
        current = 2;
      }
      poly[2].append( QgsPoint( p.x() - 360., p.y() ) );
    }
    else if ( p.x() < -180 )
    {
      // Left-overflow
      if ( current == 1 )
      {
        poly[1].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
        poly[0].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
        current = 0;
      }
      poly[0].append( QgsPoint( p.x() + 360., p.y() ) );
    }
    else
    {
      // No overflow
      if ( current == 0 )
      {
        poly[0].append( QgsPoint( 180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
        poly[1].append( QgsPoint( -180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
        current = 1;
      }
      else if ( current == 2 )
      {
        poly[2].append( QgsPoint( -180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
        poly[1].append( QgsPoint( 180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
        current = 1;
      }
      poly[1].append( p );
    }
  }
  for ( int j = 0; j < 3; ++j )
  {
    if ( !poly[j].isEmpty() )
    {
      QgsPointSequence points;
      for ( int k = 0, n = poly[j].size(); k < n; ++k )
      {
        poly[j][k].setY( qMin( clampLatitude, qMax( -clampLatitude, poly[j][k].y() ) ) );
        try
        {
          points.append( QgsPoint( t2.transform( poly[j][k] ) ) );
        }
        catch ( ... )
        {}
      }
      QgsLineString* ring = new QgsLineString();
      ring->setPoints( points );
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
}

QList<KadasMapItem::NumericAttribute> KadasCircleItem::attributes() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  QList<KadasMapItem::NumericAttribute> attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  attributes.insert(AttrR, NumericAttribute{"r", 0, dMax, 0});
  return attributes;
}

QList<double> KadasCircleItem::attributesFromPosition(const QgsPointXY& pos) const
{
  QList<double> values;
  if(state()->drawStatus == State::Drawing) {
    values.insert(AttrX, state()->centers.last().x());
    values.insert(AttrY, state()->centers.last().y());
    values.insert(AttrR, qSqrt(state()->centers.last().sqrDist(pos)));
  } else {
    values.insert(AttrX, pos.x());
    values.insert(AttrY, pos.y());
    values.insert(AttrR, 0);
  }
  return values;
}

QgsPointXY KadasCircleItem::positionFromAttributes(const QList<double>& values) const
{
  return QgsPointXY(values[AttrX] + values[AttrR], values[AttrY]);
}

