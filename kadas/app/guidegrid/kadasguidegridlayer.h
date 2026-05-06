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

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsrectangle.h>

#include "kadas/core/kadaspluginlayer.h"

/**
 * Guide grid layer: draws a numbered/lettered rectangular grid over the map.
 *
 * Implemented as a QgsAnnotationLayer subclass:
 *
 * - Configuration is stored at the layer level (as a `<KadasGuideGrid>`
 *   child element on the layer node).
 * - The visual grid (lines + cell labels) is also materialized as plain
 *   QgsAnnotationLineItem + QgsAnnotationPointTextItem items, so that
 *   vanilla QGIS can render the layer natively without any Kadas code.
 * - Inside Kadas, `createMapRenderer()` is overridden to return a custom
 *   renderer that paints lines + labels directly with QPainter. This
 *   preserves the dynamic "labels follow the visible map edge" behavior
 *   that Kadas users rely on; the stored annotation items are ignored at
 *   paint time inside Kadas (they only exist to feed the QGIS render
 *   pipeline when the project is opened in vanilla QGIS).
 */
class KadasGuideGridLayer : public QgsAnnotationLayer
{
    Q_OBJECT
  public:
    enum LabelingPos
    {
      LabelsInside,
      LabelsOutside
    };
    enum QuadrantLabeling
    {
      DontLabelQuadrants,
      LabelOneQuadrant,
      LabelAllQuadrants
    };

    static QString layerType() { return "guide_grid"; }

    /// Convert a plain QgsAnnotationLayer (one carrying the
    /// `kadas/annotation-type=guidegrid` marker customProperty, e.g. as
    /// produced by a vanilla QGIS reload of a Kadas project) into a
    /// KadasGuideGridLayer. The new layer keeps the same name, id, CRS,
    /// opacity, customProperties and annotation items as the source.
    /// Caller is responsible for swapping the plain layer with the
    /// returned subclass instance in the project.
    static KadasGuideGridLayer *promote( QgsAnnotationLayer *plain );

    explicit KadasGuideGridLayer( const QString &name );

    void setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked );

    KadasGuideGridLayer *clone() const override;
    QgsRectangle extent() const override { return mGridConfig.gridRect; }
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;

    QList<KadasPluginLayer::IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings );

    int cols() const { return mGridConfig.cols; }
    int rows() const { return mGridConfig.rows; }
    bool colSizeLocked() const { return mGridConfig.colSizeLocked; }
    bool rowSizeLocked() const { return mGridConfig.rowSizeLocked; }

    const QColor &color() const { return mGridConfig.color; }
    int lineWidth() const { return mGridConfig.lineWidth; }
    int fontSize() const { return mGridConfig.fontSize; }
    QPair<QChar, QChar> labelingMode() const { return qMakePair( mGridConfig.rowChar, mGridConfig.colChar ); }
    LabelingPos labelingPos() const { return mGridConfig.labelingPos; }
    QuadrantLabeling labelQuadrants() const { return mGridConfig.quadrantLabeling; }

  public slots:
    void setColor( const QColor &color )
    {
      mGridConfig.color = color;
      regenerate();
    }
    void setLineWidth( int lineWidth )
    {
      mGridConfig.lineWidth = lineWidth;
      regenerate();
    }
    void setFontSize( int fontSize )
    {
      mGridConfig.fontSize = fontSize;
      regenerate();
    }
    void setLabelingMode( QChar rowChar, QChar colChar )
    {
      mGridConfig.rowChar = rowChar;
      mGridConfig.colChar = colChar;
      regenerate();
    }
    void setLabelingPos( LabelingPos pos )
    {
      mGridConfig.labelingPos = pos;
      regenerate();
    }
    void setLabelQuadrants( QuadrantLabeling labelQuadrants )
    {
      mGridConfig.quadrantLabeling = labelQuadrants;
      regenerate();
    }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    friend class KadasGuideGridRenderer;
    /// Mirror mGridConfig onto layer customProperties so the config
    /// survives a round-trip through vanilla QGIS (which has no knowledge
    /// of this subclass and would strip any custom child element).
    void writeConfigToCustomProperties();

    struct GridConfig
    {
        QgsRectangle gridRect;
        int cols = 0;
        int rows = 0;
        bool colSizeLocked = false;
        bool rowSizeLocked = false;
        int fontSize = 30;
        QColor color = Qt::red;
        int lineWidth = 1;
        QChar rowChar = 'A';
        QChar colChar = '1';
        LabelingPos labelingPos = LabelsInside;
        QuadrantLabeling quadrantLabeling = DontLabelQuadrants;
    } mGridConfig;

    /// Rebuild the layer's annotation items (lines + labels) from the current GridConfig.
    void regenerate();
};

#endif // KADASGUIDEGRIDLAYER_H
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

#include "kadas/core/kadaspluginlayer.h"

class KadasGuideGridLayer : public KadasPluginLayer
{
    Q_OBJECT
  public:
    enum LabelingPos
    {
      LabelsInside,
      LabelsOutside
    };
    enum QuadrantLabeling
    {
      DontLabelQuadrants,
      LabelOneQuadrant,
      LabelAllQuadrants
    };
    static QString layerType() { return "guide_grid"; }

    KadasGuideGridLayer( const QString &name );
    void setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked );

    QString layerTypeKey() const override { return layerType(); }
    KadasGuideGridLayer *clone() const override;
    QList<IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) override;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override { return mGridConfig.gridRect; }

    int cols() const { return mGridConfig.cols; }
    int rows() const { return mGridConfig.rows; }
    bool colSizeLocked() const { return mGridConfig.colSizeLocked; }
    bool rowSizeLocked() const { return mGridConfig.rowSizeLocked; }

    const QColor &color() const { return mGridConfig.color; }
    int lineWidth() const { return mGridConfig.lineWidth; }
    int fontSize() const { return mGridConfig.fontSize; }
    QPair<QChar, QChar> labelingMode() const { return qMakePair( mGridConfig.rowChar, mGridConfig.colChar ); }
    LabelingPos labelingPos() const { return mGridConfig.labelingPos; }
    QuadrantLabeling labelQuadrants() const { return mGridConfig.quadrantLabeling; }

  public slots:
    void setColor( const QColor &color ) { mGridConfig.color = color; }
    void setLineWidth( int lineWidth ) { mGridConfig.lineWidth = lineWidth; }
    void setFontSize( int fontSize ) { mGridConfig.fontSize = fontSize; }
    void setLabelingMode( QChar rowChar, QChar colChar )
    {
      mGridConfig.rowChar = rowChar;
      mGridConfig.colChar = colChar;
    }
    void setLabelingPos( LabelingPos pos ) { mGridConfig.labelingPos = pos; }
    void setLabelQuadrants( QuadrantLabeling labelQuadrants ) { mGridConfig.quadrantLabeling = labelQuadrants; }

  protected:
    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

  private:
    class Renderer;

    struct GridConfig
    {
        QgsRectangle gridRect;
        int cols = 0;
        int rows = 0;
        bool colSizeLocked = false;
        bool rowSizeLocked = false;
        int fontSize = 30;
        QColor color = Qt::red;
        int lineWidth = 1;
        QChar rowChar = 'A';
        QChar colChar = '1';
        LabelingPos labelingPos = LabelsInside;
        QuadrantLabeling quadrantLabeling = DontLabelQuadrants;
    } mGridConfig;
};


class KadasGuideGridLayerType : public KadasPluginLayerType
{
    Q_OBJECT

  public:
    KadasGuideGridLayerType( QAction *actionGuideGridTool )
      : KadasPluginLayerType( KadasGuideGridLayer::layerType() )
      , mActionGuideGridTool( actionGuideGridTool )
    {}
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
    QgsPluginLayer *createLayer() override { return new KadasGuideGridLayer( "" ); }
    QgsPluginLayer *createLayer( const QString &uri ) override { return new KadasGuideGridLayer( "" ); }

  private:
    QAction *mActionGuideGridTool;
};

#endif // KADASGUIDEGRIDLAYER_H
