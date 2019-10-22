/***************************************************************************
    kadasguidegridlayer.h
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

#ifndef KADASGUIDEGRIDLAYER_H
#define KADASGUIDEGRIDLAYER_H

#include <kadas/core/kadaspluginlayer.h>

class KadasGuideGridLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    static QString layerTypeKey() { return "guide_grid"; }
    enum LabellingMode { LABEL_A_1, LABEL_1_A };

    KadasGuideGridLayer( const QString &name );
    void setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked );
    bool writeSymbology( QDomNode &node, QDomDocument &doc, QString &errorMessage, const QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) const override { return true; }
    bool readSymbology( const QDomNode &node, QString &errorMessage, QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) override { return true; }
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override  { return mGridRect; }
    int cols() const { return mCols; }
    int rows() const { return mRows; }
    bool colSizeLocked() const { return mColSizeLocked; }
    bool rowSizeLocked() const { return mRowSizeLocked; }

    const QColor &color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    LabellingMode labelingMode() const { return mLabellingMode; }

    KadasGuideGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPoint &mapPos, const QgsMapSettings &mapSettings ) override;

  public slots:
    void setColor( const QColor &color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabelingMode( LabellingMode labelingMode ) { mLabellingMode = labelingMode; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    class Renderer;

    QgsRectangle mGridRect;
    int mCols = 0;
    int mRows = 0;
    bool mColSizeLocked = false;
    bool mRowSizeLocked = false;
    int mFontSize = 30;
    QColor mColor = Qt::red;
    LabellingMode mLabellingMode = LABEL_A_1;
};


class KadasGuideGridLayerType : public KadasPluginLayerType
{
  public:
    KadasGuideGridLayerType( QAction *actionGuideGridTool )
      : KadasPluginLayerType( KadasGuideGridLayer::layerTypeKey() ), mActionGuideGridTool( actionGuideGridTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasGuideGridLayer( "" ); }

  private:
    QAction *mActionGuideGridTool;
};

#endif // KADASGUIDEGRIDLAYER_H
