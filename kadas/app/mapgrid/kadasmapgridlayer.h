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

#include <qgscoordinateformatter.h>
#include <qgsmaplayerrenderer.h>

#include "kadas/core/kadaspluginlayer.h"


class KadasMapGridLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    static QString layerType() { return "map_grid"; }

    enum GridType {GridLV03, GridLV95, GridDD, GridDM, GridDMS, GridUTM, GridMGRS};
    enum LabelingMode {LabelingDisabled, LabelingEnabled};

    KadasMapGridLayer( const QString &name );
    void setup( GridType type, double intervalX, double intervalY, int cellSize );

    QString layerTypeKey() const override { return layerType(); }
    KadasMapGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override  { return QgsRectangle(); }

    GridType gridType() const { return mGridConfig.gridType; }
    double intervalX() const { return mGridConfig.intervalX; }
    double intervalY() const { return mGridConfig.intervalY; }
    int cellSize() const { return mGridConfig.cellSize; }

    const QColor &color() const { return mGridConfig.color; }
    int fontSize() const { return mGridConfig.fontSize; }
    LabelingMode labelingMode() const { return mGridConfig.labelingMode; }

  public slots:
    void setColor( const QColor &color ) { mGridConfig.color = color; }
    void setFontSize( int fontSize ) { mGridConfig.fontSize = fontSize; }
    void setLabelingMode( LabelingMode labelingMode ) { mGridConfig.labelingMode = labelingMode; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    struct GridConfig
    {
      GridType gridType = GridLV95;
      double intervalX = 10000;
      double intervalY = 10000;
      int cellSize = 0;
      int fontSize = 15;
      QColor color = Qt::black;
      LabelingMode labelingMode = LabelingEnabled;
    } mGridConfig;

    class Renderer : public QgsMapLayerRenderer
    {
      public:
        Renderer( KadasMapGridLayer *layer, QgsRenderContext &rendererContext );

        bool render() override;

      private:
        GridConfig mRenderGridConfig;
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
    };
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
