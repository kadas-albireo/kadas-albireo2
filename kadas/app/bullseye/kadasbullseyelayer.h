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

#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>

#include "kadas/core/kadaspluginlayer.h"

class QgsMapCanvas;
class QgsLayerTreeView;

class KadasBullseyeLayer : public KadasPluginLayer {
  Q_OBJECT
public:
  static QString layerType() { return "bullseye"; }

  KadasBullseyeLayer(const QString &name);
  void setup(const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs,
             int rings, double interval, Qgis::DistanceUnit intervalUnit,
             double axesInterval);

  QString layerTypeKey() const override { return layerType(); }
  KadasBullseyeLayer *clone() const override;
  QgsMapLayerRenderer *
  createMapRenderer(QgsRenderContext &rendererContext) override;
  QgsRectangle extent() const override;

  QgsPointXY center() const { return mBullseyeConfig.center; }
  int rings() const { return mBullseyeConfig.rings; }
  double ringInterval() const { return mBullseyeConfig.interval; }
  Qgis::DistanceUnit ringIntervalUnit() const {
    return mBullseyeConfig.intervalUnit;
  }
  double axesInterval() const { return mBullseyeConfig.axesInterval; }

  const QColor &color() const { return mBullseyeConfig.color; }
  int fontSize() const { return mBullseyeConfig.fontSize; }
  bool labelAxes() const { return mBullseyeConfig.labelAxes; }
  bool labelQuadrants() const { return mBullseyeConfig.labelQuadrants; }
  bool labelRings() const { return mBullseyeConfig.labelRings; }
  int lineWidth() const { return mBullseyeConfig.lineWidth; }

public slots:
  void setColor(const QColor &color) { mBullseyeConfig.color = color; }
  void setFontSize(int fontSize) { mBullseyeConfig.fontSize = fontSize; }
  void setLabelAxes(bool labelAxes) { mBullseyeConfig.labelAxes = labelAxes; }
  void setLabelQuadrants(bool labelQuadrants) {
    mBullseyeConfig.labelQuadrants = labelQuadrants;
  }
  void setLabelRings(bool labelRings) {
    mBullseyeConfig.labelRings = labelRings;
  }
  void setLineWidth(int lineWidth) { mBullseyeConfig.lineWidth = lineWidth; }

protected:
  bool readXml(const QDomNode &layer_node,
               QgsReadWriteContext &context) override;
  bool writeXml(QDomNode &layer_node, QDomDocument &document,
                const QgsReadWriteContext &context) const override;

private:
  class Renderer;

  struct BullseyeConfig {
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
};

class KadasBullseyeLayerType : public KadasPluginLayerType {
  Q_OBJECT

public:
  KadasBullseyeLayerType(QAction *actionBullseyeTool)
      : KadasPluginLayerType(KadasBullseyeLayer::layerType()),
        mActionBullseyeTool(actionBullseyeTool) {}
  void addLayerTreeMenuActions(QMenu *menu,
                               QgsPluginLayer *layer) const override;
  QgsPluginLayer *createLayer() override { return new KadasBullseyeLayer(""); }
  QgsPluginLayer *createLayer(const QString &uri) override SIP_FACTORY {
    return new KadasBullseyeLayer("");
  }

private:
  QAction *mActionBullseyeTool;
};

#endif // KADASBULLSEYELAYER_H
