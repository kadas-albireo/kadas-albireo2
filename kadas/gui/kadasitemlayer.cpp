/***************************************************************************
    kadasitemlayer.cpp
    ------------------
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

#include "kadasitemlayer.h"

#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadasmapitem.h>


class KadasItemLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasItemLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id() )
      , mLayer( layer )
      , mRendererContext( rendererContext )
    {}
    bool render() override
    {
      QList<KadasMapItem *> items;
      for ( ItemId id : mLayer->mItemOrder )
      {
        items.append( mLayer->mItems[id] );
      }
      qStableSort( items.begin(), items.end(), []( KadasMapItem * a, KadasMapItem * b ) { return a->zIndex() < b->zIndex(); } );
      bool omitSinglePoint = mRendererContext.customRenderFlags().contains( "globe" );
      for ( const KadasMapItem *item : items )
      {
        if ( item && ( !omitSinglePoint || !item->isPointSymbol() ) )
        {
          mRendererContext.painter()->save();
          mRendererContext.painter()->setOpacity( mLayer->opacity() / 100. );
          mRendererContext.setCoordinateTransform( QgsCoordinateTransform( item->crs(), mRendererContext.coordinateTransform().destinationCrs(), mRendererContext.transformContext() ) );
          item->render( mRendererContext );
          mRendererContext.painter()->restore();
        }
      }
      return true;
    }

  private:
    KadasItemLayer *mLayer;
    QgsRenderContext &mRendererContext;
};


KadasItemLayer::KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs )
  : KadasPluginLayer( layerType(), name )
{
  setCrs( crs );
  mValid = true;
}

KadasItemLayer::KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs, const QString &layerType )
  : KadasPluginLayer( layerType, name )
{
  setCrs( crs );
  mValid = true;
}

KadasItemLayer::~KadasItemLayer()
{
  qDeleteAll( mItems );
}

void KadasItemLayer::addItem( KadasMapItem *item )
{
  ItemId id = ITEM_ID_NULL;
  if ( !mFreeIds.isEmpty() )
  {
    id = mFreeIds.takeLast();
  }
  else
  {
    id = ++mIdCounter;
  }
  mItems.insert( id, item );
  mItemOrder.append( id );
  QgsCoordinateTransform trans( item->crs(), crs(), mTransformContext );
  mItemBounds.insert( id, trans.transformBoundingBox( item->boundingBox() ) );
  emit itemAdded( id );
}

KadasMapItem *KadasItemLayer::takeItem( const ItemId &itemId )
{
  KadasMapItem *item = mItems.take( itemId );
  if ( item )
  {
    mFreeIds.append( itemId );
    mItemBounds.remove( itemId );
    mItemOrder.removeOne( itemId );
    emit itemRemoved( itemId );
  }
  return item;
}

KadasItemLayer *KadasItemLayer::clone() const
{
  KadasItemLayer *layer = new KadasItemLayer( name(), crs() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    layer->mItems.insert( it.key(), it.value()->clone() );
  }
  return layer;
}

QgsMapLayerRenderer *KadasItemLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle KadasItemLayer::extent() const
{
  QgsRectangle rect;
  for ( const QgsRectangle &itemBBox : mItemBounds.values() )
  {
    if ( rect.isNull() )
    {
      rect = itemBBox;
    }
    else
    {
      rect.combineExtentWith( itemBBox );
    }
  }
  return rect;
}


bool KadasItemLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  qDeleteAll( mItems );
  mItems.clear();
  mIdCounter = 0;
  mFreeIds.clear();

  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  QDomNodeList itemEls = layerEl.elementsByTagName( "MapItem" );
  for ( int i = 0, n = itemEls.size(); i < n; ++i )
  {
    QDomElement itemEl = itemEls.at( i ).toElement();
    QString name = itemEl.attribute( "name" );
    QString crs = itemEl.attribute( "crs" );
    QString editor = itemEl.attribute( "editor" );
    QJsonDocument data = QJsonDocument::fromJson( itemEl.firstChild().toCDATASection().data().toLocal8Bit() );
    KadasMapItem::RegistryItemFactory factory = KadasMapItem::registry()->value( name );
    if ( factory )
    {
      KadasMapItem *item = factory( QgsCoordinateReferenceSystem( crs ) );
      item->setEditor( editor );
      if ( item->deserialize( data.object() ) )
      {
        mItems.insert( ++mIdCounter, item );
      }
      else
      {
        QgsDebugMsg( QString( "Item deserialization failed: %1" ).arg( i ) );
      }
    }
    else
    {
      QgsDebugMsg( QString( "Unknown item: %1" ).arg( i ) );
    }
  }
  return true;
}

bool KadasItemLayer::writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    QDomElement itemEl = document.createElement( "MapItem" );
    itemEl.setAttribute( "name", it.value()->metaObject()->className() );
    itemEl.setAttribute( "crs", it.value()->crs().authid() );
    itemEl.setAttribute( "editor", it.value()->editor() );
    QJsonDocument doc;
    doc.setObject( it.value()->serialize() );
    itemEl.appendChild( document.createCDATASection( doc.toJson( QJsonDocument::Compact ) ) );
    layerEl.appendChild( itemEl );
  }
  return true;
}

KadasItemLayer::ItemId KadasItemLayer::pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const
{
  KadasMapRect rect( pickRect.xMinimum(), pickRect.yMinimum(), pickRect.xMaximum(), pickRect.yMaximum() );
  for ( auto it = mItemOrder.rbegin(), itEnd = mItemOrder.rend(); it != itEnd; ++it )
  {
    KadasMapItem *item = mItems[*it];
    if ( item->intersects( rect, mapSettings ) )
    {
      return *it;
    }
  }
  return ITEM_ID_NULL;
}

KadasItemLayer::ItemId KadasItemLayer::pickItem( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) const
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( mapSettings );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );
  return pickItem( filterRect, mapSettings );
}

QPair<QgsPointXY, double> KadasItemLayer::snapToVertex( const QgsPointXY &mapPos, const QgsMapSettings &settings, double tolPixels ) const
{
  QgsCoordinateTransform crst( crs(), settings.destinationCrs(), mTransformContext );
  QgsPointXY layerPos = crst.transform( mapPos );
  double minDist = std::numeric_limits<double>::max();
  QgsPointXY minPos;
  for ( auto it = mItemBounds.begin(), itEnd = mItemBounds.end(); it != itEnd; ++it )
  {
    QgsRectangle bbox = crst.transformBoundingBox( it.value() );
    bbox.grow( settings.mapUnitsPerPixel() * tolPixels );
    if ( bbox.contains( mapPos ) )
    {
      QgsCoordinateTransform crst( settings.destinationCrs(), mItems[it.key()]->crs(), transformContext() );
      QPair<KadasMapPos, double> result = mItems[it.key()]->closestPoint( KadasMapPos::fromPoint( mapPos ), settings );
      if ( result.second < minDist && result.second < tolPixels )
      {
        minDist = result.second;
        minPos = result.first;
      }
    }
  }
  return qMakePair( minPos, minDist );
}
QString KadasItemLayer::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Folder>" << "\n";
  outStream << "<name>" << name() << "</name>" << "\n";
  for ( const KadasMapItem *item : mItems )
  {
    outStream << item->asKml( context, kmzZip );
  }
  outStream << "</Folder>" << "\n";
  outStream.flush();

  return outString;
}


KadasItemLayer *KadasItemLayerRegistry::getOrCreateItemLayer( StandardLayer layer )
{
  KadasItemLayer *itemLayer = getItemLayer( standardLayerNames()[layer] );
  if ( !itemLayer && standardLayerNames().contains( layer ) )
  {
    itemLayer = new KadasItemLayer( standardLayerNames()[layer], QgsCoordinateReferenceSystem( "EPSG:3857" ) );
    layerIdMap()[standardLayerNames()[layer]] = itemLayer->id();
    QgsProject::instance()->addMapLayer( itemLayer );
  }
  return itemLayer;
}

KadasItemLayer *KadasItemLayerRegistry::getOrCreateItemLayer( const QString &layerName )
{
  KadasItemLayer *itemLayer = getItemLayer( layerName );
  if ( !itemLayer )
  {
    itemLayer = new KadasItemLayer( layerName, QgsCoordinateReferenceSystem( "EPSG:3857" ) );
    layerIdMap()[layerName] = itemLayer->id();
    QgsProject::instance()->addMapLayer( itemLayer );
  }
  return itemLayer;
}

KadasItemLayer *KadasItemLayerRegistry::getItemLayer( const QString &layerName )
{
  if ( layerIdMap().contains( layerName ) )
  {
    return qobject_cast<KadasItemLayer *> ( QgsProject::instance()->mapLayer( layerIdMap()[layerName] ) );
  }
  return nullptr;
}

const QMap<KadasItemLayerRegistry::StandardLayer, QString> &KadasItemLayerRegistry::standardLayerNames()
{
  static QMap<StandardLayer, QString> names =
  {
    {RedliningLayer, tr( "Redlining" )},
    {SymbolsLayer, tr( "Symbols" )},
    {PicturesLayer, tr( "Pictures" )},
    {PinsLayer, tr( "Pins" )},
    {RoutesLayer, tr( "Routes" )}
  };
  return names;
}

QMap<QString, QString> &KadasItemLayerRegistry::layerIdMap()
{
  static QMap<QString, QString> map;
  return map;
}
