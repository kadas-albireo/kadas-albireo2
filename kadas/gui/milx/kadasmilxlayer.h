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

class KADAS_GUI_EXPORT KadasMilxLayer : public KadasItemLayer
{
    Q_OBJECT

  public:
    static QString layerType() { return "KadasMilxLayer"; }

    KadasMilxLayer( const QString &name = "MilX" );
    QString layerTypeKey() const override { return layerType(); };

    bool readXml( const QDomNode &layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const override;

    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    ItemId pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const override;

    void setApproved( bool approved );
    bool isApproved() const { return mIsApproved; }

    void exportToMilxly( QDomElement &milxLayerEl, int dpi );
    bool importFromMilxly( const QDomElement &milxLayerEl, int dpi, QString &errorMsg );

  signals:
    void approvedChanged( bool approved );

  private:
    class Renderer;

    bool mIsApproved = false;
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
