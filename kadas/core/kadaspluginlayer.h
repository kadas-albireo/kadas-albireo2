/***************************************************************************
    kadaspluginlayer.h
    ------------------
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

#ifndef KADASPLUGINLAYER_H
#define KADASPLUGINLAYER_H

#include <QObject>

#include <qgis/qgsgeometry.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspluginlayer.h>
#include <qgis/qgspluginlayerregistry.h>

#include "kadas/core/kadas_core.h"

class QMenu;


class KADAS_CORE_EXPORT KadasPluginLayer : public QgsPluginLayer
{
    Q_OBJECT

  public:
    KadasPluginLayer( const QString &layerType, const QString &layerName = QString() ) : QgsPluginLayer( layerType, layerName ) {}
    virtual QString layerTypeKey() const = 0;

    void setTransformContext( const QgsCoordinateTransformContext &ctx ) override { mTransformContext = ctx; }
    bool writeSymbology( QDomNode &node, QDomDocument &doc, QString &errorMessage, const QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) const override { return true; }
    bool readSymbology( const QDomNode &node, QString &errorMessage, QgsReadWriteContext &context, StyleCategories categories = AllStyleCategories ) override { return true; }

    void setOpacity( double opacity ) override { mOpacity = opacity; }
    double opacity() const override { return mOpacity; }

    class IdentifyResult
    {
      public:
        IdentifyResult( const QString &featureName, const QMap<QString, QVariant> &attributes, const QgsGeometry &geom )
          : mFeatureName( featureName ), mAttributes( attributes ), mGeom( geom ) {}
        const QString &featureName() const { return mFeatureName; }
        const QMap<QString, QVariant> &attributes() const { return mAttributes; }
        const QgsGeometry &geometry() const { return mGeom; }

      private:
        QString mFeatureName;
        QMap<QString, QVariant> mAttributes;
        QgsGeometry mGeom;
    };
    virtual QList<KadasPluginLayer::IdentifyResult> identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings ) { return QList<IdentifyResult>(); }

  protected:
    double mOpacity = 1.0;
    QgsCoordinateTransformContext mTransformContext;
};


class KADAS_CORE_EXPORT KadasPluginLayerType : public QObject, public QgsPluginLayerType
{
  public:
    KadasPluginLayerType( const QString &name )
      : QgsPluginLayerType( name ) {}

    virtual void addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const {};

  private:
};

#endif // KADASPLUGINLAYER_H
