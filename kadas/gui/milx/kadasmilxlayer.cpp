/***************************************************************************
    kadasmilxlayer.cpp
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

#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>

#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsrendercontext.h>

#include <kadas/gui/milx/kadasmilxclient.h>
#include <kadas/gui/milx/kadasmilxitem.h>
#include <kadas/gui/milx/kadasmilxlayer.h>


class KadasMilxLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasMilxLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id() )
      , mLayer( layer )
      , mRendererContext( rendererContext )
    {}
    bool render() override
    {
      QList<KadasMapItem *> items = mLayer->items().values();
      QList<KadasMilxClient::NPointSymbol> symbols;
      QList<KadasMilxItem *> renderItems;
      int dpi = mRendererContext.painter()->device()->logicalDpiX();
      double dpiScale = double( dpi ) / double( QApplication::desktop()->logicalDpiX() );
      bool omitSinglePoint = mRendererContext.customRenderingFlags().contains( "globe" );
      for ( int i = 0, n = items.size(); i < n; ++i )
      {
        KadasMilxItem *item = dynamic_cast<KadasMilxItem *>( items[i] );
        if ( !item || item->constState()->points.isEmpty() || ( omitSinglePoint && !item->isMultiPoint() ) )
        {
          // Skip symbols
          continue;
        }
        symbols.append( item->toSymbol( mRendererContext.mapToPixel(), mRendererContext.coordinateTransform().destinationCrs(), !mLayer->mIsApproved ) );
        renderItems.append( item );
      }
      if ( symbols.isEmpty() )
      {
        return true;
      }
      QList<KadasMilxClient::NPointSymbolGraphic> result;
      KadasMilxSymbolSettings symSettings = mLayer->milxSymbolSettings();
      symSettings.lineWidth *= dpiScale;
      symSettings.symbolSize *= dpiScale;
      QRect screenExtent = KadasMilxItem::computeScreenExtent( mRendererContext.mapExtent(), mRendererContext.mapToPixel() );
      if ( !KadasMilxClient::updateSymbols( screenExtent, dpi, symbols, symSettings, result ) )
      {
        return false;
      }
      mRendererContext.painter()->save();
      mRendererContext.painter()->setOpacity( mLayer->opacity() );
      for ( int i = 0, n = result.size(); i < n; ++i )
      {
        QPoint itemOrigin = symbols[i].points.front();
        QPoint renderPos = itemOrigin + result[i].offset + renderItems[i]->constState()->userOffset * dpiScale;
        if ( !renderItems[i]->isMultiPoint() )
        {
          // Draw line from visual reference point to actual refrence point
          mRendererContext.painter()->setPen( QPen( Qt::black, dpiScale ) );
          mRendererContext.painter()->drawLine( itemOrigin, itemOrigin + renderItems[i]->constState()->userOffset * dpiScale );
        }
        mRendererContext.painter()->drawImage( renderPos, result[i].graphic );
      }
      mRendererContext.painter()->restore();
      return true;
    }

  private:
    KadasMilxLayer *mLayer;
    QgsRenderContext &mRendererContext;
};

KadasMilxLayer::KadasMilxLayer( const QString &name )
  : KadasItemLayer( name, QgsCoordinateReferenceSystem( "EPSG:4326" ), layerType() )
{
}

bool KadasMilxLayer::acceptsItem( const KadasMapItem *item ) const
{
  return dynamic_cast<const KadasMilxItem *>( item );
}

QgsMapLayerRenderer *KadasMilxLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

KadasItemLayer::ItemId KadasMilxLayer::pickItem( const KadasMapPos &mapPos, const QgsMapSettings &mapSettings, PickObjective pickObjective ) const
{
  if ( mIsApproved )
  {
    // No items can be picked from approved layer
    return ITEM_ID_NULL;
  }
  QPoint screenPos = mapSettings.mapToPixel().transform( mapPos ).toQPointF().toPoint();
  QList<KadasMilxClient::NPointSymbol> symbols;
  QMap<int, ItemId> itemIdMap;
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    const KadasMilxItem *milxItem = dynamic_cast<const KadasMilxItem *>( it.value() );
    if ( !milxItem || ( pickObjective == PICK_OBJECTIVE_TOOLTIP && milxItem->tooltip().isEmpty() ) )
    {
      continue;
    }
    itemIdMap.insert( symbols.size(), it.key() );
    symbols.append( milxItem->toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() ) );
    for ( int i = 0, n = symbols.last().points.size(); i < n; ++i )
    {
      symbols.last().points[i] += milxItem->constState()->userOffset;
    }
  }
  int selectedSymbol = -1;
  QRect bbox;
  if ( !symbols.isEmpty() && KadasMilxClient::pickSymbol( symbols, screenPos, milxSymbolSettings(), selectedSymbol, bbox ) && selectedSymbol >= 0 )
  {
    return itemIdMap[selectedSymbol];
  }
  return ITEM_ID_NULL;
}

void KadasMilxLayer::setApproved( bool approved )
{
  mIsApproved = approved;
  emit approvedChanged( approved );
  triggerRepaint();
}

bool KadasMilxLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  bool success = KadasItemLayer::readXml( layer_node, context );
  QDomElement layerEl = layer_node.toElement();
  mIsApproved = layerEl.attribute( "approved" ).toInt() == 1;
  return success;
}

bool KadasMilxLayer::writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  bool success = KadasItemLayer::writeXml( layer_node, document, context );
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "approved", mIsApproved ? "1" : "0" );
  return success;
}

void KadasMilxLayer::exportToMilxly( QDomElement &milxLayerEl, int dpi )
{
  QDomDocument doc = milxLayerEl.ownerDocument();

  QDomElement milxLayerNameEl = doc.createElement( "Name" );
  milxLayerNameEl.appendChild( doc.createTextNode( name() ) );
  milxLayerEl.appendChild( milxLayerNameEl );

  QDomElement milxLayerTypeEl = doc.createElement( "LayerType" );
  milxLayerTypeEl.appendChild( doc.createTextNode( "Normal" ) );
  milxLayerEl.appendChild( milxLayerTypeEl );

  QDomElement graphicListEl = doc.createElement( "GraphicList" );
  milxLayerEl.appendChild( graphicListEl );

  for ( const KadasMapItem *item : mItems )
  {
    if ( dynamic_cast<const KadasMilxItem *>( item ) )
    {
      QDomElement graphicEl = doc.createElement( "MilXGraphic" );
      static_cast<const KadasMilxItem *>( item )->writeMilx( doc, graphicEl );
      graphicListEl.appendChild( graphicEl );
    }
  }

  QDomElement crsEl = doc.createElement( "CoordSystemType" );
  crsEl.appendChild( doc.createTextNode( "WGS84" ) );
  milxLayerEl.appendChild( crsEl );

  QDomElement symbolSizeEl = doc.createElement( "SymbolSize" );
  symbolSizeEl.appendChild( doc.createTextNode( QString::number( ( milxSymbolSettings().symbolSize * 25.4 ) / dpi ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( mIsApproved ? "1" : "0" ) );
  milxLayerEl.appendChild( bwEl );
}

bool KadasMilxLayer::importFromMilxly( const QDomElement &milxLayerEl, int dpi, QString &errorMsg )
{
  setName( milxLayerEl.firstChildElement( "Name" ).text() );
  float symbolSize = milxLayerEl.firstChildElement( "SymbolSize" ).text().toFloat(); // This is in mm
  symbolSize = ( symbolSize * dpi ) / 25.4; // mm to px
  QString crs = milxLayerEl.firstChildElement( "CoordSystemType" ).text();
  if ( crs.isEmpty() )
  {
    errorMsg = tr( "The file is corrupt" );
    return false;
  }
  QString utmZone = milxLayerEl.firstChildElement( "CoordSystemUtmZone" ).text();
  QgsCoordinateReferenceSystem srcCrs;
  if ( crs == "SwissLv03" )
  {
    srcCrs = QgsCoordinateReferenceSystem( "EPSG:21781" );
  }
  else if ( crs == "WGS84" )
  {
    srcCrs = QgsCoordinateReferenceSystem( "EPSG:4326" );
  }
  else if ( crs == "UTM" )
  {
    QString zoneLetter = utmZone.right( 1 ).toUpper();
    QString zoneNumber = utmZone.left( utmZone.length() - 1 );
    QString projZone = zoneNumber + ( zoneLetter == "S" ? " +south" : "" );
    srcCrs.createFromProj( QString( "+proj=utm +zone=%1 +datum=WGS84 +units=m +no_defs" ).arg( projZone ) );
  }
  QgsCoordinateTransform crst( srcCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), mTransformContext );

  mIsApproved = milxLayerEl.firstChildElement( "DisplayBW" ).text().toInt();

  QDomNodeList graphicEls = milxLayerEl.firstChildElement( "GraphicList" ).elementsByTagName( "MilXGraphic" );
  for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
  {
    QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();
    addItem( KadasMilxItem::fromMilx( graphicEl, crst, symbolSize ) );
  }
  return true;
}

const KadasMilxSymbolSettings &KadasMilxLayer::milxSymbolSettings() const
{
  if ( mOverrideMilxSymbolSettings )
  {
    return mMilxSymbolSettings;
  }
  else
  {
    return KadasMilxClient::globalSymbolSettings();
  }
}

void KadasMilxLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  if ( dynamic_cast<KadasMilxLayer *>( layer ) )
  {
    KadasMilxLayer *milxLayer = static_cast<KadasMilxLayer *>( layer );
    QAction *action = menu->addAction( tr( "Approved layer" ), [milxLayer]
    {
      milxLayer->setApproved( !milxLayer->isApproved() );
    } );
    action->setCheckable( true );
    action->setChecked( milxLayer->isApproved() );
  }
}
