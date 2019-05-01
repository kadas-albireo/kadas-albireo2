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

#include <qgis/qgspluginlayer.h>

#include <kadas/core/kadas_core.h>

class KADAS_CORE_EXPORT KadasPluginLayer : public QgsPluginLayer
{
  Q_OBJECT

public:
   KadasPluginLayer( QString layerType, QString layerName );

    /** Get attributes at the specified map location */
    class IdentifyResult
    {
      public:
        IdentifyResult( const QString& featureName, const QMap<QString, QVariant>& attributes, const QgsGeometry& geom )
            : mFeatureName( featureName ), mAttributes( attributes ), mGeom( geom ) {}
        const QString& featureName() const { return mFeatureName; }
        const QMap<QString, QVariant>& attributes() const { return mAttributes; }
        const QgsGeometry& geometry() const { return mGeom; }

      private:
        QString mFeatureName;
        QMap<QString, QVariant> mAttributes;
        QgsGeometry mGeom;
    };
    virtual QList<IdentifyResult> identify( const QgsPoint& /*mapPos*/, const QgsMapSettings& /*mapSettings*/ ) { return QList<IdentifyResult>(); }

    /** Test for mouse pick. */
    virtual bool testPick( const QgsPointXY& /*mapPos*/, const QgsMapSettings& /*mapSettings*/, QVariant& /*pickResult*/, QRect &/*pickResultsExtent*/ ) { return false; }
    /** Handle a pick result. */
    virtual void handlePick( const QVariant& /*pick*/ ) {}

    /** return the current layer opacity */
    virtual double opacity() const { return mOpacity; }
    /** set the layer opacity */
    virtual void setOpacity( double opacity ) { mOpacity = opacity; }

    /** Get items in extent */
    virtual QVariantList getItems( const QgsRectangle& /*extent*/ ) const { return QVariantList(); }
    /** Delete the specified items */
    virtual void deleteItems( const QVariantList& /*items*/ ) {}
    /** Copy or cut the specified items */
    virtual void copyItems( const QVariantList& /*items*/, bool /*cut*/ ) {}

protected:
    double mOpacity = 1.0;
};

#endif // KADASPLUGINLAYER_H
