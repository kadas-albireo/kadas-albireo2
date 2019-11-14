/***************************************************************************
    kadasglobeprojectlayermanager.cpp
    ---------------------------------
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

#include <osgEarth/ModelLayer>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>

#include <qgis/qgspallabeling.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectorlayerlabeling.h>

#include <kadas/app/globe/kadasglobeprojectlayermanager.h>
#include <kadas/app/globe/kadasglobevectorlayerproperties.h>
#include <kadas/app/globe/featuresource/kadasglobefeatureoptions.h>

void KadasGlobeProjectLayerManager::init( osg::ref_ptr<osgEarth::MapNode> mapNode, const QStringList &visibleLayerIds )
{
  mMapNode = mapNode;
  mLayerSignalScope = new QObject( this );
  hardRefresh( visibleLayerIds );
}

void KadasGlobeProjectLayerManager::reset()
{
  delete mLayerSignalScope;
  mLayerSignalScope = nullptr;
  mMapNode->getMap()->removeLayer( mDrapedLayer ); // abort any rendering
  mTileSource->waitForFinished();
  mDrapedLayer = nullptr;
  mTileSource = nullptr;
  mMapNode = nullptr;
}

void KadasGlobeProjectLayerManager::hardRefresh( const QStringList &visibleLayerIds )
{
  if ( !mMapNode )
    return;

  // Remove draped layer
  if ( mDrapedLayer )
  {
    mMapNode->getMap()->removeLayer( mDrapedLayer );
    mDrapedLayer = nullptr;
  }

  // Remove model layers
  osgEarth::ModelLayerVector modelLayers;
  mMapNode->getMap()->getLayers( modelLayers );
  for ( const osg::ref_ptr<osgEarth::ModelLayer> &modelLayer : modelLayers )
  {
    mMapNode->getMap()->removeLayer( modelLayer );
  }

  delete mLayerSignalScope;
  mLayerSignalScope = new QObject( this );

  osgEarth::TileSourceOptions opts;
  opts.L2CacheSize() = 0;
  mTileSource = new KadasGlobeTileSource( opts );
  mTileSource->open();

  osgEarth::ImageLayerOptions options( "QGIS" );
  options.tileSize() = 128;
  mDrapedLayer = new osgEarth::ImageLayer( options, mTileSource );

  mMapNode->getMap()->addLayer( mDrapedLayer );

  updateLayers( visibleLayerIds );
}

void KadasGlobeProjectLayerManager::updateLayers( const QStringList &visibleLayerIds )
{
  if ( !mMapNode )
    return;

  // Disconnect all previous repaintRequested signals
  delete mLayerSignalScope;
  mLayerSignalScope = new QObject( this );

  // Go over visible layers, determine whether they are to be rendered draped or as models
  QSet<QString> newDrapedLayerIds;
  QSet<QString> newModelLayerIds;
  for ( const QString &layerId : visibleLayerIds )
  {
    QgsMapLayer *mapLayer = QgsProject::instance()->mapLayer( layerId );
    if ( mapLayer )
    {
      KadasGlobeVectorLayerConfig *layerConfig = KadasGlobeVectorLayerConfig::getConfig( mapLayer );
      if ( layerConfig && layerConfig->renderingMode != KadasGlobeVectorLayerConfig::RenderingModeRasterized )
        newModelLayerIds.insert( layerId );
      else
        newDrapedLayerIds.insert( layerId );

      connect( mapLayer, &QgsMapLayer::repaintRequested, mLayerSignalScope, [this, layerId] { updateLayer( layerId ); } );
    }
  }

  // Determine current model layers
  QSet<QString> currentModelLayers;
  osgEarth::ModelLayerVector modelLayers;
  mMapNode->getMap()->getLayers( modelLayers );
  for ( const osg::ref_ptr<osgEarth::ModelLayer> &modelLayer : modelLayers )
  {
    currentModelLayers.insert( QString::fromStdString( modelLayer->getName() ) );
  }

  // Remove old model layers
  QSet<QString> removedModelLayers = QSet<QString>( currentModelLayers ).subtract( newModelLayerIds );
  for ( const QString &layerId : removedModelLayers )
  {
    osgEarth::Layer *layer = mMapNode->getMap()->getLayerByName( layerId.toStdString() );
    if ( layer )
    {
      mMapNode->getMap()->removeLayer( layer );
    }
  }
  // Add new model layers
  QSet<QString> addedModelLayers = QSet<QString>( newModelLayerIds ).subtract( currentModelLayers );
  for ( const QString &layerId : addedModelLayers )
  {
    addModelLayer( layerId );
  }

  // Set new layers for draped layer
  mTileSource->setLayers( newDrapedLayerIds );

  // TODO?
  // mOsgViewer->requestRedraw();
}

void KadasGlobeProjectLayerManager::updateLayer( const QString &layerId )
{
  QgsMapLayer *mapLayer = QgsProject::instance()->mapLayer( layerId );
  if ( !mapLayer || !mMapNode )
  {
    return;
  }
  KadasGlobeVectorLayerConfig *layerConfig = KadasGlobeVectorLayerConfig::getConfig( mapLayer );

  if ( layerConfig && layerConfig->renderingMode != KadasGlobeVectorLayerConfig::RenderingModeRasterized )
  {
    // If was previously a draped layer, remove it and refresh the draped layer
    if ( mTileSource->layers().contains( layerId ) )
    {
      QSet<QString> newLayers =  mTileSource->layers();
      newLayers.remove( layerId );
      mTileSource->setLayers( newLayers );
    }
    // Rebuild model layer
    osgEarth::Layer *layer = mMapNode->getMap()->getLayerByName( mapLayer->id().toStdString() );
    if ( layer )
    {
      mMapNode->getMap()->removeLayer( layer );
    }
    addModelLayer( layerId );
  }
  else
  {
    // If it was previously a model layer, remove it
    osgEarth::Layer *layer = mMapNode->getMap()->getLayerByName( mapLayer->id().toStdString() );
    if ( layer )
    {
      mMapNode->getMap()->removeLayer( layer );
    }
    // Add it to the draped layer if necessary, else refresh area
    if ( !mTileSource->layers().contains( layerId ) )
    {
      QSet<QString> newLayers =  mTileSource->layers();
      newLayers.insert( layerId );
      mTileSource->setLayers( newLayers );
    }
    else
    {
      QgsRectangle extent = QgsCoordinateTransform( mapLayer->crs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() ).transform( mapLayer->extent() );
      mTileSource->refresh( extent );
    }
  }

  // TODO?
  // mOsgViewer->requestRedraw();
}


void KadasGlobeProjectLayerManager::addModelLayer( const QString &layerId )
{
  QgsMapLayer *mapLayer = QgsProject::instance()->mapLayer( layerId );
  if ( !mapLayer )
    return;

  KadasGlobeVectorLayerConfig *layerConfig = KadasGlobeVectorLayerConfig::getConfig( mapLayer );

  osgEarth::Style style;

  osgEarth::AltitudeSymbol *altitudeSymbol = style.getOrCreateSymbol<osgEarth::AltitudeSymbol>();
  altitudeSymbol->clamping() = layerConfig->altitudeClamping;
  altitudeSymbol->technique() = layerConfig->altitudeTechnique;
  altitudeSymbol->binding() = layerConfig->altitudeBinding;
  altitudeSymbol->verticalOffset() = layerConfig->verticalOffset;
  altitudeSymbol->verticalScale() = layerConfig->verticalScale;
  altitudeSymbol->clampingResolution() = layerConfig->clampingResolution;

  if ( layerConfig->extrusionEnabled )
  {
    osgEarth::ExtrusionSymbol *extrusionSymbol = style.getOrCreateSymbol<osgEarth::ExtrusionSymbol>();
    bool extrusionHeightOk = false;
    float extrusionHeight = layerConfig->extrusionHeight.toFloat( &extrusionHeightOk );
    if ( extrusionHeightOk )
    {
      extrusionSymbol->height() = extrusionHeight;
    }
    else
    {
      extrusionSymbol->heightExpression() = layerConfig->extrusionHeight.toStdString();
    }

    extrusionSymbol->flatten() = layerConfig->extrusionFlatten;
    extrusionSymbol->wallGradientPercentage() = layerConfig->extrusionWallGradient;
  }

  QgsVectorLayer *vLayer = qobject_cast<QgsVectorLayer *>( mapLayer );
  if ( vLayer && layerConfig->labelingEnabled )
  {
    osgEarth::TextSymbol *textSymbol = style.getOrCreateSymbol<osgEarth::TextSymbol>();
    textSymbol->declutter() = layerConfig->labelingDeclutter;
    QgsPalLayerSettings lyr = vLayer->labeling()->settings();
    textSymbol->content() = QString( "[%1]" ).arg( lyr.fieldName ).toStdString();
    textSymbol->font() = lyr.format().font().family().toStdString();
    textSymbol->size() = lyr.format().font().pointSize();
    textSymbol->alignment() = osgEarth::TextSymbol::ALIGN_CENTER_TOP;
    osgEarth::Stroke stroke;
    QColor bufferColor = lyr.format().buffer().color();
    stroke.color() = osgEarth::Symbology::Color( bufferColor.redF(), bufferColor.greenF(), bufferColor.blueF(), bufferColor.alphaF() );
    textSymbol->halo() = stroke;
    textSymbol->haloOffset() = lyr.format().buffer().size();
  }
  if ( vLayer )
  {
    QgsRenderContext ctx;
    for ( QgsSymbol *sym : vLayer->renderer()->symbols( ctx ) )
    {
      osgEarth::PolygonSymbol *poly = style.getOrCreateSymbol<osgEarth::PolygonSymbol>();
      QColor color = sym->color();
      poly->fill()->color() = osg::Vec4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() );
    }
  }

  osgEarth::RenderSymbol *renderSymbol = style.getOrCreateSymbol<osgEarth::RenderSymbol>();
  renderSymbol->lighting() = layerConfig->lightingEnabled;
  renderSymbol->backfaceCulling() = false;


  KadasGlobeFeatureOptions featureOpt;
  featureOpt.setLayer( mapLayer );
  featureOpt.style() = style;

  osgEarth::Drivers::FeatureGeomModelOptions geomOpt;
  geomOpt.featureOptions() = featureOpt;

  geomOpt.styles() = new osgEarth::StyleSheet();
  geomOpt.styles()->addStyle( style );

#if 0
  FeatureDisplayLayout layout;
  layout.tileSizeFactor() = 45.0;
  layout.addLevel( FeatureLevel( 0.0f, 200000.0f ) );
  geomOpt.layout() = layout;
#endif

  osgEarth::ModelLayerOptions modelOptions( mapLayer->id().toStdString(), geomOpt );
  mMapNode->getMap()->addLayer( new osgEarth::ModelLayer( modelOptions ) );
}
