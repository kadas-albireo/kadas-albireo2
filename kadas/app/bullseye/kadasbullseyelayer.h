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
    static QString layerType() { return "bullseye"; }

    KadasBullseyeLayer( const QString &name );
    void setup( const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, double axesInterval );

    QString layerTypeKey() const override { return layerType(); };
    KadasBullseyeLayer *clone() const override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override;

    QgsPointXY center() const { return mCenter; }
    int rings() const { return mRings; }
    double ringInterval() const { return mInterval; }
    double axesInterval() const { return mAxesInterval; }

    const QColor &color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    bool labelAxes() const { return mLabelAxes; }
    bool labelQuadrants() const { return mLabelQuadrants; }
    bool labelRings() const { return mLabelRings; }
    int lineWidth() const { return mLineWidth; }

  public slots:
    void setColor( const QColor &color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabelAxes( bool labelAxes ) { mLabelAxes = labelAxes; }
    void setLabelQuadrants( bool labelQuadrants ) { mLabelQuadrants = labelQuadrants; }
    void setLabelRings( bool labelRings ) { mLabelRings = labelRings; }
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
    bool mLabelAxes = false;
    bool mLabelQuadrants = false;
    bool mLabelRings = false;
    int mLineWidth = 1;
};

class KadasBullseyeLayerType : public KadasPluginLayerType
{
    Q_OBJECT

  public:
    KadasBullseyeLayerType( QAction *actionBullseyeTool )
      : KadasPluginLayerType( KadasBullseyeLayer::layerType() ), mActionBullseyeTool( actionBullseyeTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasBullseyeLayer( "" ); }
    QgsPluginLayer *createLayer( const QString &uri ) override SIP_FACTORY { return new KadasBullseyeLayer( "" ); }

  private:
    QAction *mActionBullseyeTool;
};

#endif // KADASBULLSEYELAYER_H
