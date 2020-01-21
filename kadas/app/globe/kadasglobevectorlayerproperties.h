/***************************************************************************
    kadasglobevectorlayerproperties.h
    ---------------------------------
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

#ifndef KADASGLOBEVECTORLAYERPROPERTIES_H
#define KADASGLOBEVECTORLAYERPROPERTIES_H

#include <QIcon>

#include <osgEarthSymbology/AltitudeSymbol>

#include <qgis/qgsmaplayerconfigwidget.h>
#include <qgis/qgsmaplayerconfigwidgetfactory.h>

#include "ui_kadasglobevectorlayerproperties.h"

class QDomDocument;
class QDomElement;
class QListWidgetItem;
class QgsMapLayer;
class KadasGlobeVectorLayerConfig;

class KadasGlobeVectorLayerConfig : public QObject
{
    Q_OBJECT
  public:
    enum RenderingMode
    {
      RenderingModeRasterized,
      RenderingModeModelSimple,
      RenderingModeModelAdvanced
    };

    KadasGlobeVectorLayerConfig( QObject *parent = nullptr ) : QObject( parent )
    {
    }

    RenderingMode renderingMode = RenderingModeRasterized;
    osgEarth::Symbology::AltitudeSymbol::Clamping altitudeClamping = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
    osgEarth::Symbology::AltitudeSymbol::Technique altitudeTechnique = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
    osgEarth::Symbology::AltitudeSymbol::Binding altitudeBinding = osgEarth::Symbology::AltitudeSymbol::BINDING_VERTEX;

    float verticalOffset = 0.0;
    float verticalScale = 0.0;
    float clampingResolution = 0.0;

    bool extrusionEnabled = false;
    QString extrusionHeight = "10";
    bool extrusionFlatten = false;
    float extrusionWallGradient = 0.5;

    bool labelingEnabled = false;
    QString labelingField;
    bool labelingDeclutter = false;

    bool lightingEnabled = true;

    static KadasGlobeVectorLayerConfig *getConfig( QgsMapLayer *layer );
};


class KadasGlobeVectorLayerPropertiesPage : public QgsMapLayerConfigWidget, private Ui::KadasGlobeVectorLayerPropertiesPage
{
    Q_OBJECT

  public:
    explicit KadasGlobeVectorLayerPropertiesPage( QgsMapLayer *layer, QgsMapCanvas *canvas, QWidget *parent = nullptr );

  public slots:
    virtual void apply();

  private slots:
    void onAltitudeClampingChanged( int index );
    void onAltituteTechniqueChanged( int index );
    void showRenderingModeWidget( int index );

  signals:
    void layerSettingsChanged( QgsMapLayer * );

  private:
    QgsMapLayer *mLayer = nullptr;
};


class KadasGlobeLayerPropertiesFactory : public QObject, public QgsMapLayerConfigWidgetFactory
{
    Q_OBJECT
  public:
    explicit KadasGlobeLayerPropertiesFactory( QObject *parent = nullptr );
    QgsMapLayerConfigWidget *createWidget( QgsMapLayer *layer, QgsMapCanvas *canvas, bool dockWidget, QWidget *parent ) const override;

    QIcon icon() const override;

    QString title() const override;

    bool supportLayerPropertiesDialog() const override { return true; }

    bool supportsLayer( QgsMapLayer *layer ) const override;

  signals:
    void layerSettingsChanged( QgsMapLayer *layer );

  private slots:
    void readGlobeVectorLayerConfig( QgsMapLayer *mapLayer, const QDomElement &elem );
    void writeGlobeVectorLayerConfig( QgsMapLayer *mapLayer, QDomElement &elem, QDomDocument &doc );
};

#endif // KADASGLOBEVECTORLAYERPROPERTIES_H
