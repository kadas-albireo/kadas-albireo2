/***************************************************************************
    kadasmapitem.cpp
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

#include <qgis/qgsmaplayer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadasmapitem.h>


KadasMapItem::KadasMapItem( const QgsCoordinateReferenceSystem &crs, QObject *parent )
  : QObject( parent ), mCrs( crs )
{
}

KadasMapItem::~KadasMapItem()
{
  emit aboutToBeDestroyed();
  if ( mAssociatedLayer )
  {
    setParent( nullptr );
    QgsProject::instance()->removeMapLayer( mAssociatedLayer->id() );
  }
}

KadasMapItem *KadasMapItem::clone() const
{
  KadasMapItem *item = _clone();
  item->setState( constState()->clone() );
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    prop.write( item, prop.read( this ) );
  }
  return item;
}

QJsonObject KadasMapItem::serialize() const
{
  QJsonObject props;
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    QVariant variant = prop.read( this );
    QJsonValue value = variant.toJsonValue();
    if ( value.isUndefined() )
    {
      // Manually handle conversion, i.e. variant.toJsonValue does not convert enums to int...
      if ( variant.canConvert( QVariant::Int ) )
      {
        value = QJsonValue( variant.toInt() );
      }
      else if ( variant.canConvert( QVariant::Double ) )
      {
        value = QJsonValue( variant.toDouble() );
      }
      else if ( variant.canConvert( QVariant::String ) )
      {
        value = QJsonValue( variant.toString() );
      }
      else
      {
        // Serialize non-convertible types as base64 encoded binary strings
        QByteArray data;
        QDataStream ds( &data, QIODevice::WriteOnly );
        ds << variant;
        value = QJsonValue( QString( data.toBase64() ) );
      }
    }
    props[prop.name()] = value;
  }
  QJsonObject json;
  json["state"] = constState()->serialize();
  json["props"] = props;
  return json;
}

bool KadasMapItem::deserialize( const QJsonObject &json )
{
  QJsonObject props = json["props"].toObject();
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    QJsonValue value = props[prop.name()];
    QVariant variant( prop.type() );
    if ( variant.toJsonValue().isUndefined() && value.type() == QJsonValue::String )
    {
      // Deserialize non-convertible types from base64 encoded binary strings
      QByteArray ba = QByteArray::fromBase64( value.toString().toLocal8Bit() );
      if ( !ba.isNull() )
      {
        QDataStream ds( &ba, QIODevice::ReadOnly );
        ds >> variant;
        prop.write( this, variant );
      }
      else
      {
        prop.write( this, value.toVariant() );
      }
    }
    else
    {
      prop.write( this, value.toVariant() );
    }
  }
  State *state = createEmptyState();
  bool success = state->deserialize( json["state"].toObject() );
  if ( success )
  {
    setState( state );
  }
  delete state;
  return success;
}

void KadasMapItem::associateToLayer( QgsMapLayer *layer )
{
  mAssociatedLayer = layer;
  setParent( layer );
}

void KadasMapItem::setSelected( bool selected )
{
  mSelected = selected;
  update();
}

void KadasMapItem::setZIndex( int zIndex )
{
  mZIndex = zIndex;
  update();
}

void KadasMapItem::setState( const State *state )
{
  mState->assign( state );
  update();
}

void KadasMapItem::clear()
{
  delete mState;
  mState = createEmptyState();
  update();
}

void KadasMapItem::update()
{
  emit changed();
}

KadasMapPos KadasMapItem::toMapPos( const KadasItemPos &itemPos, const QgsMapSettings &settings ) const
{
  QgsPointXY pos = QgsCoordinateTransform( mCrs, settings.destinationCrs(), settings.transformContext() ).transform( itemPos );
  return KadasMapPos( pos.x(), pos.y() );
}

KadasItemPos KadasMapItem::toItemPos( const KadasMapPos &mapPos, const QgsMapSettings &settings ) const
{
  QgsPointXY pos = QgsCoordinateTransform( settings.destinationCrs(), mCrs, settings.transformContext() ).transform( mapPos );
  return KadasItemPos( pos.x(), pos.y() );
}

KadasMapRect KadasMapItem::toMapRect( const KadasItemRect &itemRect, const QgsMapSettings &settings ) const
{
  QgsRectangle rect = QgsCoordinateTransform( mCrs, settings.destinationCrs(), settings.transformContext() ).transform( itemRect );
  return KadasMapRect( rect.xMinimum(), rect.yMinimum(), rect.xMaximum(), rect.yMaximum() );
}

KadasItemRect KadasMapItem::toItemRect( const KadasMapRect &itemRect, const QgsMapSettings &settings ) const
{
  QgsRectangle rect = QgsCoordinateTransform( settings.destinationCrs(), mCrs, settings.transformContext() ).transform( itemRect );
  return KadasItemRect( rect.xMinimum(), rect.yMinimum(), rect.xMaximum(), rect.yMaximum() );
}

double KadasMapItem::pickTolSqr( const QgsMapSettings &settings ) const
{
  return 25 * settings.mapUnitsPerPixel() * settings.mapUnitsPerPixel();
}

double KadasMapItem::pickTol( const QgsMapSettings &settings ) const
{
  return 5 * settings.mapUnitsPerPixel();
}

void KadasMapItem::defaultNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::red, 2 ) );
  painter->setBrush( Qt::white );
  painter->drawRect( QRectF( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize ) );
}

void KadasMapItem::anchorNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::red );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}
