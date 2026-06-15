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
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"

#include <limits>

KadasFeaturePicker::PickResult KadasFeaturePicker::pick( const QgsMapCanvas *canvas, const QgsPointXY &mapPos, Qgis::GeometryType geomType, KadasFeaturePicker::PickObjective pickObjective )
{
  PickResult pickResult;

  for ( QgsMapLayer *layer : canvas->layers() )
  {
    if ( qobject_cast<QgsAnnotationLayer *>( layer ) )
    {
      pickResult = pickAnnotationLayer( static_cast<QgsAnnotationLayer *>( layer ), canvas, mapPos, geomType );
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

KadasFeaturePicker::PickResult KadasFeaturePicker::pickAnnotationLayer( QgsAnnotationLayer *layer, const QgsMapCanvas *canvas, const QgsPointXY &mapPos, Qgis::GeometryType geomType )
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
  // controller for an edit context at the click position and rank the
  // valid hits using rankAnnotationCandidates (precision → z → area).
  // A controller that reports no hit means "genuinely missed" — skip
  // such candidates entirely (otherwise a right-click in empty space
  // inside an item's AABB would falsely pick it). Only items WITHOUT a
  // registered controller fall back to a bbox-only candidate so
  // identify/tooltip still resolve something for unknown item types.
  KadasAnnotationItemContext ctx( layer, canvas->mapSettings() );
  QList<AnnotationPickCandidate> precise;
  QList<AnnotationPickCandidate> fallback;
  for ( const QString &id : hits )
  {
    QgsAnnotationItem *cand = layer->item( id );
    if ( !cand )
      continue;
    const QgsRectangle bb = cand->boundingBox();
    AnnotationPickCandidate c;
    c.layer = layer;
    c.itemId = id;
    c.zIndex = cand->zIndex();
    c.bboxArea = bb.width() * bb.height();

    KadasAnnotationItemController *cc = KadasAnnotationControllerRegistry::instance()->controllerFor( cand->type() );
    if ( cc )
    {
      const KadasEditContext ec = cc->getEditContext( cand, mapPos, ctx );
      if ( !ec.isValid() )
        continue;
      // When a specific geometry type is requested (e.g. the measure tool
      // picks lines or polygons), skip items whose representative geometry
      // does not match, so a click prefers the right kind of shape.
      if ( geomType != Qgis::GeometryType::Unknown && cc->representativeGeometry( cand, ctx ).type() != geomType )
        continue;
      c.precision = ec.precision;
      precise.append( c );
    }
    else
    {
      // No controller for this item type: bbox-only fallback. Such items
      // carry no usable geometry, so only offer them for untyped picks
      // (identify / tooltip).
      if ( geomType != Qgis::GeometryType::Unknown )
        continue;
      c.precision = KadasEditContext::HitPrecision::Body;
      fallback.append( c );
    }
  }

  const QList<AnnotationPickCandidate> &list = !precise.isEmpty() ? precise : fallback;
  const int bestIdx = rankAnnotationCandidates( list );
  if ( bestIdx < 0 )
    return pickResult;

  pickResult.annotationLayer = list[bestIdx].layer;
  // Parametric layers (bullseye, guide grid, ...) own exactly one logical
  // object whose items are auto-generated from a configuration. The
  // per-item refinement above still decides WHETHER the click hit the
  // object (ring / grid line / label), but the pick is reported at layer
  // level (empty item id) so consumers treat the layer atomically instead
  // of exposing the generated child items individually.
  if ( !KadasAnnotationLayerHelpers::isParametricLayer( list[bestIdx].layer ) )
  {
    pickResult.annotationItemId = list[bestIdx].itemId;
    // Expose the item geometry so geometry consumers (measure, min/max, ...)
    // can use an annotation just like a vector feature. Geometry is in the
    // layer CRS, matching pickResult.crs below.
    if ( QgsAnnotationItem *bestItem = layer->item( list[bestIdx].itemId ) )
    {
      if ( KadasAnnotationItemController *cc = KadasAnnotationControllerRegistry::instance()->controllerFor( bestItem->type() ) )
      {
        const QgsGeometry g = cc->representativeGeometry( bestItem, ctx );
        if ( !g.isEmpty() )
          pickResult.geom = g.constGet()->clone();
      }
    }
  }
  pickResult.crs = layer->crs();
  return pickResult;
}

int KadasFeaturePicker::rankAnnotationCandidates( const QList<AnnotationPickCandidate> &candidates )
{
  if ( candidates.isEmpty() )
    return -1;
  int bestIdx = 0;
  for ( int i = 1; i < candidates.size(); ++i )
  {
    const AnnotationPickCandidate &c = candidates.at( i );
    const AnnotationPickCandidate &b = candidates.at( bestIdx );
    // Precision (Precise > Body) is the primary key, then highest z,
    // then smallest bbox area.
    if ( c.precision > b.precision || ( c.precision == b.precision && c.zIndex > b.zIndex ) || ( c.precision == b.precision && c.zIndex == b.zIndex && c.bboxArea < b.bboxArea ) )
    {
      bestIdx = i;
    }
  }
  return bestIdx;
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
