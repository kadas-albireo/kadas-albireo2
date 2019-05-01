/***************************************************************************
    kadasfeaturepicker.cpp
    ----------------------
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

#include <qgis/qgis.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapcanvasannotationitem.h>
#include <qgis/qgspallabeling.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadaspluginlayer.h>

#include "kadasfeaturepicker.h"

KadasFeaturePicker::PickResult KadasFeaturePicker::pick( const QgsMapCanvas* canvas, const QPoint &canvasPos, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType, filter_t filter )
{
  PickResult pickResult;

  // First, try annotations
  QgsMapCanvasAnnotationItem* annotationItem = nullptr;
  // TODO: Helper function which uses KadasAnnotationItem::hitTest
  for(QGraphicsItem* item : canvas->items( canvasPos ))
  {
    annotationItem = dynamic_cast<QgsMapCanvasAnnotationItem*>(item);
    if(annotationItem) {
      break;
    }
  }
  if ( annotationItem )
  {
    pickResult.annotation = annotationItem;
    pickResult.boundingBox = annotationItem->boundingRect(); // TODO screenBoundingRect?
    return pickResult;
  }

  // Then, try labels
  const QgsLabelingResults* labelingResults = canvas->labelingResults();
  QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsAtPosition( mapPos ) : QList<QgsLabelPosition>();
  if ( !labelPositions.isEmpty() )
  {
    QgsMapLayer* layer = QgsProject::instance()->mapLayer( labelPositions.front().layerID );
    if ( layer )
    {
      pickResult.layer = layer;
      pickResult.labelPos = labelPositions.front();
      return pickResult;
    }
  }

  // Last, try layer features
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas->mapSettings() );
  double radiusmm = QSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );

  QgsFeatureList features;
  for ( QgsMapLayer* layer : canvas->layers() )
  {
    if ( qobject_cast<KadasPluginLayer*>(layer) )
    {
      KadasPluginLayer* pluginLayer = static_cast<KadasPluginLayer*>( layer );
      QVariant result;
      QRect boundingBox;
      if ( pluginLayer->testPick( mapPos, canvas->mapSettings(), result, boundingBox ) )
      {
        pickResult.layer = layer;
        pickResult.otherResult = result;
        pickResult.boundingBox = boundingBox;
        return pickResult;
      }
    }
    if ( layer->type() != QgsMapLayerType::VectorLayer )
    {
      continue;
    }
    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( layer );
    if ( geomType != QgsWkbTypes::UnknownGeometry && vlayer->geometryType() != QgsWkbTypes::UnknownGeometry && vlayer->geometryType() != geomType )
    {
      continue;
    }
    if ( vlayer->hasScaleBasedVisibility() &&
         ( vlayer->minimumScale() > canvas->mapSettings().scale() ||
           vlayer->maximumScale() <= canvas->mapSettings().scale() ) )
    {
      continue;
    }

    QgsFeatureRenderer* renderer = vlayer->renderer();
    bool filteredRendering = false;
    if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
    {
      // setup scale for scale dependent visibility (rule based)
      renderer->startRender( renderContext, vlayer->fields() );
      filteredRendering = renderer->capabilities() & QgsFeatureRenderer::Filter;
    }

    QgsRectangle layerFilterRect = canvas->mapSettings().mapToLayerCoordinates( vlayer, filterRect );
    QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
    QgsFeature feature;
    while ( fit.nextFeature( feature ) )
    {
      if ( filteredRendering && !renderer->willRenderFeature( feature, renderContext ) )
      {
        continue;
      }
      if ( filter && !filter( feature ) )
      {
        continue;
      }
      if ( geomType != QgsWkbTypes::UnknownGeometry && feature.geometry().type() != geomType )
      {
        continue;
      }
      features.append( feature );
    }
    if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
    {
      renderer->stopRender( renderContext );
    }
    if ( !features.empty() )
    {
      pickResult.layer = vlayer;
      pickResult.feature = features.front();
      return pickResult;
    }
  }
  return pickResult;
}
