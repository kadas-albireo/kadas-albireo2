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

#include <qgis/qgsproject.h>

#include <kadas/gui/mapitems/kadasmapitem.h>
#include <kadas/app/globe/kadasglobebillboardmanager.h>

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
}

void KadasGlobeBillboardManager::reset()
{
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


  // Remove billboards for removed layers
  QSet<QString> removedLayerIds = mRegistry.keys().toSet().subtract( layerIds.toSet() );
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

  // Add billboards for new layers
  QSet<QString> addedLayerIds = layerIds.toSet().subtract( mRegistry.keys().toSet() );
  for ( const QString &layerId : addedLayerIds )
  {
    KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( QgsProject::instance()->mapLayer( layerId ) );
    if ( !layer )
    {
      continue;
    }
    for ( KadasItemLayer::ItemId itemId : layer->items().keys() )
    {
      addBillboard( layer->id(), itemId );
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
    connect( layer, &KadasItemLayer::itemAdded, mSignalScope, [layerId, this]( KadasItemLayer::ItemId id ) { addBillboard( layerId, id ); } );
    connect( layer, &KadasItemLayer::itemRemoved, mSignalScope, [layerId, this]( KadasItemLayer::ItemId id ) { removeBillboard( layerId, id ); } );
  }
}

void KadasGlobeBillboardManager::addBillboard( const QString &layerId, KadasItemLayer::ItemId itemId )
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

  QgsCoordinateReferenceSystem crs84( "EPSG:4326" );

  QgsCoordinateTransform crst( item->crs(), crs84, QgsProject::instance() );
  QgsPointXY pos = crst.transform( item->position() );

  osgEarth::GeoPoint geop( osgEarth::SpatialReference::get( "wgs84" ), pos.x(), pos.y(), 0, osgEarth::ALTMODE_RELATIVE );
  QImage image = item->symbolImage();
  if ( item->symbolAnchor().x() != 0.5 )
  {
    int offset = item->symbolAnchor().x() * image.width() + image.width() / 2;
    QImage newimage( image.width() + 2 * qAbs( offset ), image.height(), image.format() );
    newimage.fill( Qt::transparent );
    QPainter p( &newimage );
    p.drawImage( offset < 0 ? 0 : 2 * offset, 0, image );
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

  mRegistry[layerId][itemId] = placeNode;
  mGroup->addChild( placeNode );
}

void KadasGlobeBillboardManager::removeBillboard( const QString &layerId, KadasItemLayer::ItemId itemId )
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
