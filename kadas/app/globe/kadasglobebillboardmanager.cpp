/***************************************************************************
    kadasglobebillboardmanager.cpp
    ----------------------------
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

#include <osgEarth/Version>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/kadasmapcanvasitemmanager.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include <globe/kadasglobebillboardmanager.h>


KadasGlobeBillboardManager::KadasGlobeBillboardManager( QObject *parent )
  : QObject( parent )
{
}

void KadasGlobeBillboardManager::init( osg::ref_ptr<osgEarth::MapNode> mapNode, const QStringList &visibleLayerIds )
{
  mMapNode = mapNode;
  mGroup = new osg::Group;
  mMapNode->addChild( mGroup );
  updateLayers( visibleLayerIds );
  for ( KadasMapItem *item : KadasMapCanvasItemManager::items() )
  {
    if ( !item->associatedLayer() && !item->ownerLayer() )
    {
      addCanvasBillboard( item );
    }
  }
  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemAdded, this, &KadasGlobeBillboardManager::addCanvasBillboard );
  connect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemWillBeRemoved, this, &KadasGlobeBillboardManager::removeCanvasBillboard );
}

void KadasGlobeBillboardManager::reset()
{
  disconnect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemAdded, this, &KadasGlobeBillboardManager::addCanvasBillboard );
  disconnect( KadasMapCanvasItemManager::instance(), &KadasMapCanvasItemManager::itemWillBeRemoved, this, &KadasGlobeBillboardManager::removeCanvasBillboard );

  delete mSignalScope;
  mSignalScope = nullptr;
  if ( mGroup )
  {
    mMapNode->removeChild( mGroup );
  }
  mGroup = nullptr;
  mMapNode = nullptr;
  mRegistry.clear();
}

void KadasGlobeBillboardManager::updateLayers( const QStringList &layerIds )
{
  delete mSignalScope;
  mSignalScope = new QObject( this );

  mCurrentLayers = layerIds;

  // Remove billboards for removed layers
  QList<QString> registeredBillboards = mRegistry.keys();
  QSet<QString> removedLayerIds = QSet<QString>( registeredBillboards.begin(), registeredBillboards.end() ).subtract( QSet<QString>( layerIds.begin(), layerIds.end() ) );
  for ( const QString &layerId : removedLayerIds )
  {
    if ( mRegistry.contains( layerId ) )
    {
      for ( auto placeNode : mRegistry[layerId].values() )
      {
        mGroup->removeChild( placeNode );
      }
      mRegistry.remove( layerId );
    }
  }

  // Remove billboards for items associated to an removed layer
  for ( const KadasMapItem *item : mCanvasItemsRegistry.keys() )
  {
    if (
      ( item->associatedLayer() && !mCurrentLayers.contains( item->associatedLayer()->id() ) ) ||
      ( item->ownerLayer() && !mCurrentLayers.contains( item->ownerLayer()->id() ) )
    )
    {
      removeCanvasBillboard( item );
    }
  }

  // Add billboards for new layers
  QSet<QString> addedLayerIds = QSet<QString>( layerIds.begin(), layerIds.end() ).subtract( QSet<QString>( registeredBillboards.begin(), registeredBillboards.end() ) );
  for ( const QString &layerId : addedLayerIds )
  {
    KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( QgsProject::instance()->mapLayer( layerId ) );
    if ( !layer )
    {
      continue;
    }
    for ( KadasItemLayer::ItemId itemId : layer->items().keys() )
    {
      addLayerBillboard( layer->id(), itemId );
    }
  }

  // Add billboards for items associated to an added layer
  for ( const KadasMapItem *item : KadasMapCanvasItemManager::items() )
  {
    if ( !mCanvasItemsRegistry.contains( item ) )
    {
      if (
        ( item->associatedLayer() && mCurrentLayers.contains( item->associatedLayer()->id() ) ) ||
        ( item->ownerLayer() && mCurrentLayers.contains( item->ownerLayer()->id() ) )
      )
      {
        addCanvasBillboard( item );
      }
    }
  }

  // Connect signals
  for ( const QString &layerId : layerIds )
  {
    KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( QgsProject::instance()->mapLayer( layerId ) );
    if ( !layer )
    {
      continue;
    }
    connect( layer, &KadasItemLayer::itemAdded, mSignalScope, [layerId, this]( KadasItemLayer::ItemId id ) { addLayerBillboard( layerId, id ); } );
    connect( layer, &KadasItemLayer::itemRemoved, mSignalScope, [layerId, this]( KadasItemLayer::ItemId id ) { removeLayerBillboard( layerId, id ); } );
  }
}

void KadasGlobeBillboardManager::addLayerBillboard( const QString &layerId, KadasItemLayer::ItemId itemId )
{
  KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( QgsProject::instance()->mapLayer( layerId ) );
  if ( !layer )
  {
    return;
  }

  KadasMapItem *item = layer->items()[itemId];

  if ( !item || !item->isPointSymbol() )
  {
    return;
  }

  osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode = createBillboard( item );

  mRegistry[layerId][itemId] = placeNode;
  mGroup->addChild( placeNode );
}

void KadasGlobeBillboardManager::removeLayerBillboard( const QString &layerId, KadasItemLayer::ItemId itemId )
{
  if ( mRegistry.contains( layerId ) && mRegistry[layerId].contains( itemId ) )
  {
    mGroup->removeChild( mRegistry[layerId][itemId] );
    mRegistry[layerId].remove( itemId );
    if ( mRegistry[layerId].isEmpty() )
    {
      mRegistry.remove( layerId );
    }
  }
}

void KadasGlobeBillboardManager::addCanvasBillboard( const KadasMapItem *item )
{
  if ( item && item->isPointSymbol() )
  {
    if (
      ( item->associatedLayer() && !mCurrentLayers.contains( item->associatedLayer()->id() ) ) ||
      ( item->ownerLayer() && !mCurrentLayers.contains( item->ownerLayer()->id() ) )
    )
    {
      return;
    }
    osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode = createBillboard( item );
    connect( item, &KadasMapItem::changed, this, &KadasGlobeBillboardManager::updateCanvasBillboard );
    mCanvasItemsRegistry.insert( item, placeNode );
    mGroup->addChild( placeNode );
  }
}

void KadasGlobeBillboardManager::removeCanvasBillboard( const KadasMapItem *item )
{
  if ( mCanvasItemsRegistry.contains( item ) )
  {
    mGroup->removeChild( mCanvasItemsRegistry[item] );
    mCanvasItemsRegistry.remove( item );
  }
}

void KadasGlobeBillboardManager::updateCanvasBillboard()
{
  KadasMapItem *item = qobject_cast<KadasMapItem *>( QObject::sender() );
  if ( !item && !mCanvasItemsRegistry.contains( item ) )
  {
    return;
  }
  osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode = mCanvasItemsRegistry.value( item );
  if ( !placeNode )
  {
    return;
  }

  QgsCoordinateReferenceSystem crs84( "EPSG:4326" );

  QgsCoordinateTransform crst( item->crs(), crs84, QgsProject::instance() );
  QgsPointXY pos = crst.transform( item->position() );

  osgEarth::GeoPoint geop( osgEarth::SpatialReference::get( "wgs84" ), pos.x(), pos.y(), 0, osgEarth::ALTMODE_RELATIVE );
  placeNode->setPosition( geop );
}

osg::ref_ptr<osgEarth::Annotation::PlaceNode> KadasGlobeBillboardManager::createBillboard( const KadasMapItem *item )
{
  QgsCoordinateReferenceSystem crs84( "EPSG:4326" );

  QgsCoordinateTransform crst( item->crs(), crs84, QgsProject::instance() );
  QgsPointXY pos = crst.transform( item->position() );

  osgEarth::GeoPoint geop( osgEarth::SpatialReference::get( "wgs84" ), pos.x(), pos.y(), 0, osgEarth::ALTMODE_RELATIVE );
  QImage image = item->symbolImage();
  int offset = item->symbolAnchor().x() * image.width() - image.width() / 2;
  if ( offset != 0 )
  {
    QImage newimage( image.width() + 2 * qAbs( offset ), image.height(), image.format() );
    newimage.fill( Qt::transparent );
    QPainter p( &newimage );
    p.drawImage( offset > 0 ? 0 : 2 * -offset, 0, image );
    image = newimage;
  }

  unsigned char *imgbuf = new unsigned char[image.bytesPerLine() * image.height()];
  std::memcpy( imgbuf, image.bits(), image.bytesPerLine() * image.height() );
  osg::Image *osgImage = new osg::Image;
  osgImage->setImage( image.width(), image.height(), 1, 4, // width, height, depth, internal_format
                      GL_BGRA, GL_UNSIGNED_BYTE, imgbuf, osg::Image::USE_NEW_DELETE );
  osgImage->flipVertical();
  osgEarth::Style pin;
  pin.getOrCreateSymbol<osgEarth::IconSymbol>()->setImage( osgImage );

#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
  osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode = new osgEarth::Annotation::PlaceNode( mMapNode, geop, "", pin );
#else
  osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode = new osgEarth::Annotation::PlaceNode( geop, "", pin );
#endif
  placeNode->setOcclusionCulling( true );

  return placeNode;
}
