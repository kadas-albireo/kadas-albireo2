/***************************************************************************
    kadasmilxlayersettings.h
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

#ifndef KADASMILXLAYERSETTINGS_H
#define KADASMILXLAYERSETTINGS_H

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/milx/kadasmilxclient.h"

class QgsMapLayer;
class QgsRenderContext;

/**
 * \ingroup gui
 * \brief Helpers to read / write per-annotation-layer MilX symbol settings.
 *
 * Settings are stashed as \c QgsMapLayer::customProperty entries under the
 * \c "kadas/milx/..." namespace. When \c override is \c false (the default),
 * \c resolve() falls back to \c KadasMilxClient::globalSymbolSettings().
 */
namespace KadasMilxLayerSettings
{
  KADAS_GUI_EXPORT bool overrideEnabled( const QgsMapLayer *layer );
  KADAS_GUI_EXPORT void setOverrideEnabled( QgsMapLayer *layer, bool enabled );

  KADAS_GUI_EXPORT KadasMilxSymbolSettings layerSettings( const QgsMapLayer *layer );
  KADAS_GUI_EXPORT void setLayerSettings( QgsMapLayer *layer, const KadasMilxSymbolSettings &settings );

  //! Returns layer override if enabled, otherwise the global settings.
  KADAS_GUI_EXPORT KadasMilxSymbolSettings resolve( const QgsMapLayer *layer );

  /**
   * Resolve at render time by looking up the source layer through the
   * render context's expression scope (\c layer_id variable). Falls back
   * to global settings if the layer cannot be located.
   */
  KADAS_GUI_EXPORT KadasMilxSymbolSettings resolve( const QgsRenderContext &context );
} // namespace KadasMilxLayerSettings

#endif // KADASMILXLAYERSETTINGS_H
