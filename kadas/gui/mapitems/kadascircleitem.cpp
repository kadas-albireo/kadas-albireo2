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
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadascircleitem.h>


KADAS_REGISTER_MAP_ITEM( KadasCircleItem, []( const QgsCoordinateReferenceSystem &crs )  { return new KadasCircleItem( crs ); } );

QJsonObject KadasCircleItem::State::serialize() const
{
  QJsonArray c;
  for ( const KadasItemPos &pos : centers )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    c.append( p );
  }
  QJsonArray r;
  for ( double radius : radii )
  {
    r.append( radius );
  }
  QJsonObject json;
  json["status"] = drawStatus;
  json["centers"] = c;
  json["radii"] = r;
  return json;
}

bool KadasCircleItem::State::deserialize( const QJsonObject &json )
{
  centers.clear();
  radii.clear();

  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  for ( QJsonValue val : json["centers"].toArray() )
  {
    QJsonArray pos = val.toArray();
    centers.append( KadasItemPos( pos.at( 0 ).toDouble(), pos.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue val : json["radii"].toArray() )
  {
    radii.append( val.toDouble() );
  }
  return centers.size() == radii.size();
}

KadasCircleItem::KadasCircleItem( const QgsCoordinateReferenceSystem &crs, bool geodesic )
  : KadasGeometryItem( crs )
{
  mGeodesic = geodesic;
  clear();
}

void KadasCircleItem::setGeodesic( bool geodesic )
{
  mGeodesic = geodesic;
  update();
}

KadasItemPos KadasCircleItem::position() const
{
  double x = 0., y = 0.;
  for ( const KadasItemPos &center : constState()->centers )
  {
    x += center.x();
    y += center.y();
  }
  int n = std::max( 1, constState()->centers.length() );
  return KadasItemPos( x / n, y / n );
}

void KadasCircleItem::setPosition( const KadasItemPos &pos )
{
  KadasItemPos prevPos = position();
  double dx = pos.x() - prevPos.x();
  double dy = pos.y() - prevPos.y();
  for ( KadasItemPos &center : state()->centers )
  {
    center.setX( center.x() + dx );
    center.setY( center.y() + dy );
  }
  if ( mGeometry )
  {
    mGeometry->transformVertices( [dx, dy]( const QgsPoint & p ) { return QgsPoint( p.x() + dx, p.y() + dy ); } );
  }
  update();
}

QList<KadasMapItem::Node> KadasCircleItem::nodes( const QgsMapSettings &settings ) const
{
  QgsCoordinateTransform crst( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  QList<Node> points;
  for ( int i = 0, n = constState()->centers.size(); i < n; ++i )
  {
    const KadasItemPos &center = constState()->centers[i];
    points.append( {toMapPos( center, settings )} );
    if ( mGeodesic )
    {
      QgsPointXY wgsCenter = crst.transform( center );
      QgsPointXY wgsRPos = mDa.computeSpheroidProject( wgsCenter, constState()->radii[i], 0.5 * M_PI );
      KadasItemPos itemRPos = KadasItemPos::fromPoint( crst.transform( wgsRPos, QgsCoordinateTransform::ReverseTransform ) );
      points.append( { toMapPos( itemRPos, settings ) } );
    }
    else
    {
      points.append( { toMapPos( KadasItemPos( center.x() + constState()->radii[i], center.y() ), settings ) } );
    }
  }
  return points;
}

bool KadasCircleItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::Drawing;
  state()->centers.append( toItemPos( firstPoint, mapSettings ) );
  state()->radii.append( 0 );
  recomputeDerived();
  return true;
}

bool KadasCircleItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasCircleItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  const KadasItemPos &center = state()->centers.last();
  state()->radii.last() = qSqrt( toItemPos( p, mapSettings ).sqrDist( center ) );
  recomputeDerived();
}

void KadasCircleItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  KadasItemPos center = toItemPos( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
  KadasItemPos rPos = toItemPos( KadasMapPos( values[AttrX] + values[AttrR], values[AttrY] ), mapSettings );
  state()->centers.last() = center;
  state()->radii.last() = qSqrt( center.sqrDist( rPos ) );
  recomputeDerived();
}

bool KadasCircleItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasCircleItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasCircleItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x"} );
  attributes.insert( AttrY, NumericAttribute{"y"} );
  attributes.insert( AttrR, NumericAttribute{"r", NumericAttribute::TypeDistance, 0} );
  return attributes;
}

KadasMapItem::AttribValues KadasCircleItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  if ( constState()->drawStatus == State::Drawing )
  {
    KadasMapPos mapCenter = toMapPos( constState()->centers.last(), mapSettings );
    values.insert( AttrX, mapCenter.x() );
    values.insert( AttrY, mapCenter.y() );
    values.insert( AttrR, qSqrt( mapCenter.sqrDist( pos ) ) );
  }
  else
  {
    values.insert( AttrX, pos.x() );
    values.insert( AttrY, pos.y() );
    values.insert( AttrR, 0 );
  }
  return values;
}

KadasMapPos KadasCircleItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX] + values[AttrR], values[AttrY] );
}

KadasMapItem::EditContext KadasCircleItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  for ( int iPart = 0, nParts = constState()->centers.size(); iPart < nParts; ++iPart )
  {

    KadasMapPos ringPos = toMapPos( KadasItemPos( constState()->centers[iPart].x() + constState()->radii[iPart], constState()->centers[iPart].y() ), mapSettings );
    if ( pos.sqrDist( ringPos ) < pickTolSqr( mapSettings ) )
    {
      AttribDefs attributes;
      attributes.insert( AttrR, NumericAttribute{"r", NumericAttribute::TypeDistance, 0} );
      return EditContext( QgsVertexId( iPart, 0, 1 ), ringPos, attributes );
    }

    KadasMapPos center = toMapPos( constState()->centers[iPart], mapSettings );
    if ( pos.sqrDist( center ) < pickTolSqr( mapSettings ) )
    {
      AttribDefs attributes;
      attributes.insert( AttrX, NumericAttribute{"x"} );
      attributes.insert( AttrY, NumericAttribute{"y"} );
      return EditContext( QgsVertexId( iPart, 0, 0 ), center, attributes );
    }
  }
  if ( intersects( KadasMapRect( pos, pickTol( mapSettings ) ), mapSettings ) )
  {
    KadasMapPos refPos = toMapPos( constState()->centers.front(), mapSettings );
    return EditContext( QgsVertexId(), refPos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasCircleItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part >= 0 && context.vidx.part < state()->centers.size() )
  {
    KadasItemPos itemPos = toItemPos( newPoint, mapSettings );
    if ( context.vidx.vertex == 0 )
    {
      state()->centers[context.vidx.part] = itemPos;
    }
    else if ( context.vidx.vertex == 1 )
    {
      state()->radii[context.vidx.part] = qSqrt( itemPos.sqrDist( state()->centers[context.vidx.part] ) );
    }
    recomputeDerived();
  }
  else
  {
    // Move geometry a whole
    KadasMapPos refMapPos = toMapPos( constState()->centers.front(), mapSettings );
    for ( KadasItemPos &pos : state()->centers )
    {
      KadasMapPos mapPos = toMapPos( pos, mapSettings );
      pos = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
    }
    recomputeDerived();
  }
}

void KadasCircleItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part >= 0 && context.vidx.part < state()->centers.size() )
  {
    if ( context.vidx.vertex == 0 )
    {
      state()->centers[context.vidx.part] = toItemPos( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
    }
    else if ( context.vidx.vertex == 1 )
    {
      KadasMapPos mapPos = toMapPos( state()->centers[context.vidx.part], mapSettings );
      mapPos.setX( mapPos.x() + values[AttrR] );
      KadasItemPos rPos = toItemPos( mapPos, mapSettings );
      state()->radii[context.vidx.part] = qSqrt( state()->centers[context.vidx.part].sqrDist( rPos ) );
    }
    recomputeDerived();
  }
}

KadasMapItem::AttribValues KadasCircleItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  if ( context.vidx.part >= 0 && context.vidx.part < constState()->centers.size() )
  {
    if ( context.vidx.vertex == 0 )
    {
      values.insert( AttrX, pos.x() );
      values.insert( AttrY, pos.y() );
    }
    else if ( context.vidx.vertex == 1 )
    {
      values.insert( AttrR, qSqrt( toMapPos( constState()->centers[context.vidx.part], mapSettings ).sqrDist( pos ) ) );
    }
  }
  return values;
}

KadasMapPos KadasCircleItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  if ( context.vidx.part >= 0 && context.vidx.part < constState()->centers.size() )
  {
    if ( context.vidx.vertex == 0 )
    {
      return KadasMapPos( values[AttrX], values[AttrY] );
    }
    else if ( context.vidx.vertex == 1 )
    {
      KadasMapPos mapCenter = toMapPos( constState()->centers[context.vidx.part], mapSettings );
      return KadasMapPos( mapCenter.x() + values[AttrR], mapCenter.y() );
    }
  }
  return KadasMapPos();
}

void KadasCircleItem::addPartFromGeometry( const QgsAbstractGeometry &geom )
{
  QgsRectangle bbox = geom.boundingBox();
  state()->centers.append( KadasItemPos::fromPoint( bbox.center() ) );
  state()->radii.append( 0.5 * bbox.width() );
  recomputeDerived();
  endPart();
}

const QgsMultiSurface *KadasCircleItem::geometry() const
{
  return static_cast<QgsMultiSurface *>( mGeometry );
}

QgsMultiSurface *KadasCircleItem::geometry()
{
  return static_cast<QgsMultiSurface *>( mGeometry );
}

void KadasCircleItem::measureGeometry()
{
  double totalArea = 0;
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    double radius = state()->radii[i];

    double area = radius * radius * M_PI;

    QStringList measurements;
    measurements.append( formatArea( area, areaBaseUnit() ) );
    measurements.append( tr( "Radius: %1" ).arg( formatLength( radius, distanceBaseUnit() ) ) );
    addMeasurements( QStringList() << measurements, KadasItemPos::fromPoint( state()->centers[i] ) );
    totalArea += area;
  }
  mTotalMeasurement = formatArea( totalArea, areaBaseUnit() );
}

void KadasCircleItem::recomputeDerived()
{
  QgsMultiSurface *multiGeom = new QgsMultiSurface();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    if ( mGeodesic )
    {
      computeGeoCircle( state()->centers[i], state()->radii[i], multiGeom );
    }
    else
    {
      computeCircle( state()->centers[i], state()->radii[i], multiGeom );
    }
  }
  setInternalGeometry( multiGeom );
}

void KadasCircleItem::computeCircle( const KadasItemPos &center, double radius, QgsMultiSurface *multiGeom )
{
  QgsCircularString *string = new QgsCircularString();
  string->setPoints( QgsPointSequence()
                     << QgsPoint( center.x(), center.y() + radius )
                     << QgsPoint( center.x() + radius, center.y() )
                     << QgsPoint( center.x(), center.y() - radius )
                     << QgsPoint( center.x() - radius, center.y() )
                     << QgsPoint( center.x(), center.y() + radius )
                   );
  QgsCompoundCurve *curve = new QgsCompoundCurve();
  curve->addCurve( string );
  QgsCurvePolygon *poly = new QgsCurvePolygon();
  poly->setExteriorRing( curve );
  multiGeom->addGeometry( poly );
}

void KadasCircleItem::computeGeoCircle( const KadasItemPos &center, double radius, QgsMultiSurface *multiGeom )
{
  // 1 deg segmentized circle around center
  QgsCoordinateTransform t1( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326" ), mCrs, QgsProject::instance() );
  QgsPointXY p1 = t1.transform( center );
  double clampLatitude = mCrs.authid() == "EPSG:3857" ? 85 : 90;
  QList<QgsPointXY> wgsPoints;
  for ( int a = 0; a < 360; ++a )
  {
    wgsPoints.append( mDa.computeSpheroidProject( p1, radius, a / 180. * M_PI ) );
  }
  // Check if area would cross north or south pole
  // -> Check if destination point at bearing 0 / 180 with given radius would flip longitude
  // -> If crosses north/south pole, add points at lat 90 resp. -90 between points with max resp. min latitude
  QgsPointXY pn = mDa.computeSpheroidProject( p1, radius, 0 );
  QgsPointXY ps = mDa.computeSpheroidProject( p1, radius, M_PI );
  int shift = 0;
  int nPoints = wgsPoints.size();
  if ( qFuzzyCompare( qAbs( pn.x() - p1.x() ), 180 ) )     // crosses north pole
  {
    wgsPoints[nPoints - 1].setX( p1.x() - 179.999 );
    wgsPoints[1].setX( p1.x() + 179.999 );
    wgsPoints.append( QgsPoint( p1.x() - 179.999, clampLatitude ) );
    wgsPoints[0] = QgsPoint( p1.x() + 179.999, clampLatitude );
    wgsPoints.prepend( QgsPoint( p1.x(), clampLatitude ) );   // Needed to ensure first point does not overflow in longitude below
    wgsPoints.append( QgsPoint( p1.x(), clampLatitude ) );   // Needed to ensure last point does not overflow in longitude below
    shift = 3;
  }
  if ( qFuzzyCompare( qAbs( ps.x() - p1.x() ), 180 ) )     // crosses south pole
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
  poly[1].append( wgsPoints[0] );  // First point is always in center region
  nPoints = wgsPoints.size();
  for ( int j = 1; j < nPoints; ++j )
  {
    const QgsPointXY &p = wgsPoints[j];
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
      QgsLineString *ring = new QgsLineString();
      ring->setPoints( points );
      QgsPolygon *poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
}
