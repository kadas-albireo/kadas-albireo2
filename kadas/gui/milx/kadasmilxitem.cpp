/***************************************************************************
    kdasmilxitem.cpp
    ----------------
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

#include "kadas/gui/milx/kadasmilxitem.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"


QJsonObject KadasMilxItem::State::serialize() const
{
  QJsonArray pts;
  for ( const KadasItemPos &pos : points )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    pts.append( p );
  }
  QJsonArray attrs;
  for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it )
  {
    QJsonArray attr;
    attr.append( it.key() );
    attr.append( it.value() );
    attrs.append( attr );
  }
  QJsonArray attrPts;
  for ( auto it = attributePoints.begin(), itEnd = attributePoints.end(); it != itEnd; ++it )
  {
    QJsonArray attrPt;
    attrPt.append( it.key() );
    QJsonArray pt;
    pt.append( it.value().x() );
    pt.append( it.value().y() );
    attrPt.append( pt );
    attrPts.append( attrPt );
  }
  QJsonArray ctrlPts;
  for ( int ctrlPt : controlPoints )
  {
    ctrlPts.append( ctrlPt );
  }
  QJsonArray offset;
  offset.append( userOffset.x() );
  offset.append( userOffset.y() );

  QJsonArray marginArray;
  marginArray.append( margin.left );
  marginArray.append( margin.top );
  marginArray.append( margin.right );
  marginArray.append( margin.bottom );

  QJsonObject json;
  // TODO json["status"] = static_cast<int>( drawStatus );
  json["points"] = pts;
  json["attributes"] = attrs;
  json["attributePoints"] = attrPts;
  json["controlPoints"] = ctrlPts;
  json["userOffset"] = offset;
  json["pressedPoints"] = pressedPoints;
  json["margin"] = marginArray;
  return json;
}

bool KadasMilxItem::State::deserialize( const QJsonObject &json )
{
  points.clear();
  attributes.clear();
  attributePoints.clear();
  controlPoints.clear();

  // TODO drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  for ( QJsonValue val : json["points"].toArray() )
  {
    QJsonArray pos = val.toArray();
    points.append( KadasItemPos( pos.at( 0 ).toDouble(), pos.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue attrVal : json["attributes"].toArray() )
  {
    QJsonArray attr = attrVal.toArray();
    attributes.insert( static_cast<KadasMilxAttrType>( attr.at( 0 ).toInt() ), attr.at( 1 ).toDouble() );
  }
  for ( QJsonValue attrPtVal : json["attributePoints"].toArray() )
  {
    QJsonArray attrPt = attrPtVal.toArray();
    QJsonArray pt = attrPt.at( 1 ).toArray();
    attributePoints.insert( static_cast<KadasMilxAttrType>( attrPt.at( 0 ).toInt() ), KadasItemPos( pt.at( 0 ).toDouble(), pt.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue val : json["controlPoints"].toArray() )
  {
    controlPoints.append( val.toInt() );
  }
  QJsonArray offset = json["userOffset"].toArray();
  userOffset.setX( offset.at( 0 ).toDouble() );
  userOffset.setY( offset.at( 1 ).toDouble() );
  pressedPoints = json["pressedPoints"].toInt();
  QJsonArray marginArray = json["margin"].toArray();
  if ( marginArray.size() == 4 )
  {
    margin.left = marginArray[0].toInt();
    margin.top = marginArray[1].toInt();
    margin.right = marginArray[2].toInt();
    margin.bottom = marginArray[3].toInt();
  }
  return attributes.size() == attributePoints.size();
}


KadasMilxItem::KadasMilxItem()
  : KadasMapItem( QgsCoordinateReferenceSystem( "EPSG:4326" ) )
{
  clear();
}

void KadasMilxItem::setMssString( const QString &mssString )
{
  mMssString = mssString;
  update();
  emit propertyChanged();
}

void KadasMilxItem::setMilitaryName( const QString &militaryName )
{
  mMilitaryName = militaryName;
  update();
  emit propertyChanged();
}

void KadasMilxItem::setMinNPoints( int minNPoints )
{
  mMinNPoints = minNPoints;
  update();
  emit propertyChanged();
}

void KadasMilxItem::setHasVariablePoints( bool hasVariablePoints )
{
  mHasVariablePoints = hasVariablePoints;
  update();
  emit propertyChanged();
}

void KadasMilxItem::setSymbolType( const QString &symbolType )
{
  mSymbolType = symbolType;
  update();
  emit propertyChanged();
}

KadasItemPos KadasMilxItem::position() const
{
  double x = 0., y = 0.;
  for ( const KadasItemPos &point : constState()->points )
  {
    x += point.x();
    y += point.y();
  }
  int n = std::max( qsizetype( 1 ), constState()->points.size() );
  return KadasItemPos( x / n, y / n );
}

void KadasMilxItem::setPosition( const KadasItemPos &pos )
{
  QgsPointXY prevPos = position();
  double dx = pos.x() - prevPos.x();
  double dy = pos.y() - prevPos.y();
  for ( KadasItemPos &point : state()->points )
  {
    point.setX( point.x() + dx );
    point.setY( point.y() + dy );
  }
  for ( auto it = state()->attributePoints.begin(), itEnd = state()->attributePoints.end(); it != itEnd; ++it )
  {
    it.value().setX( it.value().x() + dx );
    it.value().setY( it.value().y() + dy );
  }

  update();
}

QgsRectangle KadasMilxItem::boundingBox() const
{
  QgsRectangle r;

  for ( const KadasItemPos &p : constState()->points )
  {
    if ( r.isNull() )
    {
      r.set( p.x(), p.y(), p.x(), p.y() );
    }
    else
    {
      r.include( p );
    }
  }
  return r;
}

void KadasMilxItem::render( QgsRenderContext &context ) const
{
  // Legacy MilX layers are converted to QgsAnnotationLayer at project
  // load by KadasItemLayerMigration, so this code path is unreachable.
  Q_UNUSED( context );
}

QList<QgsAnnotationItem *> KadasMilxItem::annotationItems( const QgsCoordinateReferenceSystem &crs ) const
{
  // KadasMilxItem and KadasMilxAnnotationItem both store points in
  // EPSG:4326 (MilX is fixed to WGS84), so no CRS transform is needed
  // even when \a crs is set; just copy the state across verbatim.
  Q_UNUSED( crs )
  if ( mMssString.isEmpty() )
    return {};

  auto *anno = new KadasMilxAnnotationItem();
  anno->setMssString( mMssString );
  anno->setMilitaryName( mMilitaryName );
  anno->setSymbolType( mSymbolType );
  anno->setMinNumPoints( mMinNPoints > 0 ? mMinNPoints : 1 );
  anno->setHasVariablePoints( mHasVariablePoints );

  QList<QgsPointXY> pts;
  pts.reserve( constState()->points.size() );
  for ( const KadasItemPos &p : constState()->points )
    pts.append( QgsPointXY( p.x(), p.y() ) );
  anno->setPoints( pts );
  anno->setControlPoints( constState()->controlPoints );

  // The legacy state stores per-attribute values in 4326 metres / degrees
  // already; carry them across as-is.
  QMap<KadasMilxAttrType, double> attrs;
  for ( auto it = constState()->attributes.cbegin(), itEnd = constState()->attributes.cend(); it != itEnd; ++it )
    attrs.insert( it.key(), it.value() );
  anno->setAttributes( attrs );

  QMap<KadasMilxAttrType, QgsPointXY> attrPts;
  for ( auto it = constState()->attributePoints.cbegin(), itEnd = constState()->attributePoints.cend(); it != itEnd; ++it )
    attrPts.insert( it.key(), QgsPointXY( it.value().x(), it.value().y() ) );
  anno->setAttributePoints( attrPts );

  anno->setUserOffset( constState()->userOffset );
  anno->setDrawStatus( KadasMilxAnnotationItem::DrawStatus::Finished );
  anno->setZIndex( zIndex() ? zIndex() : KadasAnnotationZIndex::Milx );
  return { anno };
}

void KadasMilxItem::setState( const KadasMapItem::State *state )
{
  KadasMapItem::setState( state );
  mIsPointSymbol = !isMultiPoint();
}


bool KadasMilxItem::isMultiPoint() const
{
  return constState()->points.size() > 1 || !constState()->attributes.isEmpty();
}
