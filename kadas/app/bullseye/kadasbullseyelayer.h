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

#include <kadas/core/kadaspluginlayer.h>

class QgsMapCanvas;
class QgsLayerTreeView;

class KadasBullseyeLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "bullseye"; }
    enum LabellingMode { NO_LABELS, LABEL_AXES, LABEL_RINGS, LABEL_AXES_RINGS };

    KadasBullseyeLayer( const QString &name );
    void setup( const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, double axesInterval );

    KadasBullseyeLayer *clone() const override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override;

    QgsPointXY center() const { return mCenter; }
    int rings() const { return mRings; }
    double ringInterval() const { return mInterval; }
    double axesInterval() const { return mAxesInterval; }

    const QColor &color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    LabellingMode labellingMode() const { return mLabellingMode; }
    int lineWidth() const { return mLineWidth; }

  public slots:
    void setColor( const QColor &color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabellingMode( LabellingMode labellingMode ) { mLabellingMode = labellingMode; }
    void setLineWidth( int lineWidth ) { mLineWidth = lineWidth; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    class Renderer;

    QgsPointXY mCenter;
    int mRings;
    double mInterval;
    double mAxesInterval;
    QColor mColor = Qt::black;
    int mFontSize = 10;
    LabellingMode mLabellingMode = NO_LABELS;
    int mLineWidth = 1;
};

class KadasBullseyeLayerType : public KadasPluginLayerType
{
  public:
    KadasBullseyeLayerType( QAction *actionBullseyeTool )
      : KadasPluginLayerType( KadasBullseyeLayer::layerTypeKey() ), mActionBullseyeTool( actionBullseyeTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasBullseyeLayer( "" ); }
    QgsPluginLayer *createLayer( const QString &uri ) override SIP_FACTORY { return new KadasBullseyeLayer( "" ); }

  private:
    QAction *mActionBullseyeTool;
};

#endif // KADASBULLSEYELAYER_H
