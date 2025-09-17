/***************************************************************************
    kadascircularsectoritem.cpp
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

#include <QJsonArray>

#include <qgis/qgsgeometry.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultisurface.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>

#include "kadas/gui/mapitems/kadascircularsectoritem.h"


static double toGeoAngle( double arad )
{
  double ageo = -arad / M_PI * 180 + 90.;
  while ( ageo < 0 )
  {
    ageo += 360.;
  }
  while ( ageo >= 360 )
  {
    ageo -= 360.;
  }
  return ageo;
}

static double toRadAngle( double ageo )
{
  double arad = -( ageo - 90. ) / 180. * M_PI;
  while ( arad < 0 )
  {
    arad += 2 * M_PI;
  }
  while ( arad >= 2 * M_PI )
  {
    arad -= 2 * M_PI;
  }
  return arad;
}

QJsonObject KadasCircularSectorItem::State::serialize() const
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
  QJsonArray a1;
  for ( double startAngle : startAngles )
  {
    a1.append( startAngle );
  }
  QJsonArray a2;
  for ( double stopAngle : stopAngles )
  {
    a2.append( stopAngle );
  }
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  json["centers"] = c;
  json["radii"] = r;
  json["startAngles"] = a1;
  json["stopAngles"] = a2;
  json["sectorStatus"] = static_cast<int>( sectorStatus );
  return json;
}

bool KadasCircularSectorItem::State::deserialize( const QJsonObject &json )
{
  centers.clear();
  radii.clear();
  startAngles.clear();
  stopAngles.clear();

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
  for ( QJsonValue val : json["startAngles"].toArray() )
  {
    startAngles.append( val.toDouble() );
  }
  for ( QJsonValue val : json["stopAngles"].toArray() )
  {
    stopAngles.append( val.toDouble() );
  }
  sectorStatus = static_cast<SectorStatus>( json["sectorStatus"].toInt() );

  return centers.size() == radii.size() && centers.size() == startAngles.size() && centers.size() == stopAngles.size();
}

KadasCircularSectorItem::KadasCircularSectorItem()
  : KadasGeometryItem()
{
  clear();
}

KadasItemPos KadasCircularSectorItem::position() const
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

void KadasCircularSectorItem::setPosition( const KadasItemPos &pos )
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
    mGeometry->transformVertices( [dx, dy]( const QgsPoint &p ) { return QgsPoint( p.x() + dx, p.y() + dy ); } );
  }
  update();
}

QList<KadasMapItem::Node> KadasCircularSectorItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> points;
  for ( int i = 0, n = constState()->centers.size(); i < n; ++i )
  {
    points.append( { toMapPos( constState()->centers[i], settings ) } );
  }
  return points;
}

bool KadasCircularSectorItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->sectorStatus = State::SectorStatus::HaveCenter;
  state()->centers.append( toItemPos( firstPoint, mapSettings ) );
  state()->radii.append( 0 );
  state()->startAngles.append( 0 );
  state()->stopAngles.append( 2 * M_PI );
  recomputeDerived();
  return true;
}

bool KadasCircularSectorItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  KadasItemPos center = toItemPos( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
  KadasItemPos rPos = toItemPos( KadasMapPos( values[AttrX] + values[AttrR], values[AttrY] ), mapSettings );
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->sectorStatus = values[AttrR] > 0 ? State::SectorStatus::HaveRadius : State::SectorStatus::HaveCenter;
  state()->centers.append( center );
  state()->radii.append( std::sqrt( center.sqrDist( rPos ) ) );
  state()->startAngles.append( toRadAngle( values[AttrA1] ) );
  state()->stopAngles.append( toRadAngle( values[AttrA2] ) );
  if ( state()->stopAngles.last() <= state()->startAngles.last() )
  {
    state()->stopAngles.last() += 2 * M_PI;
  }
  recomputeDerived();
  return true;
}

void KadasCircularSectorItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  if ( state()->sectorStatus == State::SectorStatus::HaveCenter )
  {
    KadasItemPos rPos = toItemPos( p, mapSettings );
    state()->radii.back() = std::sqrt( rPos.sqrDist( state()->centers.back() ) );
    state()->startAngles.back() = std::atan2( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
    if ( state()->startAngles.back() < 0 )
    {
      state()->startAngles.back() += 2 * M_PI;
    }
    state()->stopAngles.back() = state()->startAngles.back() + 2 * M_PI;
  }
  else if ( state()->sectorStatus == State::SectorStatus::HaveRadius )
  {
    state()->stopAngles.back() = std::atan2( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
    while ( state()->stopAngles.back() <= state()->startAngles.back() )
    {
      state()->stopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    const KadasItemPos &center = state()->centers.back();
    const double &radius = state()->radii.back();
    const double &startAngle = state()->startAngles.back();
    const double &stopAngle = state()->stopAngles.back();
    KadasItemPos pStart( center.x() + radius * std::cos( startAngle ), center.y() + radius * std::sin( startAngle ) );
    KadasItemPos pEnd( center.x() + radius * std::cos( stopAngle ), center.y() + radius * std::sin( stopAngle ) );
    KadasMapPos mapPStart = toMapPos( pStart, mapSettings );
    KadasMapPos mapPEnd = toMapPos( pEnd, mapSettings );
    if ( mapPStart.sqrDist( mapPEnd ) < pickTolSqr( mapSettings ) )
    {
      state()->stopAngles.back() = state()->startAngles.back() + 2 * M_PI;
    }
  }
  recomputeDerived();
}

void KadasCircularSectorItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  KadasItemPos center = toItemPos( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
  KadasItemPos rPos = toItemPos( KadasMapPos( values[AttrX] + values[AttrR], values[AttrY] ), mapSettings );
  state()->sectorStatus = values[AttrR] > 0 ? State::SectorStatus::HaveRadius : State::SectorStatus::HaveCenter;
  state()->centers.last() = center;
  state()->radii.last() = std::sqrt( center.sqrDist( rPos ) );
  state()->startAngles.last() = toRadAngle( values[AttrA1] );
  state()->stopAngles.last() = toRadAngle( values[AttrA2] );
  if ( state()->stopAngles.last() <= state()->startAngles.last() )
  {
    state()->stopAngles.last() += 2 * M_PI;
  }
  recomputeDerived();
}

bool KadasCircularSectorItem::continuePart( const QgsMapSettings &mapSettings )
{
  if ( state()->sectorStatus == State::SectorStatus::HaveCenter )
  {
    state()->sectorStatus = State::SectorStatus::HaveRadius;
    return true;
  }
  return false;
}

void KadasCircularSectorItem::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasCircularSectorItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  attributes.insert( AttrR, NumericAttribute { "r", NumericAttribute::Type::TypeDistance, 0 } );
  attributes.insert( AttrA1, NumericAttribute { QString( QChar( 0x03B1 ) ) + "1", NumericAttribute::Type::TypeAngle, 0 } );
  attributes.insert( AttrA2, NumericAttribute { QString( QChar( 0x03B1 ) ) + "2", NumericAttribute::Type::TypeAngle, 0 } );
  return attributes;
}

KadasMapItem::AttribValues KadasCircularSectorItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues attributes;
  if ( constState()->drawStatus == State::DrawStatus::Empty )
  {
    attributes.insert( AttrX, pos.x() );
    attributes.insert( AttrY, pos.y() );
    attributes.insert( AttrR, 0 );
    attributes.insert( AttrA1, 0 );
    attributes.insert( AttrA2, 0 );
  }
  else
  {
    const KadasItemPos &center = constState()->centers.last();
    KadasMapPos mapCenter = toMapPos( center, mapSettings );
    KadasMapPos mapRPos = toMapPos( KadasItemPos( center.x() + constState()->radii.last(), center.y() ), mapSettings );
    attributes.insert( AttrX, mapCenter.x() );
    attributes.insert( AttrY, mapCenter.y() );
    attributes.insert( AttrR, std::sqrt( mapCenter.sqrDist( mapRPos ) ) );
    if ( constState()->sectorStatus == State::SectorStatus::HaveRadius )
    {
      attributes.insert( AttrA1, toGeoAngle( constState()->startAngles.last() ) );
      attributes.insert( AttrA2, toGeoAngle( constState()->stopAngles.last() ) );
    }
    else
    {
      attributes.insert( AttrA1, 0 );
      attributes.insert( AttrA2, 0 );
    }
  }
  return attributes;
}

KadasMapPos KadasCircularSectorItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasCircularSectorItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  // Not yet implemented
  return EditContext();
}

void KadasCircularSectorItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  // Not yet implemented
}

void KadasCircularSectorItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Not yet implemented
}

KadasMapItem::AttribValues KadasCircularSectorItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  // Not yet implemented
  return values;
}

KadasMapPos KadasCircularSectorItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  // Not yet implemented
  return KadasMapPos();
}

void KadasCircularSectorItem::addPartFromGeometry( const QgsAbstractGeometry &geom )
{
  // Not yet implemented
}

const QgsMultiSurface *KadasCircularSectorItem::geometry() const
{
  return static_cast<QgsMultiSurface *>( mGeometry );
}

QgsMultiSurface *KadasCircularSectorItem::geometry()
{
  return static_cast<QgsMultiSurface *>( mGeometry );
}

void KadasCircularSectorItem::measureGeometry()
{
  // Not yet implemented
}

void KadasCircularSectorItem::recomputeDerived()
{
  QgsMultiSurface *multiGeom = new QgsMultiSurface();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    const KadasItemPos &center = state()->centers[i];
    double radius = state()->radii[i];
    double startAngle = state()->startAngles[i];
    double stopAngle = state()->stopAngles[i];
    QgsCompoundCurve *exterior = new QgsCompoundCurve();
    if ( stopAngle - startAngle < 2 * M_PI - std::numeric_limits<float>::epsilon() )
    {
      double alphaMid = 0.5 * ( startAngle + 2 * M_PI + stopAngle );
      QgsPoint pStart = QgsPoint( center.x() + radius * std::cos( startAngle ), center.y() + radius * std::sin( startAngle ) );
      QgsPoint pMid = QgsPoint( center.x() + radius * std::cos( alphaMid ), center.y() + radius * std::sin( alphaMid ) );
      QgsPoint pEnd = QgsPoint( center.x() + radius * std::cos( stopAngle ), center.y() + radius * std::sin( stopAngle ) );
      exterior->addCurve( new QgsCircularString( pStart, pMid, pEnd ) );

      exterior->addCurve( new QgsLineString( QgsPointSequence() << pEnd << QgsPoint( center ) << pStart ) );
    }
    else
    {
      QgsCircularString *arc = new QgsCircularString();
      arc->setPoints( QgsPointSequence() << QgsPoint( center.x(), center.y() + radius ) << QgsPoint( center.x() + radius, center.y() ) << QgsPoint( center.x(), center.y() - radius ) << QgsPoint( center.x() - radius, center.y() ) << QgsPoint( center.x(), center.y() + radius ) );
      exterior->addCurve( arc );
    }
    QgsPolygon *poly = new QgsPolygon;
    poly->setExteriorRing( exterior );
    multiGeom->addGeometry( poly );
  }
  setInternalGeometry( multiGeom );
}
