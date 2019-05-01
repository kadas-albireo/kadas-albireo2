/***************************************************************************
    kadasredlininglayer.cpp
    -----------------------
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

#include <qgis/qgsellipsesymbollayer.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgssymbollayer.h>
#include <qgis/qgswkbtypes.h>

#include "kadasredliningrenderer.h"

// HACK to get public access to protected _getXXX methods
class KadasSymbol : public QgsSymbol
{
public:
  static QPointF getPoint( QgsRenderContext &context, const QgsPoint &point ) {
    return _getPoint(context, point);
  }
  static QPolygonF getLineString( QgsRenderContext &context, const QgsCurve &curve, bool clipToExtent = true )
  {
    return _getLineString(context, curve, clipToExtent);
  }
  static QPolygonF getPolygonRing( QgsRenderContext &context, const QgsCurve &curve, bool clipToExtent, bool isExteriorRing = false, bool correctRingOrientation = false )
  {
    return _getPolygonRing(context, curve, clipToExtent, isExteriorRing);
  }
  static void getPolygon( QPolygonF &pts, QList<QPolygonF> &holes, QgsRenderContext &context, const QgsPolygon &polygon, bool clipToExtent = true, bool correctRingOrientation = false )
  {
    return _getPolygon(pts, holes, context, polygon, clipToExtent, correctRingOrientation);
  }
};

KadasRedliningRenderer::KadasRedliningRenderer()
    : QgsFeatureRenderer( "redliningSymbol" )
    , mMarkerSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << new QgsEllipseSymbolLayer() ) )
    , mLineSymbol( new QgsLineSymbol() )
    , mFillSymbol( new QgsFillSymbol() )
{
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyFillColor, QgsProperty::fromField( "fill" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyFillStyle, QgsProperty::fromField( "fill_style" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeColor, QgsProperty::fromField( "outline" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeStyle, QgsProperty::fromField( "outline_style" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeWidth, QgsProperty::fromExpression( "\"size\" / 4" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyAngle, QgsProperty::fromExpression( "eval(regexp_substr(\"flags\",'r=([^,]+)'))" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyName, QgsProperty::fromExpression( "regexp_substr(\"flags\",'symbol=(\\\\w+)')" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyHeight, QgsProperty::fromExpression( "2 * \"size\"" ) );
  mMarkerSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyWidth, QgsProperty::fromExpression( "2 * \"size\"" ) );

  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeColor, QgsProperty::fromField("outline") );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeStyle, QgsProperty::fromField("outline_style") );
  mLineSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeWidth, QgsProperty::fromExpression("\"size\" / 4") );

  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyFillColor, QgsProperty::fromField( "fill" ) );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyFillStyle, QgsProperty::fromField( "fill_style" ) );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeColor, QgsProperty::fromField( "outline" ) );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeStyle,  QgsProperty::fromField("outline_style") );
  mFillSymbol->symbolLayers().front()->setDataDefinedProperty( QgsSymbolLayer::PropertyStrokeWidth, QgsProperty::fromExpression("\"size\" / 4") );
}

QgsSymbol* KadasRedliningRenderer::originalSymbolForFeature( const QgsFeature& feature, QgsRenderContext &context) const
{
  switch ( QgsWkbTypes::flatType( QgsWkbTypes::singleType( feature.geometry().get()->wkbType() ) ) )
  {
    case QgsWkbTypes::Point:
      return mMarkerSymbol.data();
    case QgsWkbTypes::LineString:
    case QgsWkbTypes::CircularString:
    case QgsWkbTypes::CompoundCurve:
      return mLineSymbol.data();
    case QgsWkbTypes::Polygon:
    case QgsWkbTypes::CurvePolygon:
      return mFillSymbol.data();
    default:
      return 0;
  }
  return 0;
}

void KadasRedliningRenderer::startRender( QgsRenderContext& context, const QgsFields& fields )
{
  mMarkerSymbol->startRender( context, fields );
  mLineSymbol->startRender( context, fields );
  mFillSymbol->startRender( context, fields );
}

bool KadasRedliningRenderer::renderFeature( const QgsFeature& feature, QgsRenderContext& context, int layer, bool selected, bool drawVertexMarker )
{
  if ( feature.geometry().isEmpty() )
  {
    return false;
  }
  // Don't draw features whose text attribute is set - they are drawn as labels only
  if ( feature.geometry().type() == QgsWkbTypes::PointGeometry && !feature.attribute( "text" ).toString().isEmpty() && !feature.attribute( "flags" ).toString().contains( "symbol=" ) )
  {
    return true;
  }
  QgsAbstractGeometry* geom = feature.geometry().get();
  QgsAbstractGeometry* segmentized = nullptr;

  //convert curve types to normal point/line/polygon ones
  switch ( QgsWkbTypes::flatType( geom->wkbType() ) )
  {
    case QgsWkbTypes::CurvePolygon:
    case QgsWkbTypes::CircularString:
    case QgsWkbTypes::CompoundCurve:
    case QgsWkbTypes::MultiSurface:
    case QgsWkbTypes::MultiCurve:
    {
      segmentized = geom->segmentize();
      geom = segmentized;
    }
    default:
      break;
  }
  int wkbSize;

  switch ( QgsWkbTypes::flatType( geom->wkbType() ) )
  {
    case QgsWkbTypes::Point:
    {
      QPointF pt = KadasSymbol::getPoint( context, *static_cast<QgsPoint*>( geom ) );
      mMarkerSymbol->renderPoint( pt, &feature, context, layer, selected );
      break;
    }
    case QgsWkbTypes::LineString:
    {
      QPolygonF pts = KadasSymbol::getLineString( context, *static_cast<QgsLineString*>( geom ) );
      mLineSymbol->renderPolyline( pts, &feature, context, layer, selected );

      if ( drawVertexMarker )
        drawVertexMarkers( feature.geometry().get(), context );
      break;
    }
    case QgsWkbTypes::Polygon:
    {
      QPolygonF pts;
      QList<QPolygonF> holes;
      KadasSymbol::getPolygon( pts, holes, context, *static_cast<QgsPolygon*>(geom) );
      mFillSymbol->renderPolygon( pts, ( holes.count() ? &holes : NULL ), &feature, context, layer, selected );

      if ( drawVertexMarker )
        drawVertexMarkers( feature.geometry().get(), context );
      break;
    }
    default:
      break;
  }
  delete segmentized;
  return true;
}

void KadasRedliningRenderer::stopRender( QgsRenderContext& context )
{
  mMarkerSymbol->stopRender( context );
  mLineSymbol->stopRender( context );
  mFillSymbol->stopRender( context );
}

void KadasRedliningRenderer::drawVertexMarkers( QgsAbstractGeometry *geom, QgsRenderContext& context )
{
  const QgsMapToPixel& mtp = context.mapToPixel();

  QgsPoint vertexPoint;
  QgsVertexId vertexId;
  double x, y, z;
  while ( geom->nextVertex( vertexId, vertexPoint ) )
  {
    //transform
    x = vertexPoint.x(); y = vertexPoint.y(); z = vertexPoint.z();
    context.coordinateTransform().transformInPlace( x, y, z );
    mtp.transformInPlace( x, y );
    renderVertexMarker( QPointF( x, y ), context );
  }
}
