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
#include <qgis/qgspallabeling.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/mapitems/kadasgeometryitem.h>
#include <kadas/gui/milx/kadasmilxitem.h>

KadasFeaturePicker::PickResult KadasFeaturePicker::pick( const QgsMapCanvas *canvas, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType, KadasItemLayer::PickObjective pickObjective )
{
  PickResult pickResult;

  for ( QgsMapLayer *layer : canvas->layers() )
  {
    if ( qobject_cast<KadasItemLayer *> ( layer ) )
    {
      pickResult = pickItemLayer( static_cast<KadasItemLayer *>( layer ), canvas, KadasMapPos::fromPoint( mapPos ), pickObjective );
    }
    else if ( qobject_cast<QgsVectorLayer *> ( layer ) )
    {
      pickResult = pickVectorLayer( static_cast<QgsVectorLayer *>( layer ), canvas, mapPos, geomType );
    }
    if ( !pickResult.isEmpty() )
    {
      break;
    }
  }
  return pickResult;
}

KadasFeaturePicker::PickResult KadasFeaturePicker::pick( const QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType )
{
  Q_UNUSED( canvasPos );
  return pick( canvas, mapPos, geomType );
}

KadasFeaturePicker::PickResult KadasFeaturePicker::pickItemLayer( KadasItemLayer *layer, const QgsMapCanvas *canvas, const KadasMapPos &mapPos, KadasItemLayer::PickObjective pickObjective )
{
  PickResult pickResult;
  pickResult.itemId = layer->pickItem( mapPos, canvas->mapSettings(), pickObjective );
  if ( pickResult.itemId != KadasItemLayer::ITEM_ID_NULL )
  {
    pickResult.layer = layer;
    KadasMapItem *item = layer->items()[pickResult.itemId];
    pickResult.crs = item->crs();
    if ( dynamic_cast<KadasGeometryItem *>( item ) )
    {
      pickResult.geom = static_cast<KadasGeometryItem *>( item )->geometry()->clone();
    }
    else if ( dynamic_cast<KadasMilxItem *>( item ) )
    {
      pickResult.geom = static_cast<KadasMilxItem *>( item )->toGeometry();
    }
  }
  return pickResult;
}

KadasFeaturePicker::PickResult KadasFeaturePicker::pickVectorLayer( QgsVectorLayer *vlayer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos, QgsWkbTypes::GeometryType geomType )
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas->mapSettings() );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  KadasMapRect filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );

  PickResult pickResult;

  QgsFeatureList features;
  if ( geomType != QgsWkbTypes::UnknownGeometry && vlayer->geometryType() != QgsWkbTypes::UnknownGeometry && vlayer->geometryType() != geomType )
  {
    return pickResult;
  }
  if ( vlayer->hasScaleBasedVisibility() &&
       ( vlayer->minimumScale() > canvas->mapSettings().scale() ||
         vlayer->maximumScale() <= canvas->mapSettings().scale() ) )
  {
    return pickResult;
  }

  QgsFeatureRenderer *renderer = vlayer->renderer();
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
    if ( geomType != QgsWkbTypes::UnknownGeometry && feature.geometry().type() != geomType )
    {
      continue;
    }
    pickResult.layer = vlayer;
    pickResult.feature = feature;
    pickResult.geom = feature.geometry().constGet()->clone();
    pickResult.crs = vlayer->crs();
    break;
  }
  if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
  {
    renderer->stopRender( renderContext );
  }

  return pickResult;
}
