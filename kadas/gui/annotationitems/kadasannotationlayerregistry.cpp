/***************************************************************************
    kadasannotationlayerregistry.cpp
    --------------------------------
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

#include <QDomDocument>
#include <QDomElement>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"


KadasAnnotationLayerRegistry::KadasAnnotationLayerRegistry()
{}

void KadasAnnotationLayerRegistry::init()
{
  connect( QgsProject::instance(), &QgsProject::cleared, instance(), &KadasAnnotationLayerRegistry::clear );
  connect( QgsProject::instance(), &QgsProject::readProject, instance(), &KadasAnnotationLayerRegistry::readFromProject );
  connect( QgsProject::instance(), &QgsProject::writeProject, instance(), &KadasAnnotationLayerRegistry::writeToProject );
}

void KadasAnnotationLayerRegistry::clear()
{
  mLayerIdMap.clear();
}

void KadasAnnotationLayerRegistry::readFromProject( const QDomDocument &doc )
{
  mLayerIdMap.clear();
  QDomElement el = doc.firstChildElement( QStringLiteral( "qgis" ) ).firstChildElement( QStringLiteral( "StandardAnnotationLayers" ) );
  if ( el.isNull() )
    return;
  const QDomNodeList items = el.elementsByTagName( QStringLiteral( "StandardAnnotationLayer" ) );
  for ( int i = 0, n = items.size(); i < n; ++i )
  {
    const QDomElement item = items.at( i ).toElement();
    const StandardLayer stdLayer = static_cast<StandardLayer>( item.attribute( QStringLiteral( "layer" ) ).toInt() );
    mLayerIdMap[stdLayer] = item.attribute( QStringLiteral( "layerId" ) );
  }
}

void KadasAnnotationLayerRegistry::writeToProject( QDomDocument &doc )
{
  QDomElement root = doc.firstChildElement( QStringLiteral( "qgis" ) );
  QDomElement el = doc.createElement( QStringLiteral( "StandardAnnotationLayers" ) );
  for ( auto it = mLayerIdMap.begin(), itEnd = mLayerIdMap.end(); it != itEnd; ++it )
  {
    QDomElement itemEl = doc.createElement( QStringLiteral( "StandardAnnotationLayer" ) );
    itemEl.setAttribute( QStringLiteral( "layer" ), static_cast<int>( it.key() ) );
    itemEl.setAttribute( QStringLiteral( "layerId" ), it.value() );
    el.appendChild( itemEl );
  }
  root.appendChild( el );
}

QgsAnnotationLayer *KadasAnnotationLayerRegistry::getOrCreateAnnotationLayer( StandardLayer layer )
{
  QgsAnnotationLayer *annoLayer = nullptr;
  if ( instance()->mLayerIdMap.contains( layer ) )
  {
    annoLayer = qobject_cast<QgsAnnotationLayer *>( QgsProject::instance()->mapLayer( instance()->mLayerIdMap[layer] ) );
  }

  if ( !annoLayer && standardLayerNames().contains( layer ) )
  {
    QgsAnnotationLayer::LayerOptions options( QgsProject::instance()->transformContext() );
    annoLayer = new QgsAnnotationLayer( standardLayerNames()[layer], options );
    // MilX/MSS content is always WGS84 (libmss IPC convention); other
    // standard layers default to web-mercator.
    const QString crsAuthid = ( layer == StandardLayer::MssLayer ) ? QStringLiteral( "EPSG:4326" ) : QStringLiteral( "EPSG:3857" );
    annoLayer->setCrs( QgsCoordinateReferenceSystem( crsAuthid ) );
    QgsProject::instance()->addMapLayer( annoLayer );
    instance()->mLayerIdMap[layer] = annoLayer->id();
  }
  return annoLayer;
}

const QMap<KadasAnnotationLayerRegistry::StandardLayer, QString> &KadasAnnotationLayerRegistry::standardLayerNames()
{
  static const QMap<StandardLayer, QString> names = {
    { StandardLayer::RedliningLayer, tr( "Redlining" ) },
    { StandardLayer::SymbolsLayer, tr( "Symbols" ) },
    { StandardLayer::PicturesLayer, tr( "Pictures" ) },
    { StandardLayer::PinsLayer, tr( "Pins" ) },
    { StandardLayer::RoutesLayer, tr( "Routes" ) },
    { StandardLayer::MssLayer, tr( "MSS" ) },
  };
  return names;
}

QList<QgsAnnotationLayer *> KadasAnnotationLayerRegistry::getAnnotationLayers()
{
  QList<QgsAnnotationLayer *> result;
  const QList<QgsMapLayer *> layers = QgsProject::instance()->mapLayers().values();
  for ( QgsMapLayer *layer : layers )
  {
    if ( QgsAnnotationLayer *al = qobject_cast<QgsAnnotationLayer *>( layer ) )
      result.append( al );
  }
  return result;
}

KadasAnnotationLayerRegistry *KadasAnnotationLayerRegistry::instance()
{
  static KadasAnnotationLayerRegistry registry;
  return &registry;
}
