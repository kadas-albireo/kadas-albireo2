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

#include <QMenu>

#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationitem.h>

#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/mapitems/kadasmapitem.h"


class KadasItemLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasItemLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id(), &rendererContext )
    {
      for ( ItemId id : std::as_const( layer->mItemOrder ) )
      {
        mRenderItems.append( layer->mItems[id]->clone() );
      }
      std::stable_sort( mRenderItems.begin(), mRenderItems.end(), []( KadasMapItem *a, KadasMapItem *b ) { return a->zIndex() < b->zIndex(); } );
      mRenderOpacity = layer->opacity();
    }
    bool render() override
    {
      for ( const KadasMapItem *item : std::as_const( mRenderItems ) )
      {
        if ( item && item->isVisible() )
        {
          renderContext()->painter()->save();
          renderContext()->painter()->setOpacity( mRenderOpacity );
          renderContext()->setCoordinateTransform( QgsCoordinateTransform( item->crs(), renderContext()->coordinateTransform().destinationCrs(), renderContext()->transformContext() ) );
          item->render( *renderContext() );
          renderContext()->painter()->restore();
        }
      }
      return true;
    }

  private:
    QList<KadasMapItem *> mRenderItems;
    double mRenderOpacity = 1.;
};


KadasItemLayer::KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs )
  : KadasPluginLayer( layerType(), name )
{
  setCrs( crs );
  mValid = true;
}

KadasItemLayer::~KadasItemLayer()
{
  qDeleteAll( mItems );
  mItems.clear();
}

QgsAnnotationLayer *KadasItemLayer::qgisAnnotationLayer( const QgsCoordinateReferenceSystem &crs ) const
{
  QgsAnnotationLayer::LayerOptions lo( QgsProject::instance()->transformContext() );
  QgsAnnotationLayer *al = new QgsAnnotationLayer( name(), lo );
  al->setCrs( crs );

  QList<QgsAnnotationItem *> items;

  for ( ItemId id : std::as_const( mItemOrder ) )
  {
    items << mItems[id]->annotationItem( crs );
  }
  std::stable_sort( items.begin(), items.end(), []( QgsAnnotationItem *a, QgsAnnotationItem *b ) { return a->zIndex() < b->zIndex(); } );

  for ( QgsAnnotationItem *item : std::as_const( items ) )
    al->addItem( item );

  return al;
}

KadasItemLayer::ItemId KadasItemLayer::addItem( KadasMapItem *item )
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
  emit repaintRequested();
  return id;
}

KadasMapItem *KadasItemLayer::takeItem( const ItemId &itemId )
{
  KadasMapItem *item = mItems.take( itemId );
  if ( item )
  {
    mFreeIds.append( itemId );
    mItemBounds.remove( itemId );
    mItemOrder.removeOne( itemId );
    emit repaintRequested();
  }
  return item;
}

KadasItemLayer *KadasItemLayer::clone() const
{
  auto layer = std::make_unique<KadasItemLayer>( name(), crs() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    layer->mItems.insert( it.key(), it.value()->clone() );
  }
  layer->mItemOrder = mItemOrder;
  layer->mItemBounds = mItemBounds;
  layer->mIdCounter = mIdCounter;
  layer->mFreeIds = mFreeIds;
  return layer.release();
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

  bool hasScaleBasedVisibiliy { layerEl.attributes().namedItem( QStringLiteral( "hasScaleBasedVisibilityFlag" ) ).nodeValue() == '1' };
  setScaleBasedVisibility( hasScaleBasedVisibiliy );
  bool ok;
  double maxScale { layerEl.attributes().namedItem( QStringLiteral( "maxScale" ) ).nodeValue().toDouble( &ok ) };
  if ( ok )
  {
    setMaximumScale( maxScale );
  }
  double minScale { layerEl.attributes().namedItem( QStringLiteral( "minScale" ) ).nodeValue().toDouble( &ok ) };
  if ( ok )
  {
    setMinimumScale( minScale );
  }

  QDomNodeList itemEls = layerEl.elementsByTagName( "MapItem" );
  for ( int i = 0, n = itemEls.size(); i < n; ++i )
  {
    KadasMapItem *item = KadasMapItem::fromXml( itemEls.at( i ).toElement() );
    if ( item )
    {
      mItems.insert( ++mIdCounter, item );
      mItemOrder.append( mIdCounter );
      QgsCoordinateTransform trans( item->crs(), crs(), mTransformContext );
      mItemBounds.insert( mIdCounter, trans.transformBoundingBox( item->boundingBox() ) );
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
  layerEl.setAttribute( QStringLiteral( "hasScaleBasedVisibilityFlag" ), hasScaleBasedVisibility() ? 1 : 0 );
  layerEl.setAttribute( QStringLiteral( "maxScale" ), maximumScale() );
  layerEl.setAttribute( QStringLiteral( "minScale" ), minimumScale() );
  for ( auto it = mItemOrder.begin(), itEnd = mItemOrder.end(); it != itEnd; ++it )
  {
    layerEl.appendChild( mItems[*it]->writeXml( document ) );
  }
  return true;
}


void KadasItemLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  Q_UNUSED( menu );
  Q_UNUSED( layer );
}
