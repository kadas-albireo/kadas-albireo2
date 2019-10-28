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
      QList<KadasMapItem *> items = mLayer->mItems.values();
      qStableSort( items.begin(), items.end(), []( KadasMapItem * a, KadasMapItem * b ) { return a->zIndex() < b->zIndex(); } );
      for ( const KadasMapItem *item : items )
      {
        if ( item )
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
  mItems.insert( QUuid::createUuid().toString(), item );
}

KadasMapItem *KadasItemLayer::takeItem( const QString &itemId )
{
  return mItems.take( itemId );
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
  for ( const KadasMapItem *item : mItems.values() )
  {
    QgsCoordinateTransform trans( item->crs(), crs(), mTransformContext );
    if ( rect.isNull() )
    {
      rect = trans.transformBoundingBox( item->boundingBox() );
    }
    else
    {
      rect.combineExtentWith( trans.transformBoundingBox( item->boundingBox() ) );
    }
  }
  return rect;
}


bool KadasItemLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  QDomNodeList itemEls = layerEl.elementsByTagName( "MapItem" );
  for ( int i = 0, n = itemEls.size(); i < n; ++i )
  {
    QDomElement itemEl = itemEls.at( i ).toElement();
    QString name = itemEl.attribute( "name" );
    QString id = itemEl.attribute( "id" );
    QString crs = itemEl.attribute( "crs" );
    QJsonDocument data = QJsonDocument::fromJson( itemEl.firstChild().toCDATASection().data().toLocal8Bit() );
    KadasMapItem::RegistryItemFactory factory = KadasMapItem::registry()->value( name );
    if ( factory )
    {
      KadasMapItem *item = factory( QgsCoordinateReferenceSystem( crs ) );
      if ( item->deserialize( data.object() ) )
      {
        mItems.insert( id, item );
      }
      else
      {
        QgsDebugMsg( "Item deserialization failed: " + id );
      }
    }
    else
    {
      QgsDebugMsg( "Unknown item: " + id );
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
    itemEl.setAttribute( "id", it.key() );
    itemEl.setAttribute( "crs", it.value()->crs().authid() );
    QJsonDocument doc;
    doc.setObject( it.value()->serialize() );
    itemEl.appendChild( document.createCDATASection( doc.toJson( QJsonDocument::Compact ) ) );
    layerEl.appendChild( itemEl );
  }
  return true;
}

QString KadasItemLayer::pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const
{
  KadasMapRect rect( pickRect.xMinimum(), pickRect.yMinimum(), pickRect.xMaximum(), pickRect.yMaximum() );
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    if ( it.value()->intersects( rect, mapSettings ) )
    {
      return it.key();
    }
  }
  return QString();
}

QString KadasItemLayer::pickItem( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) const
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
