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

#include <QUuid>

#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>

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
      QList<KadasMapItem *> items = mLayer->mItems.values();
      qStableSort( items.begin(), items.end(), []( KadasMapItem * a, KadasMapItem * b ) { return a->zIndex() < b->zIndex(); } );
      for ( const KadasMapItem *item : items )
      {
        if ( item )
        {
          mRendererContext.painter()->save();
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


KadasItemLayer::KadasItemLayer( const QString &name )
  : QgsPluginLayer( layerType(), name )
{
  mValid = true;
}

void KadasItemLayer::addItem( KadasMapItem *item )
{
  mItems.insert( QUuid::createUuid().toString(), item );
}

KadasMapItem *KadasItemLayer::takeItem( const QString &itemId )
{
  return mItems.take( itemId );
}

KadasItemLayer *KadasItemLayer::clone() const
{
  KadasItemLayer *layer = new KadasItemLayer( name() );
  // TODO
//  layer->mItems =
  return layer;
}

QgsMapLayerRenderer *KadasItemLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle KadasItemLayer::extent() const
{
  QgsRectangle rect;
  for ( const KadasMapItem *item : mItems.values() )
  {
    QgsCoordinateTransform trans( item->crs(), crs(), mTransformContext );
    if ( rect.isNull() )
    {
      rect = trans.transform( item->boundingBox() );
    }
    else
    {
      rect.combineExtentWith( trans.transform( item->boundingBox() ) );
    }
  }
  return rect;
}

void KadasItemLayer::setTransformContext( const QgsCoordinateTransformContext &ctx )
{
  mTransformContext = ctx;
}

QString KadasItemLayer::pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const
{
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    QgsCoordinateTransform crst( mapSettings.destinationCrs(), it.value()->crs(), transformContext() );
    if ( it.value()->intersects( crst.transform( pickRect ), mapSettings ) )
    {
      return it.key();
    }
  }
  return QString();
}
