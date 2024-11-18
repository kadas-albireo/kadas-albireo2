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

#include <thread>

#include <QApplication>
#include <QDesktopWidget>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsunittypes.h>

#include "kadas/gui/mapitems/kadasmapitem.h"

#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/mapitems/kadascircularsectoritem.h"
#include "kadas/gui/mapitems/kadascoordinatecrossitem.h"
#include "kadas/gui/mapitems/kadasgpxrouteitem.h"
#include "kadas/gui/mapitems/kadasgpxwaypointitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/mapitems/kadaspictureitem.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadasrectangleitem.h"
#include "kadas/gui/mapitems/kadasselectionrectitem.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"
#include "kadas/gui/mapitems/kadastextitem.h"
#include "kadas/gui/milx/kadasmilxitem.h"

Q_GLOBAL_STATIC(KadasMapItem::Registry, sRegistry)

std::once_flag onceFlagMapItemRegistry;


KadasMapItem::KadasMapItem( const QgsCoordinateReferenceSystem &crs )
  : mCrs( crs )
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
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    prop.write( item, prop.read( this ) );
  }
  item->setState( constState()->clone() );
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
    // TODO: use custom type
    if ( prop.name() == QString( "filePath" ) )
    {
      value = QJsonValue( QgsProject::instance()->writePath( variant.toString() ) );
    }
    else if ( value.isUndefined() )
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
      else if ( prop.type() == QVariant::Pen )
      {
        QPen pen = variant.value<QPen>();
        value = QString( "%1;%2;%3" ).arg( pen.color().name( QColor::HexArgb ) ).arg( pen.width() ).arg( pen.style() );
      }
      else if ( prop.type() == QVariant::Brush )
      {
        QBrush brush = variant.value<QBrush>();
        value = QString( "%1;%2" ).arg( brush.color().name( QColor::HexArgb ) ).arg( brush.style() );
      }
      else if ( variant.canConvert( QVariant::String ) )
      {
        value = QJsonValue( variant.toString() );
      }
      else
      {
        QgsDebugMsgLevel( QString( "Skipping unserializable property: %1" ).arg( prop.name() ) , 2 );
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
    // TODO: use custom type
    if ( prop.name() == QString( "filePath" ) )
    {
      prop.write( this, QVariant::fromValue( QgsProject::instance()->readPath( value.toString() ) ) );
    }
    else if ( prop.type() == QVariant::Pen )
    {
      QStringList penStr = value.toString().split( ";" );
      if ( penStr.size() == 3 )
      {
        QPen pen( QColor( penStr[0] ), penStr[1].toInt(), static_cast<Qt::PenStyle>( penStr[2].toInt() ) );
        prop.write( this, QVariant::fromValue( pen ) );
      }
    }
    else if ( prop.type() == QVariant::Brush )
    {
      QStringList brushStr = value.toString().split( ";" );
      if ( brushStr.size() == 2 )
      {
        QBrush brush( QColor( brushStr[0] ), static_cast<Qt::BrushStyle>( brushStr[1].toInt() ) );
        prop.write( this, QVariant::fromValue( brush ) );
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

QString KadasMapItem::exportName() const
{
  return itemName();
}

void KadasMapItem::associateToLayer( QgsMapLayer *layer )
{
  mAssociatedLayer = layer;
  setParent( layer );
}

void KadasMapItem::setOwnerLayer( KadasItemLayer *layer )
{
  mOwnerLayer = layer;
}

void KadasMapItem::setSelected( bool selected )
{
  mSelected = selected;
  update();
}

void KadasMapItem::setAuthId( const QString &authId )
{
  mCrs = QgsCoordinateReferenceSystem( authId );
  update();
  emit propertyChanged();
}

void KadasMapItem::setZIndex( int zIndex )
{
  mZIndex = zIndex;
  update();
  emit propertyChanged();
}

void KadasMapItem::setTooltip( const QString &tooltip )
{
  mTooltip = tooltip;
  update();
  emit propertyChanged();
}

void KadasMapItem::setVisible( bool visible )
{
  mVisible = visible;
  update();
}

void KadasMapItem::setSymbolScale( double scale )
{
  mSymbolScale = scale;
  update();
  emit propertyChanged();
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

QPair<KadasMapPos, double> KadasMapItem::closestPoint( const KadasMapPos &pos, const QgsMapSettings &settings ) const
{
  KadasMapPos mapPos = toMapPos( position(), settings );
  QgsPointXY itemPosScreen = settings.mapToPixel().transform( mapPos );
  QgsPointXY testPosScreen = settings.mapToPixel().transform( pos );
  double dist = std::sqrt( itemPosScreen.sqrDist( testPosScreen ) );
  return qMakePair( mapPos, dist );
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

bool KadasMapItem::hitTest( const KadasMapPos &pos, const QgsMapSettings &settings ) const
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( settings );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  KadasMapRect filterRect;
  filterRect.setXMinimum( pos.x() - radiusmu );
  filterRect.setXMaximum( pos.x() + radiusmu );
  filterRect.setYMinimum( pos.y() - radiusmu );
  filterRect.setYMaximum( pos.y() + radiusmu );
  return intersects( filterRect, settings );
}

double KadasMapItem::pickTol( const QgsMapSettings &settings ) const
{
  return 5 * settings.mapUnitsPerPixel();
}

void KadasMapItem::cleanupAttachment( const QString &filePath ) const
{
  if ( !mDontCleanupAttachment && !filePath.isEmpty() && QgsProject::instance()->pathResolver().writePath( filePath ).startsWith( "attachment:" ) )
  {
    QgsProject::instance()->removeAttachedFile( filePath );
  }
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

double KadasMapItem::outputDpiScale( const QgsRenderContext &context )
{
  return double( context.painter()->device()->logicalDpiX() ) / qApp->desktop()->logicalDpiX();
}

double KadasMapItem::getTextRenderScale( const QgsRenderContext &context )
{
  // no idea why this works, otherwise text scales up when edited
  // the render context is coming from KadasMapItem when edited while it comes from the QgsMapLayerRenderer otherwise
  double scale = 1.0;
  if ( context.painter()->device()->physicalDpiX() )
    scale = 1.0 / context.painter()->device()->physicalDpiX() * context.painter()->device()->logicalDpiX();

  return scale;
}

KadasMapItem::Registry *KadasMapItem::registry()
{
  std::call_once(onceFlagMapItemRegistry, []() {
    sRegistry->insert( QStringLiteral( "KadasCircleItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasCircleItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasCircularSectorItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasCircularSectorItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasCoordinateCrossItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasCoordinateCrossItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasGpxRouteItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasGpxRouteItem(); });
    sRegistry->insert( QStringLiteral( "KadasGpxWaypointItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasGpxWaypointItem(); });
    sRegistry->insert( QStringLiteral( "KadasLineItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasLineItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasPictureItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasPictureItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasPointItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasPointItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasPolygonItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasPolygonItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasRectangleItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasRectangleItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasSelectionRectItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasSelectionRectItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasSymbolItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasSymbolItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasPinItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasPinItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasTextItem"), [](const QgsCoordinateReferenceSystem& crs) { return new KadasTextItem(crs); });
    sRegistry->insert( QStringLiteral( "KadasMilxItem" ), [](const QgsCoordinateReferenceSystem& crs) { return new KadasMilxItem(); });
  });

  return sRegistry;
};

int KadasMapItem::NumericAttribute::precision( const QgsMapSettings &mapSettings ) const
{
  if ( decimals >= 0 )
  {
    return decimals;
  }
  if ( type == Type::TypeCoordinate )
  {
    return mapSettings.destinationCrs().mapUnits() == Qgis::DistanceUnit::Degrees ? 3 : 0;
  }
  return 0;
}

QString KadasMapItem::NumericAttribute::suffix( const QgsMapSettings &mapSettings ) const
{
  switch ( type )
  {
    case Type::TypeCoordinate:
      return QString();
    case Type::TypeDistance:
      return QgsUnitTypes::toAbbreviatedString( mapSettings.destinationCrs().mapUnits() );
    case Type::TypeAngle:
      return QString( "°" );
    case Type::TypeOther:
      return QString();
  }
  return QString();
}

QDomElement KadasMapItem::writeXml( QDomDocument &document ) const
{
  QDomElement itemEl = document.createElement( "MapItem" );
  itemEl.setAttribute( "name", metaObject()->className() );
  itemEl.setAttribute( "crs", crs().authid() );
  itemEl.setAttribute( "editor", editor() );
  if ( associatedLayer() )
  {
    itemEl.setAttribute( "associatedLayer", associatedLayer()->id() );
  }
  QJsonDocument doc;
  doc.setObject( serialize() );
  itemEl.appendChild( document.createCDATASection( doc.toJson( QJsonDocument::Compact ) ) );
  return itemEl;
}

KadasMapItem *KadasMapItem::fromXml( const QDomElement &element )
{
  QDomElement itemEl = element;
  QString name = itemEl.attribute( "name" );
  QString crs = itemEl.attribute( "crs" );
  QString editor = itemEl.attribute( "editor" );
  QString layerId = itemEl.attribute( "associatedLayer" );
  QJsonDocument data = QJsonDocument::fromJson( itemEl.firstChild().toCDATASection().data().toLocal8Bit() );
  KadasMapItem::RegistryItemFactory factory = KadasMapItem::registry()->value( name );
  if ( factory )
  {
    KadasMapItem *item = factory( QgsCoordinateReferenceSystem( crs ) );
    item->setEditor( editor );
    if ( !layerId.isEmpty() )
    {
      item->associateToLayer( QgsProject::instance()->mapLayer( layerId ) );
    }
    if ( item->deserialize( data.object() ) )
    {
      return item;
    }
    else
    {
      delete item;
      QgsDebugMsgLevel( QString( "Item deserialization failed: %1" ).arg( name ) , 2 );
    }
  }
  else
  {
    QgsDebugMsgLevel( QString( "Unknown item: %1" ).arg( name ) , 2 );
  }
  return nullptr;
}

QMap<QString, QVariant> KadasMapItem::getProps() const
{
  QMap<QString, QVariant> result;
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    result[prop.name()] = prop.read( this );
  }
  return result;
}

void KadasMapItem::setProps( const QMap<QString, QVariant> &props )
{
  blockSignals( true );
  for ( int i = 0, n = metaObject()->propertyCount(); i < n; ++i )
  {
    QMetaProperty prop = metaObject()->property( i );
    auto it = props.find( prop.name() );
    if ( it != props.end() )
    {
      prop.write( this, it.value() );
    }
  }
  blockSignals( false );
  emit changed();
  emit propertyChanged();
}
