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

#include <kadas/core/kadaspluginlayer.h>

class KadasMapGridLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    static QString layerType() { return "map_grid"; }

    enum GridType {GridLV03, GridLV95, GridDD, GridDM, GridDMS, GridUTM, GridMGRS};
    enum LabelingMode {LabelingDisabled, LabelingEnabled};

    KadasMapGridLayer( const QString &name );
    void setup( GridType type, double intervalX, double intervalY );

    QString layerTypeKey() const override { return layerType(); };
    KadasMapGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override  { return QgsRectangle(); }

    GridType gridType() const { return mGridType; }
    double intervalX() const { return mIntervalX; }
    double intervalY() const { return mIntervalY; }

    const QColor &color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    LabelingMode labelingMode() const { return mLabelingMode; }

  public slots:
    void setColor( const QColor &color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabelingMode( LabelingMode labelingMode ) { mLabelingMode = labelingMode; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    class Renderer;

    GridType mGridType = GridLV95;
    double mIntervalX = 10000;
    double mIntervalY = 10000;
    int mFontSize = 15;
    QColor mColor = Qt::black;
    LabelingMode mLabelingMode = LabelingEnabled;
};


class KadasMapGridLayerType : public KadasPluginLayerType
{
  public:
    KadasMapGridLayerType( QAction *actionMapGridTool )
      : KadasPluginLayerType( KadasMapGridLayer::layerType() ), mActionMapGridTool( actionMapGridTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasMapGridLayer( "" ); }

  private:
    QAction *mActionMapGridTool;
};

#endif // KADASMAPGRIDLAYER_H
