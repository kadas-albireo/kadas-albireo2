/***************************************************************************
    kadasanchoreditem.cpp
    ---------------------
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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadasanchoreditem.h>


QJsonObject KadasAnchoredItem::State::serialize() const
{
  QJsonArray p;
  p.append( pos.x() );
  p.append( pos.y() );
  QJsonArray s;
  s.append( size.width() );
  s.append( size.height() );

  QJsonObject json;
  json["status"] = drawStatus;
  json["pos"] = p;
  json["angle"] = angle;
  json["size"] = s;
  return json;
}

bool KadasAnchoredItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  QJsonArray p = json["pos"].toArray();
  pos = KadasItemPos( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() );
  angle = json["angle"].toDouble();
  QJsonArray s = json["size"].toArray();
  size = QSize( s.at( 0 ).toDouble(), s.at( 1 ).toDouble() );
  return true;
}


KadasAnchoredItem::KadasAnchoredItem( const QgsCoordinateReferenceSystem &crs )
  : KadasMapItem( crs )
{
  mIsPointSymbol = true;
  clear();
}

void KadasAnchoredItem::setAnchorX( double anchorX )
{
  mAnchorX = anchorX;
  update();
}

void KadasAnchoredItem::setAnchorY( double anchorY )
{
  mAnchorY = anchorY;
  update();
}

void KadasAnchoredItem::setPosition( const KadasItemPos &pos )
{
  state()->pos = pos;
  state()->drawStatus = State::DrawStatus::Finished;
  update();
}

void KadasAnchoredItem::setAngle( double angle )
{
  state()->angle = angle;
  update();
}

KadasItemRect KadasAnchoredItem::boundingBox() const
{
  return KadasItemRect( constState()->pos, constState()->pos );
}

QList<KadasMapPos> KadasAnchoredItem::rotatedCornerPoints( double angle, const QgsMapSettings &settings ) const
{
  double dx1 = - mAnchorX * constState()->size.width();
  double dx2 = + ( 1. - mAnchorX ) * constState()->size.width();
  double dy1 = - ( 1. - mAnchorY ) * constState()->size.height();
  double dy2 = + mAnchorY * constState()->size.height();

  KadasMapPos mapPos = toMapPos( position(), settings );
  double x = mapPos.x();
  double y = mapPos.y();
  double mup = settings.mapUnitsPerPixel();
  double cosa = qCos( angle / 180 * M_PI );
  double sina = qSin( angle / 180 * M_PI );
  KadasMapPos p1( x + ( cosa * dx1 - sina * dy1 ) * mup, y + ( sina * dx1 + cosa * dy1 ) * mup );
  KadasMapPos p2( x + ( cosa * dx2 - sina * dy1 ) * mup, y + ( sina * dx2 + cosa * dy1 ) * mup );
  KadasMapPos p3( x + ( cosa * dx2 - sina * dy2 ) * mup, y + ( sina * dx2 + cosa * dy2 ) * mup );
  KadasMapPos p4( x + ( cosa * dx1 - sina * dy2 ) * mup, y + ( sina * dx1 + cosa * dy2 ) * mup );

  return QList<KadasMapPos>() << p1 << p2 << p3 << p4;
}

KadasMapItem::Margin KadasAnchoredItem::margin() const
{
  double left = -mAnchorX * constState()->size.width();
  double top = -mAnchorY * constState()->size.height();
  double right = ( 1. - mAnchorX ) * constState()->size.width();
  double bottom = ( 1. - mAnchorY ) * constState()->size.height();

  double cosa = qCos( constState()->angle / 180 * M_PI );
  double sina = qSin( constState()->angle / 180 * M_PI );
  QPointF p1( ( cosa * left + sina * top ), ( -sina * left + cosa * top ) );
  QPointF p2( ( cosa * right + sina * top ), ( -sina * right + cosa * top ) );
  QPointF p3( ( cosa * right + sina * bottom ), ( -sina * right + cosa * bottom ) );
  QPointF p4( ( cosa * left + sina * bottom ), ( -sina * left + cosa * bottom ) );

  int iLeft = qFloor( qMin( qMin( p1.x(), p2.x() ), qMin( p3.x(), p4.x() ) ) );
  int iRight = qCeil( qMax( qMax( p1.x(), p2.x() ), qMax( p3.x(), p4.x() ) ) );
  int iTop = qFloor( qMin( qMin( p1.y(), p2.y() ), qMin( p3.y(), p4.y() ) ) );
  int iBottom = qCeil( qMax( qMax( p1.y(), p2.y() ), qMax( p3.y(), p4.y() ) ) );
  return Margin{ -iLeft, -iTop, iRight, iBottom };
}

QList<KadasMapItem::Node> KadasAnchoredItem::nodes( const QgsMapSettings &settings ) const
{
  QList<KadasMapPos> points = rotatedCornerPoints( constState()->angle, settings );
  QList<Node> nodes;
  nodes.append( {points[0]} );
  nodes.append( {points[1]} );
  nodes.append( {points[2]} );
  nodes.append( {points[3]} );
  nodes.append( {KadasMapPos( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) ), rotateNodeRenderer} );
  nodes.append( {toMapPos( constState()->pos, settings ), anchorNodeRenderer} );
  return nodes;
}

bool KadasAnchoredItem::intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const
{
  if ( constState()->size.isEmpty() )
  {
    return false;
  }

  QList<KadasMapPos> points = rotatedCornerPoints( constState()->angle, settings );
  QgsPolygon imageRect;
  imageRect.setExteriorRing( new QgsLineString( QgsPointSequence() << QgsPoint( points[0] ) << QgsPoint( points[1] ) << QgsPoint( points[2] ) << QgsPoint( points[3] ) << QgsPoint( points[0] ) ) );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &imageRect );
  bool intersects = geomEngine->intersects( &filterRect );
  delete geomEngine;
  return intersects;
}

bool KadasAnchoredItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::Drawing;
  state()->pos = toItemPos( firstPoint, mapSettings );
  update();
  return false;
}

bool KadasAnchoredItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasAnchoredItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

void KadasAnchoredItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasAnchoredItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasAnchoredItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasAnchoredItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x"} );
  attributes.insert( AttrY, NumericAttribute{"y"} );
  return attributes;
}

KadasMapItem::AttribValues KadasAnchoredItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasAnchoredItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasAnchoredItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  QList<KadasMapPos> points = rotatedCornerPoints( constState()->angle, mapSettings );
  KadasMapPos rotateHandlePos( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) );
  if ( pos.sqrDist( rotateHandlePos ) < pickTolSqr( mapSettings ) )
  {
    AttribDefs attributes;
    attributes.insert( AttrA, NumericAttribute{QString( QChar( 0x03B1 ) )} );
    return EditContext( QgsVertexId( 0, 0, 1 ), rotateHandlePos, attributes );
  }
  if ( intersects( KadasMapRect( pos, pickTol( mapSettings ) ), mapSettings ) )
  {
    return EditContext( QgsVertexId( 0, 0, 0 ), toMapPos( constState()->pos, mapSettings ), drawAttribs() );
  }
  return EditContext();
}

void KadasAnchoredItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.isValid() )
  {
    if ( context.vidx.vertex == 1 )
    {
      KadasMapPos mapPos = toMapPos( state()->pos, mapSettings );
      double dx = newPoint.x() - mapPos.x();
      double dy = newPoint.y() - mapPos.y();
      // Rotate handle is in the middle of the right edge
      QgsPointXY dir( state()->size.width() - mAnchorX * state()->size.width(), 0.5 * state()->size.height() - mAnchorY * state()->size.height() );
      double offset = qAtan2( dir.y(), dir.x() );
      double angle = ( qAtan2( dy, dx ) + offset ) / M_PI * 180.;
      // If less than 5 deg from quarter, snap to quarter
      if ( qAbs( angle - qRound( angle / 90. ) * 90. ) < 5 )
      {
        angle = qRound( angle / 90. ) * 90.;
      }
      state()->angle = angle;
    }
    else
    {
      state()->pos = toItemPos( newPoint, mapSettings );
    }
    update();
  }
}

void KadasAnchoredItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.isValid() )
  {
    if ( context.vidx.vertex == 1 )
    {
      state()->angle = values[AttrA];
    }
    else
    {
      state()->pos = toItemPos( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
    }
    update();
  }
}

KadasMapItem::AttribValues KadasAnchoredItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  if ( context.vidx.vertex == 1 )
  {
    QgsPointXY dir( constState()->size.width() - mAnchorX * constState()->size.width(), 0.5 * constState()->size.height() - mAnchorY * constState()->size.height() );
    double offset = qAtan2( dir.y(), dir.x() );
    KadasMapPos mapPos = toMapPos( constState()->pos, mapSettings );
    double dx = pos.x() - mapPos.x();
    double dy = pos.y() - mapPos.y();
    double angle = ( qAtan2( dy, dx ) + offset ) / M_PI * 180.;
    while ( angle < 0 )
    {
      angle += 360;
    }
    AttribValues values;
    values.insert( AttrA, angle );
    return values;
  }
  else
  {
    return drawAttribsFromPosition( pos, mapSettings );
  }
}

KadasMapPos KadasAnchoredItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  if ( context.vidx.vertex == 1 )
  {
    QList<KadasMapPos> points = rotatedCornerPoints( values[AttrA], mapSettings );
    return KadasMapPos( 0.5 * ( points[1].x() + points[2].x() ), 0.5 * ( points[1].y() + points[2].y() ) );
  }
  else
  {
    return positionFromDrawAttribs( values, mapSettings );
  }
}

void KadasAnchoredItem::rotateNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::red, 2 ) );
  painter->setBrush( Qt::white );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}
