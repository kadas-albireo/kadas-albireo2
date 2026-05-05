/***************************************************************************
    kadasannotationprojectintegration.cpp
    -------------------------------------
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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadasannotationprojectintegration.h"


namespace
{
  QList<QgsAnnotationLayer *> annotationLayers()
  {
    QList<QgsAnnotationLayer *> result;
    const QMap<QString, QgsMapLayer *> layers = QgsProject::instance()->mapLayers();
    for ( QgsMapLayer *layer : layers )
    {
      if ( auto *al = qobject_cast<QgsAnnotationLayer *>( layer ) )
        result.append( al );
    }
    // Also include the project main annotation layer.
    if ( auto *main = QgsProject::instance()->mainAnnotationLayer() )
    {
      if ( !result.contains( main ) )
        result.append( main );
    }
    return result;
  }
} // namespace


KadasAnnotationProjectIntegration::KadasAnnotationProjectIntegration( QObject *parent )
  : QObject( parent )
{
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasAnnotationProjectIntegration::onProjectRead );
}

void KadasAnnotationProjectIntegration::prepareForSave()
{
  for ( QgsAnnotationLayer *layer : annotationLayers() )
    KadasAnnotationLayerHelpers::prepareLayerForSave( layer );
}

void KadasAnnotationProjectIntegration::stripAfterSave()
{
  for ( QgsAnnotationLayer *layer : annotationLayers() )
    KadasAnnotationLayerHelpers::stripShadowsFromLayer( layer );
}

void KadasAnnotationProjectIntegration::onProjectRead( const QDomDocument & )
{
  // Defensive: a project written by an older Kadas session might still
  // contain shadow items if the save-side strip didn't run. Drop them now
  // so they don't appear as duplicates next to the masters.
  stripAfterSave();
}
