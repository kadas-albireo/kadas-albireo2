/***************************************************************************
    kadasmapgridlayer.h
    -------------------
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

#ifndef KADASMAPGRIDLAYER_H
#define KADASMAPGRIDLAYER_H


#include "kadas/core/kadaspluginlayer.h"


class KadasMapGridLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    static QString layerType() { return "map_grid"; }

    enum GridType
    {
      GridLV03,
      GridLV95,
      GridDD,
      GridDM,
      GridDMS,
      GridUTM,
      GridMGRS
    };
    enum LabelingMode
    {
      LabelingDisabled,
      LabelingEnabled
    };

    struct GridConfig
    {
        GridType gridType = GridLV95;
        double intervalX = 10000;
        double intervalY = 10000;
        int cellSize = 0;
        int fontSize = 15;
        double lineWidth = 1;
        QColor color = Qt::black;
        LabelingMode labelingMode = LabelingEnabled;
    };

    KadasMapGridLayer( const QString &name );
    void setup( GridType type, double intervalX, double intervalY, int cellSize );

    QString layerTypeKey() const override { return layerType(); }
    KadasMapGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override { return QgsRectangle(); }

    GridType gridType() const { return mGridConfig.gridType; }
    double intervalX() const { return mGridConfig.intervalX; }
    double intervalY() const { return mGridConfig.intervalY; }
    int cellSize() const { return mGridConfig.cellSize; }

    const QColor &color() const { return mGridConfig.color; }
    int fontSize() const { return mGridConfig.fontSize; }
    double lineWidth() const { return mGridConfig.lineWidth; }
    LabelingMode labelingMode() const { return mGridConfig.labelingMode; }

    GridConfig gridConfig() const { return mGridConfig; }

  public slots:
    void setColor( const QColor &color ) { mGridConfig.color = color; }
    void setFontSize( int fontSize ) { mGridConfig.fontSize = fontSize; }
    void setLineWidth( double lineWidth ) { mGridConfig.lineWidth = lineWidth; }
    void setLabelingMode( LabelingMode labelingMode ) { mGridConfig.labelingMode = labelingMode; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    GridConfig mGridConfig;
};


class KadasMapGridLayerType : public KadasPluginLayerType
{
    Q_OBJECT

  public:
    KadasMapGridLayerType( QAction *actionMapGridTool )
      : KadasPluginLayerType( KadasMapGridLayer::layerType() ), mActionMapGridTool( actionMapGridTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasMapGridLayer( "" ); }
    QgsPluginLayer *createLayer( const QString &uri ) override { return new KadasMapGridLayer( "" ); }

  private:
    QAction *mActionMapGridTool;
};

#endif // KADASMAPGRIDLAYER_H
