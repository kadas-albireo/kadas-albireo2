/***************************************************************************
    kadasmapgridlayer.cpp
    ---------------------
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
#include <QMenu>
#include <QVector2D>

#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinateformatter.h>
#include <qgis/qgsgeometryutils.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/core/kadaslatlontoutm.h"
#include "kadasmapgridlayer.h"
#include "kadasmapgridlayerrenderer.h"


KadasMapGridLayer::KadasMapGridLayer( const QString &name )
  : KadasPluginLayer( layerType(), name )
{
  mValid = true;
}

void KadasMapGridLayer::setup( GridType type, double intervalX, double intervalY, int cellSize )
{
  mGridConfig.gridType = type;
  mGridConfig.intervalX = intervalX;
  mGridConfig.intervalY = intervalY;
  mGridConfig.cellSize = cellSize;
}

KadasMapGridLayer *KadasMapGridLayer::clone() const
{
  KadasMapGridLayer *layer = new KadasMapGridLayer( name() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  layer->mGridConfig = mGridConfig;
  return layer;
}

QgsMapLayerRenderer *KadasMapGridLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new KadasMapGridLayerRenderer( this, rendererContext );
}

QList<KadasMapGridLayer::IdentifyResult> KadasMapGridLayer::identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings )
{
  // TODO?
  return QList<KadasMapGridLayer::IdentifyResult>();
}

bool KadasMapGridLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mOpacity = ( 100. - layerEl.attribute( "transparency" ).toInt() ) / 100.;
  mGridConfig.gridType = static_cast<GridType>( layerEl.attribute( "gridtype" ).toInt() );
  mGridConfig.intervalX = layerEl.attribute( "intervalX" ).toDouble();
  mGridConfig.intervalY = layerEl.attribute( "intervalY" ).toDouble();
  mGridConfig.cellSize = layerEl.attribute( "cellSize" ).toInt();
  mGridConfig.fontSize = layerEl.attribute( "fontSize" ).toInt();
  mGridConfig.color = QgsSymbolLayerUtils::decodeColor( layerEl.attribute( "color" ) );
  mGridConfig.labelingMode = static_cast<LabelingMode>( layerEl.attribute( "labelingMode" ).toInt() );
  return true;
}

bool KadasMapGridLayer::writeXml( QDomNode &layer_node, QDomDocument & /*document*/, const QgsReadWriteContext &context ) const
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", 100. - mOpacity * 100. );
  layerEl.setAttribute( "gridtype", mGridConfig.gridType );
  layerEl.setAttribute( "intervalX", mGridConfig.intervalX );
  layerEl.setAttribute( "intervalY", mGridConfig.intervalY );
  layerEl.setAttribute( "cellSize", mGridConfig.cellSize );
  layerEl.setAttribute( "fontSize", mGridConfig.fontSize );
  layerEl.setAttribute( "color", QgsSymbolLayerUtils::encodeColor( mGridConfig.color ) );
  layerEl.setAttribute( "labelingMode", static_cast<int>( mGridConfig.labelingMode ) );
  return true;
}

void KadasMapGridLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  menu->addAction( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ), tr( "Edit" ), this, [this, layer] {
    mActionMapGridTool->trigger();
  } );
}
