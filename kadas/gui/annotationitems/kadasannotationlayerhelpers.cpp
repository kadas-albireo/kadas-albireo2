/***************************************************************************
    kadasannotationlayerhelpers.cpp
    -------------------------------
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

#include <QSet>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"


namespace
{
  const QString KEY_PREFIX = QStringLiteral( "kadas:item-meta:" );

  QString tooltipKey( const QString &itemId )
  {
    return KEY_PREFIX + itemId + QStringLiteral( ":tooltip" );
  }
} // namespace


QString KadasAnnotationLayerHelpers::tooltip( const QgsAnnotationLayer *layer, const QString &itemId )
{
  if ( !layer || itemId.isEmpty() )
    return QString();
  return layer->customProperty( tooltipKey( itemId ) ).toString();
}

void KadasAnnotationLayerHelpers::setTooltip( QgsAnnotationLayer *layer, const QString &itemId, const QString &tooltip )
{
  if ( !layer || itemId.isEmpty() )
    return;
  const QString key = tooltipKey( itemId );
  if ( tooltip.isEmpty() )
    layer->removeCustomProperty( key );
  else
    layer->setCustomProperty( key, tooltip );
}

void KadasAnnotationLayerHelpers::clearMetadata( QgsAnnotationLayer *layer, const QString &itemId )
{
  if ( !layer || itemId.isEmpty() )
    return;
  const QString prefix = KEY_PREFIX + itemId + QLatin1Char( ':' );
  const QStringList keys = layer->customPropertyKeys();
  for ( const QString &key : keys )
  {
    if ( key.startsWith( prefix ) )
      layer->removeCustomProperty( key );
  }
}

QStringList KadasAnnotationLayerHelpers::itemsWithMetadata( const QgsAnnotationLayer *layer )
{
  if ( !layer )
    return {};
  QStringList result;
  QSet<QString> seen;
  const QStringList keys = layer->customPropertyKeys();
  for ( const QString &key : keys )
  {
    if ( !key.startsWith( KEY_PREFIX ) )
      continue;
    const QString remainder = key.mid( KEY_PREFIX.size() );
    const int sep = remainder.indexOf( QLatin1Char( ':' ) );
    if ( sep <= 0 )
      continue;
    const QString itemId = remainder.left( sep );
    if ( !seen.contains( itemId ) )
    {
      seen.insert( itemId );
      result.append( itemId );
    }
  }
  return result;
}

QgsAnnotationLayer *KadasAnnotationLayerHelpers::createLayer( const QString &name, const QgsCoordinateReferenceSystem &preferredCrs )
{
  QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
  auto *layer = new QgsAnnotationLayer( name, options );
  QgsCoordinateReferenceSystem crs = preferredCrs;
  if ( !crs.isValid() )
    crs = QgsProject::instance()->crs();
  if ( !crs.isValid() )
    crs = QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) );
  layer->setCrs( crs );
  return layer;
}
