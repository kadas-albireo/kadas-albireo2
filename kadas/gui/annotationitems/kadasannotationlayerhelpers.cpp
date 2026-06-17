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

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontext.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"


namespace
{
  const QString KEY_PREFIX = QStringLiteral( "kadas:item-meta:" );
  const QString ORPHAN_PREFIX = QStringLiteral( "kadas:orphan:" );

  QString tooltipKey( const QString &itemId )
  {
    return KEY_PREFIX + itemId + QStringLiteral( ":tooltip" );
  }

  QString orphanKey( const QString &masterId )
  {
    return ORPHAN_PREFIX + masterId;
  }

  // Drop every kadas:orphan:* side-channel record so repeated saves don't accumulate stale entries.
  void clearOrphanRecords( QgsAnnotationLayer *layer )
  {
    const QStringList keys = layer->customPropertyKeys();
    for ( const QString &key : keys )
    {
      if ( key.startsWith( ORPHAN_PREFIX ) )
        layer->removeCustomProperty( key );
    }
  }

  // Dispatch shadow-id access through the controller registry.
  KadasAnnotationItemController *controllerFor( const QgsAnnotationItem *item )
  {
    if ( !item )
      return nullptr;
    auto *registry = KadasAnnotationControllerRegistry::instance();
    if ( !registry )
      return nullptr;
    return registry->controllerFor( item->type() );
  }

  QStringList masterShadowIds( const QgsAnnotationItem *item )
  {
    KadasAnnotationItemController *c = controllerFor( item );
    return c ? c->shadowIds( item ) : QStringList();
  }

  void setMasterShadowIds( QgsAnnotationItem *item, const QStringList &ids )
  {
    if ( KadasAnnotationItemController *c = controllerFor( item ) )
      c->setShadowIds( item, ids );
  }
} // namespace


bool KadasAnnotationLayerHelpers::isParametricLayer( const QgsAnnotationLayer *layer )
{
  return layer && !layer->customProperty( QStringLiteral( "kadas/annotation-type" ) ).toString().isEmpty();
}

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
  // Capture (master, ids) first; removeItem() destroys items, invalidating the snapshot mid-iteration.
  struct Work
  {
      QgsAnnotationItem *master;
      QStringList ids;
  };
  QList<Work> work;
  const QMap<QString, QgsAnnotationItem *> snapshot = layer->items();
  for ( auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it )
  {
    QgsAnnotationItem *master = it.value();
    const QStringList ids = masterShadowIds( master );
    if ( ids.isEmpty() )
      continue;
    work.append( { master, ids } );
  }

  for ( const Work &w : work )
  {
    for ( const QString &id : w.ids )
      layer->removeItem( id );
    setMasterShadowIds( w.master, {} );
  }
}

void KadasAnnotationLayerHelpers::prepareLayerForSave( QgsAnnotationLayer *layer )
{
  if ( !layer )
    return;

  // Drop any pre-existing shadows so repeated saves don't accumulate them.
  stripShadowsFromLayer( layer );
  clearOrphanRecords( layer );

  auto *registry = KadasAnnotationControllerRegistry::instance();
  if ( !registry )
    return;

  const KadasAnnotationItemContext ctx( layer, QgsMapSettings() );

  // Snapshot first; addItem() during iteration would mutate the map.
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

    // Side-channel the master identity (type + shadow ids) as a layer custom
    // property: it survives a vanilla-QGIS round trip that drops the unknown
    // master item type but keeps the stock shadows, letting us rebuild on load.
    QStringList record;
    record << master->type() << shadowIds;
    layer->setCustomProperty( orphanKey( it.key() ), record.join( QLatin1Char( '|' ) ) );
  }
}

void KadasAnnotationLayerHelpers::reconstructOrphanCrosses( QgsAnnotationLayer *layer )
{
  if ( !layer )
    return;

  const QStringList keys = layer->customPropertyKeys();
  for ( const QString &key : keys )
  {
    if ( !key.startsWith( ORPHAN_PREFIX ) )
      continue;

    const QString masterId = key.mid( ORPHAN_PREFIX.length() );
    const QStringList record = layer->customProperty( key ).toString().split( QLatin1Char( '|' ), Qt::SkipEmptyParts );
    layer->removeCustomProperty( key ); // consume; prepareLayerForSave rewrites it on the next save

    if ( record.isEmpty() )
      continue;
    // Master survived the round trip (project opened straight in Kadas): the
    // regular shadow-strip path handles its shadows, nothing to reconstruct.
    if ( layer->item( masterId ) )
      continue;
    if ( record.first() != KadasCoordCrossAnnotationItem::itemTypeId() )
      continue;

    const QStringList shadowIds = record.mid( 1 );
    // Recover the (possibly QGIS-moved) position from the stock cross marker.
    bool found = false;
    QgsPointXY pos;
    for ( const QString &sid : shadowIds )
    {
      if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( layer->item( sid ) ) )
      {
        pos = marker->geometry();
        found = true;
        break;
      }
    }
    if ( !found )
      continue;

    layer->addItem( new KadasCoordCrossAnnotationItem( QgsPoint( pos.x(), pos.y() ) ) );
    for ( const QString &sid : shadowIds )
      layer->removeItem( sid );
  }
}
