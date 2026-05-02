/***************************************************************************
    kadasmilxlayersettings.cpp
    --------------------------
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

#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"

#include <qgis/qgsexpressioncontext.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

namespace
{
  constexpr const char *kOverride = "kadas/milx/override";
  constexpr const char *kSymbolSize = "kadas/milx/symbolSize";
  constexpr const char *kLineWidth = "kadas/milx/lineWidth";
  constexpr const char *kWorkMode = "kadas/milx/workMode";
  constexpr const char *kLeaderLineWidth = "kadas/milx/leaderLineWidth";
  constexpr const char *kLeaderLineColor = "kadas/milx/leaderLineColor";
} // namespace

namespace KadasMilxLayerSettings
{
  bool overrideEnabled( const QgsMapLayer *layer )
  {
    if ( !layer )
      return false;
    return layer->customProperty( kOverride, false ).toBool();
  }

  void setOverrideEnabled( QgsMapLayer *layer, bool enabled )
  {
    if ( !layer )
      return;
    layer->setCustomProperty( kOverride, enabled );
  }

  KadasMilxSymbolSettings layerSettings( const QgsMapLayer *layer )
  {
    KadasMilxSymbolSettings s;
    if ( !layer )
      return s;
    s.symbolSize = layer->customProperty( kSymbolSize, s.symbolSize ).toInt();
    s.lineWidth = layer->customProperty( kLineWidth, s.lineWidth ).toInt();
    s.workMode = static_cast<KadasMilxSymbolSettings::WorkMode>( layer->customProperty( kWorkMode, static_cast<int>( s.workMode ) ).toInt() );
    s.leaderLineWidth = layer->customProperty( kLeaderLineWidth, s.leaderLineWidth ).toInt();
    s.leaderLineColor = QColor( layer->customProperty( kLeaderLineColor, QColor( s.leaderLineColor ).name() ).toString() );
    return s;
  }

  void setLayerSettings( QgsMapLayer *layer, const KadasMilxSymbolSettings &s )
  {
    if ( !layer )
      return;
    layer->setCustomProperty( kSymbolSize, s.symbolSize );
    layer->setCustomProperty( kLineWidth, s.lineWidth );
    layer->setCustomProperty( kWorkMode, static_cast<int>( s.workMode ) );
    layer->setCustomProperty( kLeaderLineWidth, s.leaderLineWidth );
    layer->setCustomProperty( kLeaderLineColor, s.leaderLineColor.name() );
  }

  KadasMilxSymbolSettings resolve( const QgsMapLayer *layer )
  {
    if ( layer && overrideEnabled( layer ) )
      return layerSettings( layer );
    return KadasMilxClient::globalSymbolSettings();
  }

  KadasMilxSymbolSettings resolve( const QgsRenderContext &context )
  {
    const QString layerId = context.expressionContext().variable( QStringLiteral( "layer_id" ) ).toString();
    if ( !layerId.isEmpty() )
    {
      if ( QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId ) )
        return resolve( layer );
    }
    return KadasMilxClient::globalSymbolSettings();
  }
} // namespace KadasMilxLayerSettings
