/***************************************************************************
    kadasitemlayermigration.cpp
    ---------------------------
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

#include <QList>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"
#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/kadasitemlayermigration.h"
#include "kadas/gui/mapitems/kadasmapitem.h"
#include "kadas/gui/milx/kadasmilxlayer.h"


int KadasItemLayerMigration::migrateProject( QgsProject *project )
{
  if ( !project )
    return 0;

  // Collect candidates first; we mutate the project map during the loop.
  QList<KadasItemLayer *> candidates;
  const QList<QgsMapLayer *> layers = project->mapLayers().values();
  for ( QgsMapLayer *layer : layers )
  {
    if ( auto *itemLayer = qobject_cast<KadasItemLayer *>( layer ) )
      candidates.append( itemLayer );
  }

  int migrated = 0;
  for ( KadasItemLayer *itemLayer : candidates )
  {
    const QMap<KadasItemLayer::ItemId, KadasMapItem *> &items = itemLayer->items();

    // Bail out if any item type has no annotationItem() override yet —
    // we will re-attempt on the next slice once the missing types
    // implement the override.
    bool allTranslatable = true;
    for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
    {
      const QList<QgsAnnotationItem *> probe = it.value()->annotationItems();
      if ( probe.isEmpty() )
      {
        QgsDebugMsgLevel( QStringLiteral( "Skipping migration of layer %1: item type %2 has no annotationItem() override yet" ).arg( itemLayer->name(), it.value()->metaObject()->className() ), 2 );
        allTranslatable = false;
        qDeleteAll( probe );
        break;
      }
      qDeleteAll( probe );
    }
    if ( !allTranslatable )
      continue;

    auto *annoLayer = KadasAnnotationLayerHelpers::createLayer( itemLayer->name(), itemLayer->crs() );

    // Carry over any per-layer overrides specific to legacy KadasMilxLayer
    // (override flag + symbol settings) onto the new annotation layer's
    // custom properties. The legacy layer never persisted these to XML,
    // so this only matters for projects migrated in-memory.
    if ( auto *milxLayer = qobject_cast<KadasMilxLayer *>( itemLayer ) )
    {
      KadasMilxLayerSettings::setOverrideEnabled( annoLayer, milxLayer->overrideMilxSymbolSettings() );
      if ( milxLayer->overrideMilxSymbolSettings() )
        KadasMilxLayerSettings::setLayerSettings( annoLayer, milxLayer->milxSymbolSettings() );
    }

    // Iterate by ascending KadasMapItem::zIndex() so the legacy paint
    // order is preserved when items happen to share a Kadas annotation
    // z-index bucket (tie-broken by item id otherwise).
    QList<KadasItemLayer::ItemId> orderedIds = items.keys();
    std::stable_sort( orderedIds.begin(), orderedIds.end(), [&]( KadasItemLayer::ItemId a, KadasItemLayer::ItemId b ) { return items[a]->zIndex() < items[b]->zIndex(); } );

    for ( KadasItemLayer::ItemId id : orderedIds )
    {
      KadasMapItem *legacy = items[id];
      const QList<QgsAnnotationItem *> annos = legacy->annotationItems( annoLayer->crs() );
      for ( QgsAnnotationItem *anno : annos )
      {
        const QString newId = annoLayer->addItem( anno );
        if ( !legacy->tooltip().isEmpty() )
          KadasAnnotationLayerHelpers::setTooltip( annoLayer, newId, legacy->tooltip() );
        if ( !legacy->editor().isEmpty() )
          KadasAnnotationLayerHelpers::setEditorName( annoLayer, newId, legacy->editor() );
      }
    }

    project->addMapLayer( annoLayer );
    project->removeMapLayer( itemLayer->id() );
    ++migrated;
  }

  return migrated;
}
