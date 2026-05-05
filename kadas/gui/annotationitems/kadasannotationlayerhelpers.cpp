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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"


namespace
{
  const QString KEY_PREFIX = QStringLiteral( "kadas:item-meta:" );

  QString tooltipKey( const QString &itemId )
  {
    return KEY_PREFIX + itemId + QStringLiteral( ":tooltip" );
  }

  // Polymorphic accessors over the (currently 4) Kadas master types that
  // carry a shadow-ids list. Centralized here so callers don't sprinkle
  // dynamic_casts. Returns nullptr for items that aren't Kadas masters.
  QStringList masterShadowIds( const QgsAnnotationItem *item )
  {
    if ( const auto *m = dynamic_cast<const KadasRectangleAnnotationItem *>( item ) )
      return m->shadowIds();
    if ( const auto *m = dynamic_cast<const KadasCircleAnnotationItem *>( item ) )
      return m->shadowIds();
    if ( const auto *m = dynamic_cast<const KadasPinAnnotationItem *>( item ) )
      return m->shadowIds();
    if ( const auto *m = dynamic_cast<const KadasCoordCrossAnnotationItem *>( item ) )
      return m->shadowIds();
    return {};
  }

  bool setMasterShadowIds( QgsAnnotationItem *item, const QStringList &ids )
  {
    if ( auto *m = dynamic_cast<KadasRectangleAnnotationItem *>( item ) )
    {
      m->setShadowIds( ids );
      return true;
    }
    if ( auto *m = dynamic_cast<KadasCircleAnnotationItem *>( item ) )
    {
      m->setShadowIds( ids );
      return true;
    }
    if ( auto *m = dynamic_cast<KadasPinAnnotationItem *>( item ) )
    {
      m->setShadowIds( ids );
      return true;
    }
    if ( auto *m = dynamic_cast<KadasCoordCrossAnnotationItem *>( item ) )
    {
      m->setShadowIds( ids );
      return true;
    }
    return false;
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

void KadasAnnotationLayerHelpers::stripShadowsFromLayer( QgsAnnotationLayer *layer )
{
  if ( !layer )
    return;
  // Snapshot ids first to avoid iterator invalidation while we remove items.
  const QMap<QString, QgsAnnotationItem *> snapshot = layer->items();
  for ( auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it )
  {
    QgsAnnotationItem *master = it.value();
    const QStringList ids = masterShadowIds( master );
    if ( ids.isEmpty() )
      continue;
    for ( const QString &id : ids )
      layer->removeItem( id );
    setMasterShadowIds( master, {} );
  }
}

void KadasAnnotationLayerHelpers::prepareLayerForSave( QgsAnnotationLayer *layer )
{
  if ( !layer )
    return;

  // Drop any pre-existing shadows so repeated saves don't accumulate them.
  stripShadowsFromLayer( layer );

  auto *registry = KadasAnnotationControllerRegistry::instance();
  if ( !registry )
    return;

  const KadasAnnotationItemContext ctx( layer->crs(), QgsMapSettings(), layer );

  // Snapshot master ids/items first; addItem() during iteration would
  // mutate the underlying map.
  const QMap<QString, QgsAnnotationItem *> snapshot = layer->items();
  for ( auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it )
  {
    QgsAnnotationItem *master = it.value();
    if ( !master )
      continue;
    KadasAnnotationItemController *controller = registry->controllerFor( master->type() );
    if ( !controller )
      continue;
    const QList<QgsAnnotationItem *> shadows = controller->generateShadows( master, ctx );
    if ( shadows.isEmpty() )
      continue;
    QStringList shadowIds;
    shadowIds.reserve( shadows.size() );
    for ( QgsAnnotationItem *shadow : shadows )
    {
      const QString id = layer->addItem( shadow ); // takes ownership
      if ( !id.isEmpty() )
        shadowIds.append( id );
    }
    setMasterShadowIds( master, shadowIds );
  }
}
