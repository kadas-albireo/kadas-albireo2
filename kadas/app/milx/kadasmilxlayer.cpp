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

#include <kadas/app/milx/kadasmilxclient.h>
#include <kadas/app/milx/kadasmilxitem.h>
#include <kadas/app/milx/kadasmilxlayer.h>


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
      // TODO
//      QStringList flags = mRendererContext.customRenderFlags().split( ";" );
//      bool omitSinglePoint = flags.contains( "globe" ) || flags.contains( "kml" );
      bool omitSinglePoint = false;
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
      int dpi = mRendererContext.painter()->device()->logicalDpiX();
      double scaleFactor = double( mRendererContext.painter()->device()->logicalDpiX() ) / double( QApplication::desktop()->logicalDpiX() );
      QRect screenExtent = KadasMilxItem::computeScreenExtent( mRendererContext.coordinateTransform().transform( mRendererContext.extent() ), mRendererContext.mapToPixel() );
      if ( !KadasMilxClient::updateSymbols( screenExtent, dpi, scaleFactor, symbols, result ) )
      {
        return false;
      }
      mRendererContext.painter()->save();
      mRendererContext.painter()->setOpacity( mLayer->opacity() / 100. );
      for ( int i = 0, n = result.size(); i < n; ++i )
      {
        QPoint itemOrigin = symbols[i].points.front();
        QPoint renderPos = itemOrigin + result[i].offset;
        if ( !renderItems[i]->isMultiPoint() )
        {
          // Draw line from visual reference point to actual refrence point
          mRendererContext.painter()->drawLine( itemOrigin, itemOrigin - renderItems[i]->constState()->userOffset );
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

QgsMapLayerRenderer *KadasMilxLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QString KadasMilxLayer::pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const
{
  if ( mIsApproved )
  {
    // No items can be picked from approved layer
    return QString();
  }
  QPoint screenPos = mapSettings.mapToPixel().transform( pickRect.center() ).toQPointF().toPoint();
  QList<KadasMilxClient::NPointSymbol> symbols;
  QMap<int, QString> itemIdMap;
  for ( auto it = mItems.begin(), itEnd = mItems.end(); it != itEnd; ++it )
  {
    const KadasMilxItem *milxItem = dynamic_cast<const KadasMilxItem *>( it.value() );
    if ( milxItem )
    {
      itemIdMap.insert( symbols.size(), it.key() );
      symbols.append( milxItem->toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() ) );
    }
  }
  int selectedSymbol = -1;
  QRect bbox;
  if ( KadasMilxClient::pickSymbol( symbols, screenPos, selectedSymbol, bbox ) && selectedSymbol >= 0 )
  {
    return itemIdMap[selectedSymbol];
  }
  return QString();
}

void KadasMilxLayer::setApproved( bool approved )
{
  mIsApproved = approved;
  emit approvedChanged( approved );
  triggerRepaint();
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
  symbolSizeEl.appendChild( doc.createTextNode( QString::number( ( KadasMilxClient::getSymbolSize() * 25.4 ) / dpi ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( mIsApproved ? "1" : "0" ) );
  milxLayerEl.appendChild( bwEl );
}

bool KadasMilxLayer::importFromMilxly( QDomElement &milxLayerEl, int dpi, QString &errorMsg )
{
  setName( milxLayerEl.firstChildElement( "Name" ).text() );
  //    QString layerType = milxLayerEl.firstChildElement( "LayerType" ).text(); // TODO
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
    srcCrs.createFromProj4( QString( "+proj=utm +zone=%1 +datum=WGS84 +units=m +no_defs" ).arg( projZone ) );
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


void KadasMilxLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  if ( dynamic_cast<KadasMilxLayer *>( layer ) )
  {
    KadasMilxLayer *milxLayer = static_cast<KadasMilxLayer *>( layer );
    menu->addAction( tr( "Approved layer" ), [milxLayer]
    {
      milxLayer->setApproved( !milxLayer->isApproved() );
    } )->setChecked( milxLayer->isApproved() );
  }
}
