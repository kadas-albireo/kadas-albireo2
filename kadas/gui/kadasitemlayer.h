/***************************************************************************
    kadasitemlayer.h
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

#ifndef KADASITEMLAYER_H
#define KADASITEMLAYER_H

#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>

#include <kadas/gui/kadas_gui.h>

class QMenu;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasItemLayer : public QgsPluginLayer
{
    Q_OBJECT
  public:
    static QString layerType() { return "KadasItemLayer"; }
    KadasItemLayer( const QString &name );

    void addItem( KadasMapItem *item );
    KadasMapItem *takeItem( const QString &itemId );
    const QMap<QString, KadasMapItem *> &items() const { return mItems; }

    void setOpacity( int opacity ) { mOpacity = opacity; }
    int opacity() const { return mOpacity; }

    KadasItemLayer *clone() const override SIP_FACTORY;
    QgsMapLayerRenderer *createMapRenderer( QgsRenderContext &rendererContext ) override;
    QgsRectangle extent() const override;
    virtual QString pickItem( const QgsRectangle &pickRect, const QgsMapSettings &mapSettings ) const;
    QString pickItem( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) const;
    void setTransformContext( const QgsCoordinateTransformContext &ctx ) override;

    bool writeSymbology( QDomNode &node, QDomDocument &doc, QString &errorMessage, const QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) const override { return true; }
    bool readSymbology( const QDomNode &node, QString &errorMessage, QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) override { return true; }

    virtual void addLayerMenuActions( QMenu *menu ) const {}

  protected:
    class Renderer;

    QMap<QString, KadasMapItem *> mItems;
    QgsCoordinateTransformContext mTransformContext;
    int mOpacity = 100;
};

class KADAS_GUI_EXPORT KadasItemLayerType : public QgsPluginLayerType
{
  public:
    KadasItemLayerType()
      : QgsPluginLayerType( KadasItemLayer::layerType() ) {}
    QgsPluginLayer *createLayer() override SIP_FACTORY { return new KadasItemLayer( "Items" ); }
    QgsPluginLayer *createLayer( const QString &uri ) override SIP_FACTORY { return new KadasItemLayer( "Items" ); }
};

#endif // KADASITEMLAYER_H
