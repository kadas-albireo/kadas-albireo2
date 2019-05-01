/***************************************************************************
    kadasredlininglayer.h
    ---------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASREDLININGRENDERER_H
#define KADASREDLININGRENDERER_H

#include <QScopedPointer>

#include <qgis/qgsrenderer.h>

#include <kadas/core/kadas_core.h>

class KADAS_CORE_EXPORT KadasRedliningRenderer : public QgsFeatureRenderer
{
  public:
    KadasRedliningRenderer( );
    KadasRedliningRenderer* clone() const override { return new KadasRedliningRenderer(); }

    QgsSymbol *symbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override { return originalSymbolForFeature( feature, context ); }
    QgsSymbol* originalSymbolForFeature( const QgsFeature& feature, QgsRenderContext &context) const override;

    void startRender( QgsRenderContext& context, const QgsFields& fields ) override;
    bool renderFeature( const QgsFeature &feature, QgsRenderContext &context, int layer, bool selected, bool drawVertexMarker ) override;
    void stopRender( QgsRenderContext& context ) override;

    QSet<QString> usedAttributes( const QgsRenderContext &context ) const override { return QSet<QString>(); }
    QString dump() const override { return "QgsRedliningRendererV2"; }

    QgsFeatureRenderer::Capabilities capabilities() override { return SymbolLevels; }

  protected:
    QScopedPointer<QgsMarkerSymbol> mMarkerSymbol;
    QScopedPointer<QgsLineSymbol> mLineSymbol;
    QScopedPointer<QgsFillSymbol> mFillSymbol;

    void drawVertexMarkers( QgsAbstractGeometry* geom, QgsRenderContext& context );
};

#endif // KADASREDLININGRENDERER_H
