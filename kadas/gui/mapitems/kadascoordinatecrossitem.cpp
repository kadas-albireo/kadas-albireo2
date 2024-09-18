/***************************************************************************
    kadascoordinatecrossitem.cpp
    ----------------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsrendercontext.h>

#include "kadas/gui/mapitems/kadascoordinatecrossitem.h"


QJsonObject KadasCoordinateCrossItem::State::serialize() const
{
  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );;
  json["x"] = pos.x();
  json["y"] = pos.y();
  return json;
}

bool KadasCoordinateCrossItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  pos.setX( json["x"].toDouble() );
  pos.setY( json["y"].toDouble() );
  return true;
}

KadasCoordinateCrossItem::KadasCoordinateCrossItem( const QgsCoordinateReferenceSystem &crs )
  : KadasMapItem( crs.mapUnits() == Qgis::DistanceUnit::Meters ? crs : QgsCoordinateReferenceSystem( "EPSG:3857" ) )
{
  clear();
}

KadasItemRect KadasCoordinateCrossItem::boundingBox() const
{
  return KadasItemRect( constState()->pos.x(), constState()->pos.y(),
                        constState()->pos.x(), constState()->pos.y() );
}

KadasMapItem::Margin KadasCoordinateCrossItem::margin() const
{
  return {sCrossSize, sCrossSize, sCrossSize, sCrossSize};
}

QList<KadasMapItem::Node> KadasCoordinateCrossItem::nodes( const QgsMapSettings &settings ) const
{
  return {{toMapPos( constState()->pos, settings )}};
}

bool KadasCoordinateCrossItem::intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains ) const
{
  double mapCrossSize = sCrossSize * settings.mapUnitsPerPixel();
  KadasMapPos itemMapPos = toMapPos( constState()->pos, settings );
  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );
  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &filterRect );
  bool result = false;
  if ( contains )
  {
    QgsPolygon crossRect;
    QgsLineString *exterior = new QgsLineString();
    exterior->setPoints( QgsPointSequence()
                         << QgsPoint( itemMapPos.x() - mapCrossSize, itemMapPos.y() - mapCrossSize )
                         << QgsPoint( itemMapPos.x() + mapCrossSize, itemMapPos.y() - mapCrossSize )
                         << QgsPoint( itemMapPos.x() + mapCrossSize, itemMapPos.y() + mapCrossSize )
                         << QgsPoint( itemMapPos.x() - mapCrossSize, itemMapPos.y() + mapCrossSize )
                         << QgsPoint( itemMapPos.x() - mapCrossSize, itemMapPos.y() - mapCrossSize ) );
    crossRect.setExteriorRing( exterior );
    result = geomEngine->contains( &crossRect );
  }
  else
  {
    QgsLineString hLine( QVector<QgsPointXY>
    {
      QgsPointXY( itemMapPos.x() - mapCrossSize, itemMapPos.y() ),
      QgsPointXY( itemMapPos.x() + mapCrossSize, itemMapPos.y() )
    } );
    QgsLineString vLine( QVector<QgsPointXY>
    {
      QgsPointXY( itemMapPos.x(), itemMapPos.y() - mapCrossSize ),
      QgsPointXY( itemMapPos.x(), itemMapPos.y() + mapCrossSize )
    } );

    result = geomEngine->intersects( &hLine ) || geomEngine->intersects( &vLine );
  }
  delete geomEngine;
  return result;
}

void KadasCoordinateCrossItem::render( QgsRenderContext &context ) const
{
  double crossSize = sCrossSize * outputDpiScale( context );

  QgsPointXY mapPos = context.coordinateTransform().transform( constState()->pos.x(), constState()->pos.y() );
  QPointF screenPos = context.mapToPixel().transform( mapPos ).toQPointF();
  context.painter()->setPen( QPen( Qt::white, 10 ) );
  context.painter()->drawLine( screenPos.x() - crossSize, screenPos.y(), screenPos.x() + crossSize, screenPos.y() );
  context.painter()->drawLine( screenPos.x(), screenPos.y() - crossSize, screenPos.x(), screenPos.y() + crossSize );
  context.painter()->setPen( QPen( Qt::black, 3 ) );
  context.painter()->drawLine( screenPos.x() - crossSize, screenPos.y(), screenPos.x() + crossSize, screenPos.y() );
  context.painter()->drawLine( screenPos.x(), screenPos.y() - crossSize, screenPos.x(), screenPos.y() + crossSize );

  struct LabelData
  {
    double x, y;
    double mapCoord;
    double angle;
  };
  QList<LabelData> labels =
  {
    {screenPos.x() - crossSize, screenPos.y() - 12, constState()->pos.y(), 0},
    {screenPos.x() - 12, screenPos.y() + crossSize, constState()->pos.x(), -90}
  };

  QFont font = context.painter()->font();
  font.setPixelSize( sFontSize * outputDpiScale( context ) );

  for ( const LabelData &label : labels )
  {
    QPainterPath path;
    path.addText( 0, 0, font, QString::number( label.mapCoord / 1000., 'f', 0 ) );
    context.painter()->save();
    context.painter()->translate( label.x, label.y );
    context.painter()->rotate( label.angle );
    context.painter()->setBrush( Qt::black );
    context.painter()->setPen( QPen( Qt::white, qRound( sFontSize / 3. ) ) );
    context.painter()->drawPath( path );
    context.painter()->setPen( Qt::NoPen );
    context.painter()->drawPath( path );
    context.painter()->restore();
  }
}

QString KadasCoordinateCrossItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  // Not supported in KML
  return "";
}

KadasItemPos KadasCoordinateCrossItem::position() const
{
  return constState()->pos;
}

void KadasCoordinateCrossItem::setPosition( const KadasItemPos &pos )
{
  state()->pos = roundToKilometre( pos );
  update();
}

bool KadasCoordinateCrossItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->pos = roundToKilometre( toItemPos( firstPoint, mapSettings ) );
  update();
  return false;
}

bool KadasCoordinateCrossItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasCoordinateCrossItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  state()->pos = roundToKilometre( toItemPos( p, mapSettings ) );
  update();
}

void KadasCoordinateCrossItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasCoordinateCrossItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasCoordinateCrossItem::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasCoordinateCrossItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x"} );
  attributes.insert( AttrY, NumericAttribute{"y"} );
  return attributes;
}

KadasMapItem::AttribValues KadasCoordinateCrossItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasCoordinateCrossItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasCoordinateCrossItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  KadasMapPos testPos = toMapPos( constState()->pos, mapSettings );
  if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
  {
    return EditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }
  return EditContext();
}

void KadasCoordinateCrossItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.part == 0 )
  {
    state()->pos = roundToKilometre( toItemPos( newPoint, mapSettings ) );
    update();
  }
}

void KadasCoordinateCrossItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

KadasMapItem::AttribValues KadasCoordinateCrossItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasCoordinateCrossItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

KadasItemPos KadasCoordinateCrossItem::roundToKilometre( const KadasItemPos &itemPos ) const
{
  return KadasItemPos( std::round( itemPos.x() / 1000. ) * 1000, std::round( itemPos.y() / 1000. ) * 1000 );
}
