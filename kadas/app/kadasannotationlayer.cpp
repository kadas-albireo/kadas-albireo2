/***************************************************************************
    kadasannotationlayer.cpp
    ------------------------
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

#include "kadas/app/kadasannotationlayer.h"

#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QMap>
#include <QPair>

#include <qgis/qgslayertree.h>
#include <qgis/qgslayertreegroup.h>
#include <qgis/qgslayertreelayer.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsreadwritecontext.h>


KadasAnnotationLayer::KadasAnnotationLayer( const QString &name, const QString &kadasType )
  : QgsAnnotationLayer( name, QgsAnnotationLayer::LayerOptions( QgsProject::instance()->transformContext() ) )
{
  setCustomProperty( QStringLiteral( "kadas/annotation-type" ), kadasType );
}

QHash<QString, KadasAnnotationLayer::Factory> &KadasAnnotationLayer::registry()
{
  static QHash<QString, Factory> r;
  return r;
}

void KadasAnnotationLayer::registerType( const QString &kadasType, Factory factory )
{
  registry().insert( kadasType, std::move( factory ) );
}

void KadasAnnotationLayer::promoteAll( QgsProject *project )
{
  if ( !project )
    return;

  // QGIS serializes annotation layers as plain type="annotation"; rebuild Kadas subclasses from the kadas/annotation-type marker.
  QList<QPair<QgsAnnotationLayer *, KadasAnnotationLayer *>> swaps;
  const QMap<QString, QgsMapLayer *> layers = project->mapLayers();
  for ( QgsMapLayer *layer : layers )
  {
    auto *plain = qobject_cast<QgsAnnotationLayer *>( layer );
    if ( !plain || qobject_cast<KadasAnnotationLayer *>( plain ) )
      continue;
    const QString kadasType = plain->customProperty( QStringLiteral( "kadas/annotation-type" ) ).toString();
    const auto it = registry().constFind( kadasType );
    if ( it == registry().constEnd() )
      continue;

    // XML round-trip carries customProperties / items / CRS / opacity / name into the subclass.
    QDomDocument doc;
    QDomElement layerEl = doc.createElement( QStringLiteral( "maplayer" ) );
    doc.appendChild( layerEl );
    QgsReadWriteContext ctx;
    if ( !plain->writeLayerXml( layerEl, doc, ctx ) )
      continue;

    KadasAnnotationLayer *promoted = ( *it )( plain->name() );
    if ( !promoted )
      continue;
    if ( !promoted->readLayerXml( layerEl, ctx ) )
    {
      delete promoted;
      continue;
    }
    swaps.append( qMakePair( plain, promoted ) );
  }

  for ( const auto &swap : swaps )
  {
    QgsAnnotationLayer *plain = swap.first;
    KadasAnnotationLayer *promoted = swap.second;
    const QString id = plain->id();
    QgsLayerTreeLayer *node = project->layerTreeRoot()->findLayer( id );
    QgsLayerTreeGroup *parentGroup = node ? qobject_cast<QgsLayerTreeGroup *>( node->parent() ) : nullptr;
    const int row = ( parentGroup && node ) ? parentGroup->children().indexOf( node ) : -1;
    const bool visible = node ? node->isVisible() : true;
    const bool expanded = node ? node->isExpanded() : false;

    project->removeMapLayer( id );
    promoted->setId( id ); // keep references in layer-tree, project files, etc.
    project->addMapLayer( promoted, false );
    if ( parentGroup )
    {
      QgsLayerTreeLayer *newNode = parentGroup->insertLayer( row, promoted );
      newNode->setItemVisibilityChecked( visible );
      newNode->setExpanded( expanded );
    }
  }
}
