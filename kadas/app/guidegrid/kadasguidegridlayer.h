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
    enum LabelingPos {LabelsInside, LabelsOutside};
    static QString layerType() { return "guide_grid"; }

    KadasGuideGridLayer( const QString &name );
    void setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked );

    QString layerTypeKey() const override { return layerType(); };
    KadasGuideGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override  { return mGridRect; }

    int cols() const { return mCols; }
    int rows() const { return mRows; }
    bool colSizeLocked() const { return mColSizeLocked; }
    bool rowSizeLocked() const { return mRowSizeLocked; }

    const QColor &color() const { return mColor; }
    int fontSize() const { return mFontSize; }
    QPair<QChar, QChar> labelingMode() const { return qMakePair( mRowChar, mColChar ); }
    LabelingPos labelingPos() const { return mLabelingPos; }
    bool labelQuadrants() const { return mLabelQuadrants; }

  public slots:
    void setColor( const QColor &color ) { mColor = color; }
    void setFontSize( int fontSize ) { mFontSize = fontSize; }
    void setLabelingMode( QChar rowChar, QChar colChar ) { mRowChar = rowChar; mColChar = colChar; }
    void setLabelingPos( LabelingPos pos ) { mLabelingPos = pos; }
    void setLabelQuadrants( bool labelQuadrants ) { mLabelQuadrants = labelQuadrants; }

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
    QChar mRowChar = 'A';
    QChar mColChar = '1';
    LabelingPos mLabelingPos = LabelsInside;
    bool mLabelQuadrants = false;
};


class KadasGuideGridLayerType : public KadasPluginLayerType
{
    Q_OBJECT

  public:
    KadasGuideGridLayerType( QAction *actionGuideGridTool )
      : KadasPluginLayerType( KadasGuideGridLayer::layerType() ), mActionGuideGridTool( actionGuideGridTool ) {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasGuideGridLayer( "" ); }

  private:
    QAction *mActionGuideGridTool;
};

#endif // KADASGUIDEGRIDLAYER_H
