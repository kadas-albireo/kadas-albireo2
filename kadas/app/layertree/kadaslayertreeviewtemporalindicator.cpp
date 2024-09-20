/***************************************************************************
  kadaslayertreeviewtemporalindicator.cpp
  --------------------------------------
  Date                 : July 2024
  Copyright            : (C) 2024 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAction>

#include "qgslayertreeview.h"
#include "qgslayertree.h"
#include "qgsmaplayertemporalproperties.h"

#include "kadaslayertreeviewtemporalindicator.h"
#include "kadastemporalcontroller.h"

KadasLayerTreeViewTemporalIndicator::KadasLayerTreeViewTemporalIndicator(QgsLayerTreeView *view , KadasTemporalController *kadasTemporalController)
  : QgsLayerTreeViewIndicatorProvider( view )
  , mKadasTemporalController( kadasTemporalController )
{
}

void KadasLayerTreeViewTemporalIndicator::connectSignals( QgsMapLayer *layer )
{
  if ( !layer || !layer->temporalProperties() )
    return;

  connect( layer->temporalProperties(), &QgsMapLayerTemporalProperties::changed, this, [ this, layer ]( ) { this->onLayerChanged( layer ); } );
}

void KadasLayerTreeViewTemporalIndicator::onIndicatorClicked( const QModelIndex &index )
{
  QgsLayerTreeNode *node = mLayerTreeView->index2node( index );
  if ( !QgsLayerTree::isLayer( node ) )
    return;

  QgsMapLayer *layer = QgsLayerTree::toLayer( node )->layer();
  if ( !layer )
    return;

  switch ( layer->type() )
  {
    case Qgis::LayerType::Raster:
    case Qgis::LayerType::Mesh:
    case Qgis::LayerType::Vector:
      mKadasTemporalController->setVisible( !mKadasTemporalController->isVisible() );
      break;
    case Qgis::LayerType::Plugin:
    case Qgis::LayerType::VectorTile:
    case Qgis::LayerType::Annotation:
    case Qgis::LayerType::PointCloud:
    case Qgis::LayerType::Group:
    case Qgis::LayerType::TiledScene:
      return;
  }
}

bool KadasLayerTreeViewTemporalIndicator::acceptLayer( QgsMapLayer *layer )
{
  if ( !layer )
    return false;
  if ( layer->temporalProperties() &&
       layer->temporalProperties()->isActive() )
    return true;
  return false;
}

QString KadasLayerTreeViewTemporalIndicator::iconName( QgsMapLayer * )
{
  return QStringLiteral( "/mIndicatorTemporal.svg" );
}

QString KadasLayerTreeViewTemporalIndicator::tooltipText( QgsMapLayer * )
{
  return tr( "<b>Temporal layer</b>" );
}

void KadasLayerTreeViewTemporalIndicator::onLayerChanged( QgsMapLayer *layer )
{
  if ( !layer )
    return;
  updateLayerIndicator( layer );
}
