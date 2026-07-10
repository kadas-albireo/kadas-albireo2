/***************************************************************************
    kadasannotationitemcontroller.cpp
    ---------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>
#include <limits>

#include <QGuiApplication>
#include <QPaintDevice>
#include <QPainter>
#include <QScreen>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgscurve.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgsfillsymbol.h>
#include <qgis/qgsfillsymbollayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsunittypes.h>

#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"


void KadasAnnotationItemController::populateContextMenu( QgsAnnotationItem *, QMenu *, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext & )
{}

void KadasAnnotationItemController::onDoubleClick( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{}

QgsGeometry KadasAnnotationItemController::representativeGeometry( const QgsAnnotationItem *item, const KadasAnnotationItemContext & ) const
{
  if ( !item )
    return QgsGeometry();
  if ( const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( item ) )
    return line->geometry() ? QgsGeometry( line->geometry()->clone() ) : QgsGeometry();
  if ( const auto *poly = dynamic_cast<const QgsAnnotationPolygonItem *>( item ) )
    return poly->geometry() ? QgsGeometry( poly->geometry()->clone() ) : QgsGeometry();
  const QgsRectangle bb = item->boundingBox();
  if ( bb.isEmpty() )
    return QgsGeometry();
  return QgsGeometry::fromRect( bb );
}

KadasAnnotationPreviewStyle KadasAnnotationItemController::previewStyle( const QgsAnnotationItem *item ) const
{
  KadasAnnotationPreviewStyle style;
  if ( const auto *line = dynamic_cast<const QgsAnnotationLineItem *>( item ) )
  {
    if ( const QgsLineSymbol *sym = line->symbol() )
    {
      style.strokeColor = sym->color();
      style.secondaryColor = QColor();
      if ( const auto *sl = dynamic_cast<const QgsSimpleLineSymbolLayer *>( sym->symbolLayer( 0 ) ) )
      {
        style.width = sl->width();
        style.widthUnit = sl->widthUnit();
        // Mirror the dash pattern so a dashed line does not preview as solid.
        style.lineStyle = sl->penStyle();
      }
      else
      {
        style.width = sym->width();
        style.widthUnit = Qgis::RenderUnit::Millimeters;
      }
    }
  }
  else if ( const auto *poly = dynamic_cast<const QgsAnnotationPolygonItem *>( item ) )
  {
    if ( const QgsFillSymbol *sym = poly->symbol() )
    {
      style.fillColor = sym->color();
      style.secondaryColor = QColor();
      if ( const auto *sl = dynamic_cast<const QgsSimpleFillSymbolLayer *>( sym->symbolLayer( 0 ) ) )
      {
        style.strokeColor = sl->strokeColor();
        style.width = sl->strokeWidth();
        style.widthUnit = sl->strokeWidthUnit();
        // Mirror the fill pattern so a hatched polygon does not preview as a solid block.
        style.brushStyle = sl->brushStyle();
        // Mirror the dash pattern so a dashed outline does not preview as solid.
        style.lineStyle = sl->strokeStyle();
      }
    }
  }
  return style;
}

bool KadasAnnotationItemController::hitTest( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  if ( !item )
    return false;
  return toMapRect( item->boundingBox(), ctx ).contains( pos );
}

QPair<QgsPointXY, double> KadasAnnotationItemController::closestPoint( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  if ( !item )
    return { pos, std::numeric_limits<double>::max() };
  const QgsPointXY centerItem = item->boundingBox().center();
  const QgsPointXY centerMap = toMapPos( centerItem, ctx );
  const QgsPointXY cp( centerMap.x(), centerMap.y() );
  return { cp, std::hypot( cp.x() - pos.x(), cp.y() - pos.y() ) };
}

bool KadasAnnotationItemController::intersects( const QgsAnnotationItem *item, const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx, bool contains ) const
{
  if ( !item )
    return false;
  const QgsRectangle itemBounds = item->boundingBox();
  const QgsRectangle itemRect = toItemRect( mapRect, ctx );
  return contains ? itemRect.contains( itemBounds ) : itemRect.intersects( itemBounds );
}

// ----- Transform helpers ---------------------------------------------------

QgsPointXY KadasAnnotationItemController::toMapPos( const QgsPointXY &itemPos, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() ).transform( itemPos );
}

QgsPointXY KadasAnnotationItemController::toItemPos( const QgsPointXY &mapPos, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY p = QgsCoordinateTransform( ctx.mapSettings().destinationCrs(), ctx.itemCrs(), ctx.mapSettings().transformContext() ).transform( mapPos );
  return QgsPointXY( p.x(), p.y() );
}

QgsRectangle KadasAnnotationItemController::toItemRect( const QgsRectangle &mapRect, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.mapSettings().destinationCrs(), ctx.itemCrs(), ctx.mapSettings().transformContext() ).transform( mapRect );
}

QgsRectangle KadasAnnotationItemController::toMapRect( const QgsRectangle &itemRect, const KadasAnnotationItemContext &ctx )
{
  return QgsCoordinateTransform( ctx.itemCrs(), ctx.mapSettings().destinationCrs(), ctx.mapSettings().transformContext() ).transform( itemRect );
}

double KadasAnnotationItemController::pickTolSqr( const KadasAnnotationItemContext &ctx )
{
  const double mupp = ctx.mapSettings().mapUnitsPerPixel();
  return 25 * mupp * mupp;
}

QgsPointXY KadasAnnotationItemController::centroidMap( const QgsAbstractGeometry *geom, const KadasAnnotationItemContext &ctx )
{
  if ( !geom )
    return QgsPointXY();
  // Rotate around the geometric centroid (centre of mass), not the bounding-box
  // centre: the latter drifts off the visual centre for asymmetric shapes and
  // makes the item appear to swing rather than spin in place.
  const QgsGeometry g( geom->clone() );
  const QgsPointXY centroidItem = g.centroid().asPoint();
  return toMapPos( centroidItem, ctx );
}

// ----- Measurement formatting helpers --------------------------------------

QString KadasAnnotationItemController::formatLengthMeters( double meters )
{
  const int decimals = QgsSettings().value( QStringLiteral( "/kadas/measure_decimals" ), "2" ).toInt();
  return QgsUnitTypes::formatDistance( meters, decimals, Qgis::DistanceUnit::Meters );
}

QString KadasAnnotationItemController::formatAreaSquareMeters( double sqMeters )
{
  const int decimals = QgsSettings().value( QStringLiteral( "/kadas/measure_decimals" ), "2" ).toInt();
  if ( sqMeters >= 1000000.0 )
    return QStringLiteral( "%1 km²" ).arg( sqMeters / 1000000.0, 0, 'f', decimals );
  return QStringLiteral( "%1 m²" ).arg( sqMeters, 0, 'f', decimals );
}

double KadasAnnotationItemController::outputDpiScale( const QgsRenderContext &context )
{
  const QScreen *screen = QGuiApplication::primaryScreen();
  const double screenDpi = screen ? screen->logicalDotsPerInchX() : 96.0;
  if ( !context.painter() || !context.painter()->device() || screenDpi <= 0.0 )
    return 1.0;
  return static_cast<double>( context.painter()->device()->logicalDpiX() ) / screenDpi;
}
