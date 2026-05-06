/***************************************************************************
    kadasbullseyelayer.h
    --------------------
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

#ifndef KADASBULLSEYELAYER_H
#define KADASBULLSEYELAYER_H

#include <qgis/qgsannotationlayer.h>

class QgsMapCanvas;
class QgsLayerTreeView;

/**
 * Bullseye layer: concentric range rings + bearing axes around a center
 * point.
 *
 * Implemented as a `QgsAnnotationLayer` subclass:
 *
 * - Configuration is stored at the layer level (as a `<KadasBullseye>`
 *   child element on the layer node, on top of the stock annotation
 *   layer XML).
 * - The visual is also materialized as plain `QgsAnnotationLineItem` +
 *   `QgsAnnotationPointTextItem` items so vanilla QGIS can render the
 *   layer natively without any Kadas code.
 * - Inside Kadas, `createMapRenderer()` returns the existing custom
 *   QPainter renderer so the visual stays pixel-identical to the old
 *   plugin-layer behavior (geodesic ring sampling, axis rendering with
 *   ~100 km segments, label placement, etc.). The static items emitted
 *   by `regenerate()` are ignored at paint time inside Kadas.
 */
class KadasBullseyeLayer : public QgsAnnotationLayer
{
    Q_OBJECT
  public:
    static QString layerType() { return "bullseye"; }

    explicit KadasBullseyeLayer( const QString &name );
    void setup( const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, Qgis::DistanceUnit intervalUnit, double axesInterval );

    KadasBullseyeLayer *clone() const override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override;

    QgsPointXY center() const { return mBullseyeConfig.center; }
    int rings() const { return mBullseyeConfig.rings; }
    double ringInterval() const { return mBullseyeConfig.interval; }
    Qgis::DistanceUnit ringIntervalUnit() const { return mBullseyeConfig.intervalUnit; }
    double axesInterval() const { return mBullseyeConfig.axesInterval; }

    const QColor &color() const { return mBullseyeConfig.color; }
    int fontSize() const { return mBullseyeConfig.fontSize; }
    bool labelAxes() const { return mBullseyeConfig.labelAxes; }
    bool labelQuadrants() const { return mBullseyeConfig.labelQuadrants; }
    bool labelRings() const { return mBullseyeConfig.labelRings; }
    int lineWidth() const { return mBullseyeConfig.lineWidth; }

  public slots:
    void setColor( const QColor &color )
    {
      mBullseyeConfig.color = color;
      regenerate();
    }
    void setFontSize( int fontSize )
    {
      mBullseyeConfig.fontSize = fontSize;
      regenerate();
    }
    void setLabelAxes( bool labelAxes )
    {
      mBullseyeConfig.labelAxes = labelAxes;
      regenerate();
    }
    void setLabelQuadrants( bool labelQuadrants )
    {
      mBullseyeConfig.labelQuadrants = labelQuadrants;
      regenerate();
    }
    void setLabelRings( bool labelRings )
    {
      mBullseyeConfig.labelRings = labelRings;
      regenerate();
    }
    void setLineWidth( int lineWidth )
    {
      mBullseyeConfig.lineWidth = lineWidth;
      regenerate();
    }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    class Renderer;
    /// Mirror mBullseyeConfig onto layer customProperties so the config
    /// survives a round-trip through vanilla QGIS (which has no knowledge
    /// of this subclass and would strip any custom child element).
    void writeConfigToCustomProperties();

    struct BullseyeConfig
    {
        QgsPointXY center;
        int rings;
        double interval;
        Qgis::DistanceUnit intervalUnit = Qgis::DistanceUnit::NauticalMiles;
        double axesInterval;
        QColor color = Qt::black;
        int fontSize = 20;
        bool labelAxes = false;
        bool labelQuadrants = false;
        bool labelRings = false;
        int lineWidth = 1;
    } mBullseyeConfig;

    /// Rebuild the layer's annotation items (rings + axes + labels) from
    /// the current BullseyeConfig.
    void regenerate();
};

#endif // KADASBULLSEYELAYER_H
