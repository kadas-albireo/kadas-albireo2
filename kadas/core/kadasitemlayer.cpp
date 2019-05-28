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

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadasmapitem.h>


class KadasItemLayer::Renderer : public QgsMapLayerRenderer {
public:
  Renderer( KadasItemLayer* layer, QgsRenderContext& rendererContext )
    : QgsMapLayerRenderer( layer->id() )
    , mLayer(layer)
    , mRendererContext( rendererContext )
  {}
  bool render() override
  {
    for(const KadasMapItem* item : mLayer->mItems.values()) {
      if ( item ) {
        mRendererContext.painter()->save();
        mRendererContext.painter()->translate(item->translationOffset());
        item->render( mRendererContext );
        mRendererContext.painter()->restore();
      }
    }
    return true;
  }

private:
  KadasItemLayer* mLayer;
  QgsRenderContext& mRendererContext;
};


KadasItemLayer::KadasItemLayer(const QString &name)
  : QgsPluginLayer(layerType(), name)
{

}

void KadasItemLayer::addItem(KadasMapItem* item)
{
  mItems.insert(QUuid::createUuid().toString(), item);
}

KadasMapItem* KadasItemLayer::takeItem(const QString& itemId)
{
  return mItems.take(itemId);
}

KadasItemLayer* KadasItemLayer::clone() const{
  KadasItemLayer* layer = new KadasItemLayer(name());
  // TODO
//  layer->mItems =
  return layer;
}

QgsMapLayerRenderer* KadasItemLayer::createMapRenderer( QgsRenderContext& rendererContext )
{
  return new Renderer(this, rendererContext);
}

QgsRectangle KadasItemLayer::extent() const
{
  QgsRectangle rect;
  for(const KadasMapItem* item : mItems.values())
  {
    if(rect.isNull()) {
      rect = item->boundingBox();
    } else {
      rect.combineExtentWith(item->boundingBox());
    }
  }
  return rect;
}

void KadasItemLayer::setTransformContext(const QgsCoordinateTransformContext& ctx)
{
  // TODO
}

QString KadasItemLayer::pickItem(const QgsRectangle& pickRect, const QgsMapSettings& mapSettings) const
{
  for(auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it) {
    if(it.value()->intersects(pickRect, mapSettings)) {
      return it.key();
    }
  }
  return QString();
}
