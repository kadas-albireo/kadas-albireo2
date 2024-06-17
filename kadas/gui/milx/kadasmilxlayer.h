/***************************************************************************
    kadasmilxlayer.h
    ----------------
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

#ifndef KADASMILXLAYER_H
#define KADASMILXLAYER_H

#include <kadas/core/kadaspluginlayer.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/milx/kadasmilxclient.h>


class KADAS_GUI_EXPORT KadasMilxLayer : public KadasItemLayer
{
    Q_OBJECT

  public:
    static QString layerType() { return "KadasMilxLayer"; }

    KadasMilxLayer( const QString &name = "MilX" );
    QString layerTypeKey() const override { return layerType(); }
    bool acceptsItem( const KadasMapItem *item ) const override;

    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    ItemId pickItem( const KadasMapPos &mapPos, const QgsMapSettings &mapSettings, KadasItemLayer::PickObjective pickObjective = KadasItemLayer::PickObjective::PICK_OBJECTIVE_ANY ) const override;

    void setApproved( bool approved );
    bool isApproved() const { return mIsApproved; }

    void exportToMilxly( QDomElement &milxLayerEl, int dpi );
    bool importFromMilxly( const QDomElement &milxLayerEl, int dpi, QString &errorMsg );

    void setOverrideMilxSymbolSettings( bool overrideSettings ) { mOverrideMilxSymbolSettings = overrideSettings; }
    bool overrideMilxSymbolSettings() const { return mOverrideMilxSymbolSettings; }

    void setMilxSymbolSize( int symbolSize ) { mMilxSymbolSettings.symbolSize = symbolSize; }
    int milxSymbolSize() const { return mMilxSymbolSettings.symbolSize; }

    void setMilxLineWidth( int lineWidth ) { mMilxSymbolSettings.lineWidth = lineWidth; }
    int milxLineWidth() const { return mMilxSymbolSettings.lineWidth; }

    void setMilxWorkMode( KadasMilxSymbolSettings::WorkMode workMode ) { mMilxSymbolSettings.workMode = workMode; }
    KadasMilxSymbolSettings::WorkMode milxWorkMode() const { return mMilxSymbolSettings.workMode; }

    void setMilxLeaderLineWidth( int width ) { mMilxSymbolSettings.leaderLineWidth = width; }
    int milxLeaderLineWidth() const { return mMilxSymbolSettings.leaderLineWidth; }

    void setMilxLeaderLineColor( const QColor &color ) { mMilxSymbolSettings.leaderLineColor = color; }
    QColor milxLeaderLineColor() const { return mMilxSymbolSettings.leaderLineColor; }

    const KadasMilxSymbolSettings &milxSymbolSettings() const;

  signals:
    void approvedChanged( bool approved );

  private:
    class Renderer;

    bool mIsApproved = false;
    bool mOverrideMilxSymbolSettings = false;
    KadasMilxSymbolSettings mMilxSymbolSettings;
};


class KADAS_GUI_EXPORT KadasMilxLayerType : public KadasPluginLayerType
{
    Q_OBJECT

  public:
    KadasMilxLayerType()
      : KadasPluginLayerType( KadasMilxLayer::layerType() ) {}
    QgsPluginLayer *createLayer() override SIP_FACTORY { return new KadasMilxLayer(); }
    QgsPluginLayer *createLayer( const QString &uri ) override SIP_FACTORY { return new KadasMilxLayer(); }
    void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const override;
};

#endif // KADASMILXLAYER_H
