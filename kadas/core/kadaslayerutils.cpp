/***************************************************************************
    kadaslayerutils.cpp
    -------------------
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

#include <QCoreApplication>

#include <qgis/qgsmaplayer.h>
#include <qgis/qgsvectorlayer.h>

#include "kadas/core/kadaslayerutils.h"


QString KadasLayerUtils::lockedPropertyKey()
{
  return QStringLiteral( "kadas/locked" );
}

bool KadasLayerUtils::isLocked( const QgsMapLayer *layer )
{
  if ( !layer )
    return false;
  const QVariant locked = layer->customProperty( lockedPropertyKey() );
  if ( locked.isValid() )
    return locked.toBool();
  // Layers locked before the lock had a property of its own only carry what
  // locking did to them back then: they are not removable and, for vector
  // layers, read-only.
  if ( layer->flags().testFlag( QgsMapLayer::Removable ) )
    return false;
  const QgsVectorLayer *vl = qobject_cast<const QgsVectorLayer *>( layer );
  return !vl || vl->readOnly();
}

void KadasLayerUtils::setLocked( QgsMapLayer *layer, bool locked )
{
  if ( !layer )
    return;
  QgsMapLayer::LayerFlags flags = layer->flags();
  flags.setFlag( QgsMapLayer::Removable, !locked );
  layer->setFlags( flags );
  if ( QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( layer ) )
    vl->setReadOnly( locked );
  // Last, so that whoever follows the property finds the rest already in place.
  layer->setCustomProperty( lockedPropertyKey(), locked );
}

QString KadasLayerUtils::lockedMessage( const QgsMapLayer *layer )
{
  return QCoreApplication::translate( "KadasLayerUtils", "The layer \"%1\" is locked." ).arg( layer ? layer->name() : QString() );
}
