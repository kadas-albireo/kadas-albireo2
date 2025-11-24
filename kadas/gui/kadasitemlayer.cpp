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

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>

#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationitem.h>

#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/3d/kadasmapitemlayer3drenderer.h"


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
      bool omitSinglePoint = renderContext()->customProperties().contains( "globe" );
      for ( const KadasMapItem *item : std::as_const( mRenderItems ) )
      {
        if ( item && item->isVisible() && ( !omitSinglePoint || !item->isPointSymbol() ) )
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

  std::unique_ptr<KadasMapItemLayer3DRenderer> r = std::make_unique<KadasMapItemLayer3DRenderer>();
  r->setLayer( this );
  QgsTextFormat format = r->textFormat();
  format.setColor( QColor( Qt::yellow ) );
  r->setTextFormat( format );
  r->setCalloutLineColor( QColor( Qt::yellow ) );
  setRenderer3D( r.release() );
}

KadasItemLayer::KadasItemLayer( const QString &name, const QgsCoordinateReferenceSystem &crs, const QString &layerType )
  : KadasPluginLayer( layerType, name )
{
  setCrs( crs );
  mValid = true;

  std::unique_ptr<KadasMapItemLayer3DRenderer> r = std::make_unique<KadasMapItemLayer3DRenderer>();
  r->setLayer( this );
  setRenderer3D( r.release() );
}

KadasItemLayer::~KadasItemLayer()
{
  qDeleteAll( mItems );
  mItems.clear();
}

QgsAnnotationLayer *KadasItemLayer::qgisAnnotationLayer() const
{
  QgsAnnotationLayer::LayerOptions lo( QgsProject::instance()->transformContext() );
  QgsAnnotationLayer *al = new QgsAnnotationLayer( name(), lo );

  QList<QgsAnnotationItem *> items;

  for ( ItemId id : std::as_const( mItemOrder ) )
  {
    if ( !mItems[id]->useQgisAnnotations() )
      continue;

    // TODO: we need to handle different CRS here!
    items << mItems[id]->annotationItem()->clone();
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
  item->setOwnerLayer( this );
  mItems.insert( id, item );
  mItemOrder.append( id );
  QgsCoordinateTransform trans( item->crs(), crs(), mTransformContext );
  mItemBounds.insert( id, trans.transformBoundingBox( item->boundingBox() ) );
  item->setSymbolScale( mSymbolScale );
  emit itemAdded( id );
  emit repaintRequested();
  return id;
}

void KadasItemLayer::lowerItem( const ItemId &itemId )
{
  int pos = mItemOrder.indexOf( itemId );
  mItemOrder.removeAt( pos );
  mItemOrder.insert( std::max( 0, pos - 1 ), itemId );
  emit repaintRequested();
}

void KadasItemLayer::raiseItem( const ItemId &itemId )
{
  int pos = mItemOrder.indexOf( itemId );
  mItemOrder.removeAt( pos );
  mItemOrder.insert( std::min( mItemOrder.length(), pos + 1 ), itemId );
  emit repaintRequested();
}

KadasMapItem *KadasItemLayer::takeItem( const ItemId &itemId )
{
  KadasMapItem *item = mItems.take( itemId );
  if ( item )
  {
    item->setOwnerLayer( nullptr );
    mFreeIds.append( itemId );
    mItemBounds.remove( itemId );
    mItemOrder.removeOne( itemId );
    emit itemRemoved( itemId );
    emit repaintRequested();
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
      item->setOwnerLayer( this );
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

KadasItemLayer::ItemId KadasItemLayer::pickItem( const KadasMapPos &mapPos, const QgsMapSettings &mapSettings, KadasItemLayer::PickObjective pickObjective ) const
{
  for ( auto it = mItemOrder.rbegin(), itEnd = mItemOrder.rend(); it != itEnd; ++it )
  {
    KadasMapItem *item = mItems[*it];
    if ( pickObjective == PickObjective::PICK_OBJECTIVE_TOOLTIP && item->tooltip().isEmpty() )
    {
      continue;
    }
    if ( item->hitTest( mapPos, mapSettings ) )
    {
      return *it;
    }
  }
  return ITEM_ID_NULL;
}

KadasItemLayer::ItemId KadasItemLayer::pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const
{
  return pickItem( KadasMapPos::fromPoint( pickRect.center() ), mapSettings );
}

QPair<QgsPointXY, double> KadasItemLayer::snapToVertex( const QgsPointXY &mapPos, const QgsMapSettings &settings, double tolPixels ) const
{
  QgsCoordinateTransform crst( crs(), settings.destinationCrs(), mTransformContext );
  double minDist = std::numeric_limits<double>::max();
  QgsPointXY minPos;
  for ( auto it = mItemBounds.begin(), itEnd = mItemBounds.end(); it != itEnd; ++it )
  {
    QgsRectangle bbox = crst.transformBoundingBox( it.value() );
    bbox.grow( settings.mapUnitsPerPixel() * tolPixels );
    if ( bbox.contains( mapPos ) )
    {
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

QString KadasItemLayer::asKml( const QgsRenderContext &context, QuaZip *kmzZip, const QgsRectangle &exportRect ) const
{
  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Folder>" << "\n";
  outStream << "<name>" << name() << "</name>" << "\n";
  for ( const KadasMapItem *item : mItems )
  {
    if ( !exportRect.isEmpty() )
    {
      QgsRectangle testRect = QgsCoordinateTransform( crs(), item->crs(), mTransformContext ).transformBoundingBox( exportRect );
      if ( !testRect.intersects( item->boundingBox() ) )
      {
        continue;
      }
    }
    outStream << item->asKml( context, kmzZip );
  }
  outStream << "</Folder>" << "\n";
  outStream.flush();

  return outString;
}

void KadasItemLayer::setSymbolScale( double scale )
{
  mSymbolScale = scale;
  for ( KadasMapItem *item : mItems )
  {
    item->setSymbolScale( scale );
  }
  triggerRepaint();
}


KadasItemLayerRegistry::KadasItemLayerRegistry()
{
}

void KadasItemLayerRegistry::init()
{
  connect( QgsProject::instance(), &QgsProject::cleared, instance(), &KadasItemLayerRegistry::clear );
  connect( QgsProject::instance(), &QgsProject::readProject, instance(), &KadasItemLayerRegistry::readFromProject );
  connect( QgsProject::instance(), &QgsProject::writeProject, instance(), &KadasItemLayerRegistry::writeToProject );
}
void KadasItemLayerRegistry::clear()
{
  mLayerIdMap.clear();
}

void KadasItemLayerRegistry::readFromProject( const QDomDocument &doc )
{
  mLayerIdMap.clear();
  QDomElement itemsEl = doc.firstChildElement( "qgis" ).firstChildElement( "StandardItemLayers" );
  if ( !itemsEl.isNull() )
  {
    QDomNodeList items = itemsEl.elementsByTagName( "StandardItemLayer" );
    for ( int i = 0, n = items.size(); i < n; ++i )
    {
      QDomElement item = items.at( i ).toElement();
      StandardLayer stdLayer = static_cast<StandardLayer>( item.attribute( "layer" ).toInt() );
      QString layerId = item.attribute( "layerId" );
      mLayerIdMap[stdLayer] = layerId;
    }
  }
}

void KadasItemLayerRegistry::writeToProject( QDomDocument &doc )
{
  QDomElement root = doc.firstChildElement( "qgis" );
  QDomElement itemsEl = doc.createElement( "StandardItemLayers" );
  for ( auto it = mLayerIdMap.begin(), itEnd = mLayerIdMap.end(); it != itEnd; ++it )
  {
    QDomElement itemEl = doc.createElement( "StandardItemLayer" );
    itemEl.setAttribute( "layer", static_cast<int>( it.key() ) );
    itemEl.setAttribute( "layerId", it.value() );
    itemsEl.appendChild( itemEl );
  }
  root.appendChild( itemsEl );
}

KadasItemLayer *KadasItemLayerRegistry::getOrCreateItemLayer( StandardLayer layer )
{
  KadasItemLayer *itemLayer = nullptr;
  if ( instance()->mLayerIdMap.contains( layer ) )
  {
    itemLayer = qobject_cast<KadasItemLayer *>( QgsProject::instance()->mapLayer( instance()->mLayerIdMap[layer] ) );
  }

  if ( !itemLayer && standardLayerNames().contains( layer ) )
  {
    itemLayer = new KadasItemLayer( standardLayerNames()[layer], QgsCoordinateReferenceSystem( "EPSG:3857" ) );
    instance()->mLayerIdMap[layer] = itemLayer->id();
    QgsProject::instance()->addMapLayer( itemLayer );
  }
  return itemLayer;
}

const QMap<KadasItemLayerRegistry::StandardLayer, QString> &KadasItemLayerRegistry::standardLayerNames()
{
  static QMap<StandardLayer, QString> names = {
    { StandardLayer::RedliningLayer, tr( "Redlining" ) },
    { StandardLayer::SymbolsLayer, tr( "Symbols" ) },
    { StandardLayer::PicturesLayer, tr( "Pictures" ) },
    { StandardLayer::PinsLayer, tr( "Pins" ) },
    { StandardLayer::RoutesLayer, tr( "Routes" ) }
  };
  return names;
}

QList<KadasItemLayer *> KadasItemLayerRegistry::getItemLayers()
{
  const auto mapLayers = QgsProject::instance()->mapLayers().values();

  QList<KadasItemLayer *> itemLayers;
  for ( QgsMapLayer *mapLayer : mapLayers )
  {
    KadasItemLayer *itemLayer = qobject_cast<KadasItemLayer *>( mapLayer );
    if ( itemLayer )
    {
      itemLayers.append( itemLayer );
    }
  }

  return itemLayers;
}

KadasItemLayerRegistry *KadasItemLayerRegistry::instance()
{
  static KadasItemLayerRegistry registry;
  return &registry;
}

void KadasItemLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  if ( dynamic_cast<KadasItemLayer *>( layer ) )
  {
    KadasItemLayer *itemLayer = static_cast<KadasItemLayer *>( layer );

    QWidget *transpWidget = new QWidget();
    QHBoxLayout *transpLayout = new QHBoxLayout( transpWidget );

    QLabel *transpLabel = new QLabel( tr( "Symbol scale:" ) );
    transpLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    transpLayout->addWidget( transpLabel );

    QSlider *scaleSlider = new QSlider( Qt::Horizontal );
    scaleSlider->setRange( -10, 10 );
    scaleSlider->setValue( log10( itemLayer->symbolScale() ) * 10 );
    scaleSlider->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    scaleSlider->setTracking( false );
    connect( scaleSlider, &QSlider::valueChanged, this, [=]( double value ) {
      itemLayer->setSymbolScale( pow( 10., value / 10. ) );
    } );
    transpLayout->addWidget( scaleSlider );

    QWidgetAction *scaleAction = new QWidgetAction( menu );
    scaleAction->setDefaultWidget( transpWidget );
    menu->addAction( scaleAction );
  }
}
