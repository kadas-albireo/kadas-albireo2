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
#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsrenderer.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspallabeling.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"

#include <limits>

KadasFeaturePicker::PickResult KadasFeaturePicker::pick( const QgsMapCanvas *canvas, const QgsPointXY &mapPos, Qgis::GeometryType geomType, KadasFeaturePicker::PickObjective pickObjective )
{
  PickResult pickResult;

  for ( QgsMapLayer *layer : canvas->layers() )
  {
    if ( qobject_cast<QgsAnnotationLayer *>( layer ) )
    {
      pickResult = pickAnnotationLayer( static_cast<QgsAnnotationLayer *>( layer ), canvas, mapPos );
    }
    else if ( qobject_cast<QgsVectorLayer *>( layer ) && pickObjective != KadasFeaturePicker::PickObjective::PICK_OBJECTIVE_TOOLTIP )
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

KadasFeaturePicker::PickResult KadasFeaturePicker::pickAnnotationLayer( QgsAnnotationLayer *layer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos )
{
  PickResult pickResult;

  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas->mapSettings() );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  const double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();

  const QgsRectangle mapBounds( mapPos.x() - radiusmu, mapPos.y() - radiusmu, mapPos.x() + radiusmu, mapPos.y() + radiusmu );
  const QgsRectangle layerBounds = canvas->mapSettings().mapToLayerCoordinates( layer, mapBounds );

  QgsFeedback feedback;
  const QStringList hits = layer->itemsInBounds( layerBounds, renderContext, &feedback );
  if ( hits.isEmpty() )
  {
    return pickResult;
  }

  // QgsAnnotationLayer::itemsInBounds is a bounding-box test only, so
  // long diagonal lines / U-shaped polygons would falsely capture clicks
  // anywhere inside their AABB. Refine by asking each candidate's
  // controller for an edit context at the click position: a candidate is
  // a real hit only if its controller reports a valid context (vertex,
  // handle, or actual body containment). Among real hits, prefer the
  // highest zIndex, then smallest bbox area. If no candidate has a
  // controller (or none reports a hit), fall back to the previous
  // bbox-only behavior so identify/tooltip still resolve something.
  KadasAnnotationItemContext ctx( layer, canvas->mapSettings() );
  QString best;
  int bestZ = std::numeric_limits<int>::min();
  double bestArea = std::numeric_limits<double>::infinity();
  QString fallback;
  int fallbackZ = std::numeric_limits<int>::min();
  double fallbackArea = std::numeric_limits<double>::infinity();
  for ( const QString &id : hits )
  {
    QgsAnnotationItem *cand = layer->item( id );
    if ( !cand )
      continue;
    const QgsRectangle bb = cand->boundingBox();
    const double area = bb.width() * bb.height();
    const int z = cand->zIndex();

    if ( z > fallbackZ || ( z == fallbackZ && area < fallbackArea ) )
    {
      fallbackZ = z;
      fallbackArea = area;
      fallback = id;
    }

    KadasAnnotationItemController *cc = KadasAnnotationControllerRegistry::instance()->controllerFor( cand->type() );
    if ( !cc )
      continue;
    const KadasEditContext ec = cc->getEditContext( cand, mapPos, ctx );
    if ( !ec.isValid() )
      continue;
    if ( z > bestZ || ( z == bestZ && area < bestArea ) )
    {
      bestZ = z;
      bestArea = area;
      best = id;
    }
  }

  pickResult.annotationLayer = layer;
  pickResult.annotationItemId = best.isEmpty() ? fallback : best;
  pickResult.crs = layer->crs();
  return pickResult;
}

KadasFeaturePicker::PickResult KadasFeaturePicker::pickVectorLayer( QgsVectorLayer *vlayer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos, Qgis::GeometryType geomType )
{
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas->mapSettings() );
  double radiusmm = QgsSettings().value( "/Map/searchRadiusMM", Qgis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : Qgis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );

  PickResult pickResult;

  QgsFeatureList features;
  if ( geomType != Qgis::GeometryType::Unknown && vlayer->geometryType() != Qgis::GeometryType::Unknown && vlayer->geometryType() != geomType )
  {
    return pickResult;
  }
  if ( vlayer->hasScaleBasedVisibility() && ( vlayer->maximumScale() > canvas->mapSettings().scale() || vlayer->minimumScale() <= canvas->mapSettings().scale() ) )
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
#if _QGIS_VERSION_INT >= 33500
  QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( Qgis::FeatureRequestFlag::ExactIntersect ) );
#else
  QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
#endif
  QgsFeature feature;
  while ( fit.nextFeature( feature ) )
  {
    if ( filteredRendering && !renderer->willRenderFeature( feature, renderContext ) )
    {
      continue;
    }
    if ( geomType != Qgis::GeometryType::Unknown && feature.geometry().type() != geomType )
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
