/***************************************************************************
    kadasmapgridlayerrenderer.h
    -------------------
    copyright            : (C) 2024.11.08
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASMAPGRIDLAYERRENDERER_H
#define KADASMAPGRIDLAYERRENDERER_H

#include <qgscoordinateformatter.h>
#include <qgsmaplayerrenderer.h>

#include "kadasmapgridlayer.h"
#include "kadas/core/kadaslatlontoutm.h"

class KadasMapGridLayerRenderer : public QgsMapLayerRenderer
{
  public:
    KadasMapGridLayerRenderer( KadasMapGridLayer *layer, QgsRenderContext &rendererContext );

    bool render() override;

  private:
    KadasMapGridLayer::GridConfig mRenderGridConfig;
    double mRenderOpacity = 1.;

    struct GridLabel
    {
        QString text;
        QPointF screenPos;
    };

    void drawCrsGrid( const QString &crs, double segmentLength, QgsCoordinateFormatter::Format format, int precision, QgsCoordinateFormatter::FormatFlags flags );
    void adjustZoneLabelPos( QPointF &labelPos, const QPointF &maxLabelPos, const QRectF &visibleExtent );
    QRect computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel );
    void drawMgrsGrid();
    void drawGridLabel( const QPointF &pos, const QString &text, const QFont &font, const QColor &bufferColor );

    QPen level2pen( KadasLatLonToUTM::Level level ) const;
    static double exponentialScale( double value, double domainMin, double domainMax, double rangeMin, double rangeMax, double exponent = 1 );
};

#endif // KADASMAPGRIDLAYERRENDERER_H
