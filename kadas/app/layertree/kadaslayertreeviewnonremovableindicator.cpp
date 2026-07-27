/***************************************************************************
  kadaslayertreeviewnonremovableindicator.cpp
  --------------------------------------
  Date                 : July 2026
  Copyright            : (C) 2026 by Valentin Buira
  Email                : valentin at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslayertreeutils.h"
#include "qgslayertreeview.h"

#include "kadaslayertreeviewnonremovableindicator.h"

#include <QString>


using namespace Qt::StringLiterals;

/**
 * Forked from https://github.com/ValentinBuira/QGIS/blob/8c9806d/src/app/qgslayertreeviewnonremovableindicator.cpp 
 */
KadasLayerTreeViewNonRemovableIndicatorProvider::KadasLayerTreeViewNonRemovableIndicatorProvider( QgsLayerTreeView *view )
  : QgsLayerTreeViewIndicatorProvider( view )
{}

QString KadasLayerTreeViewNonRemovableIndicatorProvider::iconName( QgsMapLayer *layer )
{
  Q_UNUSED( layer )
  return u"/mIndicatorNonRemovable.svg"_s;
}

QString KadasLayerTreeViewNonRemovableIndicatorProvider::tooltipText( QgsMapLayer *layer )
{
  Q_UNUSED( layer )
  return tr( "Layer required by the project" );
}

bool KadasLayerTreeViewNonRemovableIndicatorProvider::acceptLayer( QgsMapLayer *layer )
{
  return !layer->flags().testFlag( QgsMapLayer::LayerFlag::Removable );
}

void KadasLayerTreeViewNonRemovableIndicatorProvider::connectSignals( QgsMapLayer *layer )
{
  QgsLayerTreeViewIndicatorProvider::connectSignals( layer );
  connect( layer, &QgsMapLayer::flagsChanged, this, &KadasLayerTreeViewNonRemovableIndicatorProvider::onLayerChanged );
}

void KadasLayerTreeViewNonRemovableIndicatorProvider::disconnectSignals( QgsMapLayer *layer )
{
  QgsLayerTreeViewIndicatorProvider::disconnectSignals( layer );
  disconnect( layer, &QgsMapLayer::flagsChanged, this, &KadasLayerTreeViewNonRemovableIndicatorProvider::onLayerChanged );
}
