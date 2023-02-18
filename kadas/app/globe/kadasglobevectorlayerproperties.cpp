/***************************************************************************
    kadasglobevectorlayerproperties.cpp
    -----------------------------------
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

#include <qgis/qgsproject.h>
#include <qgis/qgsmaplayer.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/app/globe/kadasglobevectorlayerproperties.h>


Q_DECLARE_METATYPE( KadasGlobeVectorLayerConfig * )

KadasGlobeVectorLayerConfig *KadasGlobeVectorLayerConfig::getConfig( QgsMapLayer *layer )
{
  KadasGlobeVectorLayerConfig *layerConfig = layer->property( "globe-config" ).value<KadasGlobeVectorLayerConfig *>();
  if ( !layerConfig && ( layer->type() == QgsMapLayerType::VectorLayer || qobject_cast<KadasItemLayer *>( layer ) ) )
  {
    layerConfig = new KadasGlobeVectorLayerConfig( layer );
    layer->setProperty( "globe-config", QVariant::fromValue<KadasGlobeVectorLayerConfig *>( layerConfig ) );
  }
  return layerConfig;
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeVectorLayerPropertiesPage::KadasGlobeVectorLayerPropertiesPage( QgsMapLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : QgsMapLayerConfigWidget( layer, canvas, parent )
  , mLayer( layer )
{
  setupUi( this );

  // Populate combo boxes
  comboBoxRenderingMode->addItem( tr( "Rasterized" ), KadasGlobeVectorLayerConfig::RenderingModeRasterized );
  comboBoxRenderingMode->addItem( tr( "Model (Simple)" ), KadasGlobeVectorLayerConfig::RenderingModeModelSimple );
  comboBoxRenderingMode->addItem( tr( "Model (Advanced)" ), KadasGlobeVectorLayerConfig::RenderingModeModelAdvanced );
  comboBoxRenderingMode->setItemData( 0, tr( "Rasterize the layer to a texture, and drape it on the terrain" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setItemData( 1, tr( "Render the layer features as models" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setCurrentIndex( -1 );

  comboBoxAltitudeClamping->addItem( tr( "None" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_NONE ) );
  comboBoxAltitudeClamping->addItem( tr( "Terrain" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN ) );
  comboBoxAltitudeClamping->addItem( tr( "Relative" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN ) );
  comboBoxAltitudeClamping->addItem( tr( "Absolute" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_ABSOLUTE ) );
  comboBoxAltitudeClamping->setItemData( 0, tr( "Do not clamp Z values to the terrain (but still apply the offset, if applicable)" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setItemData( 1, tr( "Sample the terrain under the point, and set the feature's Z to the terrain height, ignoring the feature's original Z value" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setItemData( 2, tr( "Sample the terrain under the point, and add the terrain height to the feature's original Z value" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setItemData( 3, tr( "The feature's Z value describes its height above \"height zero\", which is typically the ellipsoid or MSL" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setCurrentIndex( -1 );

  comboBoxAltitudeTechnique->addItem( tr( "Map" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_MAP ) );
  comboBoxAltitudeTechnique->addItem( tr( "Drape" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE ) );
  comboBoxAltitudeTechnique->addItem( tr( "GPU" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_GPU ) );
  comboBoxAltitudeTechnique->addItem( tr( "Scene" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_SCENE ) );
  comboBoxAltitudeTechnique->setItemData( 0, tr( "Clamp geometry to the map model's elevation data" ), Qt::ToolTipRole );
  comboBoxAltitudeTechnique->setItemData( 1, tr( "Clamp geometry to the terrain's scene graph" ), Qt::ToolTipRole );
  comboBoxAltitudeTechnique->setItemData( 2, tr( "Clamp geometry to the terrain as they are rendered by the GPU" ), Qt::ToolTipRole );
  comboBoxAltitudeTechnique->setItemData( 3, tr( "Clamp geometry at draw time using projective texturing" ), Qt::ToolTipRole );
  comboBoxAltitudeTechnique->setCurrentIndex( -1 );

  comboBoxAltitudeBinding->addItem( tr( "Vertex" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::BINDING_VERTEX ) );
  comboBoxAltitudeBinding->addItem( tr( "Centroid" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::BINDING_CENTROID ) );
  comboBoxAltitudeBinding->setItemData( 0, tr( "Clamp every vertex independently" ), Qt::ToolTipRole );
  comboBoxAltitudeBinding->setItemData( 1, tr( "Clamp to the centroid of the entire geometry" ), Qt::ToolTipRole );
  comboBoxAltitudeBinding->setCurrentIndex( -1 );

  // Connect signals (setCurrentIndex(-1) above ensures the signal is called when the current values are set below)
  connect( comboBoxRenderingMode, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasGlobeVectorLayerPropertiesPage::showRenderingModeWidget );
  connect( comboBoxAltitudeClamping, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasGlobeVectorLayerPropertiesPage::onAltitudeClampingChanged );
  connect( comboBoxAltitudeTechnique, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasGlobeVectorLayerPropertiesPage::onAltituteTechniqueChanged );

  // Set values
  KadasGlobeVectorLayerConfig *layerConfig = KadasGlobeVectorLayerConfig::getConfig( mLayer );

  comboBoxRenderingMode->setCurrentIndex( comboBoxRenderingMode->findData( static_cast<int>( layerConfig->renderingMode ) ) );

  comboBoxAltitudeClamping->setCurrentIndex( comboBoxAltitudeClamping->findData( static_cast<int>( layerConfig->altitudeClamping ) ) );
  comboBoxAltitudeTechnique->setCurrentIndex( comboBoxAltitudeTechnique->findData( static_cast<int>( layerConfig->altitudeTechnique ) ) );
  comboBoxAltitudeBinding->setCurrentIndex( comboBoxAltitudeBinding->findData( static_cast<int>( layerConfig->altitudeBinding ) ) );

  spinBoxAltitudeOffset->setValue( layerConfig->verticalOffset );
  spinBoxAltitudeScale->setValue( layerConfig->verticalScale );
  spinBoxAltitudeResolution->setValue( layerConfig->clampingResolution );

  groupBoxExtrusion->setChecked( layerConfig->extrusionEnabled );
  labelExtrusionHeight->setText( layerConfig->extrusionHeight );
  checkBoxExtrusionFlatten->setChecked( layerConfig->extrusionFlatten );
  spinBoxExtrusionWallGradient->setValue( layerConfig->extrusionWallGradient );

  // TODO: labeling broken with osgEarth 2.8
  groupBoxLabelingEnabled->setChecked( false );
  checkBoxLabelingDeclutter->setChecked( false );
  groupBoxLabelingEnabled->setVisible( false );
  checkBoxLabelingDeclutter->setVisible( false );
}

void KadasGlobeVectorLayerPropertiesPage::apply()
{
  KadasGlobeVectorLayerConfig *layerConfig = KadasGlobeVectorLayerConfig::getConfig( mLayer );

  layerConfig->renderingMode = static_cast<KadasGlobeVectorLayerConfig::RenderingMode>( comboBoxRenderingMode->currentData().toInt() );
  layerConfig->altitudeClamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( comboBoxAltitudeClamping->currentData().toInt() );
  layerConfig->altitudeTechnique = static_cast<osgEarth::Symbology::AltitudeSymbol::Technique>( comboBoxAltitudeTechnique->currentData().toInt() );
  layerConfig->altitudeBinding = static_cast<osgEarth::Symbology::AltitudeSymbol::Binding>( comboBoxAltitudeBinding->currentData().toInt() );

  layerConfig->verticalOffset = spinBoxAltitudeOffset->value();
  layerConfig->verticalScale = spinBoxAltitudeScale->value();
  layerConfig->clampingResolution = spinBoxAltitudeResolution->value();

  layerConfig->extrusionEnabled = groupBoxExtrusion->isChecked();
  layerConfig->extrusionHeight = labelExtrusionHeight->text();
  layerConfig->extrusionFlatten = checkBoxExtrusionFlatten->isChecked();
  layerConfig->extrusionWallGradient = spinBoxExtrusionWallGradient->value();

  layerConfig->labelingEnabled = groupBoxLabelingEnabled->isChecked();
  layerConfig->labelingDeclutter = checkBoxLabelingDeclutter->isChecked();
}

void KadasGlobeVectorLayerPropertiesPage::onAltitudeClampingChanged( int index )
{
  osgEarth::Symbology::AltitudeSymbol::Clamping clamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( comboBoxAltitudeClamping->itemData( index ).toInt() );

  bool terrainClamping = clamping == osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
  labelAltitudeTechnique->setVisible( terrainClamping );
  comboBoxAltitudeTechnique->setVisible( terrainClamping );
  onAltituteTechniqueChanged( comboBoxAltitudeTechnique->currentIndex() );
}

void KadasGlobeVectorLayerPropertiesPage::onAltituteTechniqueChanged( int index )
{
  osgEarth::Symbology::AltitudeSymbol::Clamping clamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( comboBoxAltitudeClamping->currentData().toInt() );
  osgEarth::Symbology::AltitudeSymbol::Technique technique = static_cast<osgEarth::Symbology::AltitudeSymbol::Technique>( comboBoxAltitudeTechnique->itemData( index ).toInt() );

  bool mapTechnique = technique == osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_MAP && clamping == osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
  labelAltitudeBinding->setVisible( mapTechnique );
  comboBoxAltitudeBinding->setVisible( mapTechnique );
  labelAltitudeResolution->setVisible( mapTechnique );
  spinBoxAltitudeResolution->setVisible( mapTechnique );
}

void KadasGlobeVectorLayerPropertiesPage::showRenderingModeWidget( int index )
{
  stackedWidgetRenderingMode->setCurrentIndex( index != 0 );
  bool advanced = index == 2;
  groupBoxAltitude->setVisible( advanced );
  checkBoxExtrusionFlatten->setVisible( advanced );
  if ( !advanced )
  {
    comboBoxAltitudeClamping->setCurrentIndex( comboBoxAltitudeClamping->findData( static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_NONE ) ) );
    spinBoxAltitudeResolution->setValue( 0 );
    spinBoxAltitudeOffset->setValue( 0 );
    spinBoxAltitudeScale->setValue( 1 );
    checkBoxExtrusionFlatten->setChecked( false );
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeLayerPropertiesFactory::KadasGlobeLayerPropertiesFactory( QObject *parent )
  : QObject( parent )
{
  connect( QgsProject::instance(), &QgsProject::readMapLayer, this, &KadasGlobeLayerPropertiesFactory::readGlobeVectorLayerConfig );
  connect( QgsProject::instance(), &QgsProject::writeMapLayer, this, &KadasGlobeLayerPropertiesFactory::writeGlobeVectorLayerConfig );
}

QgsMapLayerConfigWidget *KadasGlobeLayerPropertiesFactory::createWidget( QgsMapLayer *layer, QgsMapCanvas *canvas, bool dockWidget, QWidget *parent ) const
{
  Q_UNUSED( dockWidget )
  return new KadasGlobeVectorLayerPropertiesPage( layer, canvas, parent );
}

QIcon KadasGlobeLayerPropertiesFactory::icon() const
{
  return QIcon( ":/kadas/icons/globe_color" );
}

QString KadasGlobeLayerPropertiesFactory::title() const
{
  return tr( "Globe" );
}

bool KadasGlobeLayerPropertiesFactory::supportsLayer( QgsMapLayer *layer ) const
{
  return layer->type() == QgsMapLayerType::VectorLayer || ( qobject_cast<KadasItemLayer *>( layer ) && !qobject_cast<KadasMilxLayer *>( layer ) );
}

void KadasGlobeLayerPropertiesFactory::readGlobeVectorLayerConfig( QgsMapLayer *mapLayer, const QDomElement &elem )
{
  KadasGlobeVectorLayerConfig *config = KadasGlobeVectorLayerConfig::getConfig( mapLayer );
  if ( !config )
  {
    return;
  }

  QDomElement globeElem = elem.firstChildElement( "globe" );
  if ( !globeElem.isNull() )
  {
    QDomElement renderingModeElem = globeElem.firstChildElement( "renderingMode" );
    config->renderingMode = static_cast<KadasGlobeVectorLayerConfig::RenderingMode>( renderingModeElem.attribute( "mode" ).toInt() );

    QDomElement modelRenderingElem = globeElem.firstChildElement( "modelRendering" );
    if ( !modelRenderingElem.isNull() )
    {
      QDomElement altitudeElem = modelRenderingElem.firstChildElement( "altitude" );
      config->altitudeClamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( altitudeElem.attribute( "clamping" ).toInt() );
      config->altitudeTechnique = static_cast<osgEarth::Symbology::AltitudeSymbol::Technique>( altitudeElem.attribute( "technique" ).toInt() );
      config->altitudeBinding = static_cast<osgEarth::Symbology::AltitudeSymbol::Binding>( altitudeElem.attribute( "binding" ).toInt() );
      config->verticalOffset = altitudeElem.attribute( "verticalOffset" ).toFloat();
      config->verticalScale = altitudeElem.attribute( "verticalScale" ).toFloat();
      config->clampingResolution = altitudeElem.attribute( "clampingResolution" ).toFloat();

      QDomElement extrusionElem = modelRenderingElem.firstChildElement( "extrusion" );
      config->extrusionEnabled = extrusionElem.attribute( "enabled" ).toInt() == 1;
      config->extrusionHeight = extrusionElem.attribute( "height", QString( "10" ) ).trimmed();
      if ( config->extrusionHeight.isEmpty() )
        config->extrusionHeight = "10";
      config->extrusionFlatten = extrusionElem.attribute( "flatten" ).toInt() == 1;
      config->extrusionWallGradient = extrusionElem.attribute( "wall-gradient" ).toDouble();

      QDomElement labelingElem = modelRenderingElem.firstChildElement( "labeling" );
      config->labelingEnabled = labelingElem.attribute( "enabled", "0" ).toInt() == 1;
      config->labelingField = labelingElem.attribute( "field" );
      config->labelingDeclutter = labelingElem.attribute( "declutter", "1" ).toInt() == 1;

      config->lightingEnabled = modelRenderingElem.attribute( "lighting", "1" ).toInt() == 1;
    }
  }
}

void KadasGlobeLayerPropertiesFactory::writeGlobeVectorLayerConfig( QgsMapLayer *mapLayer, QDomElement &elem, QDomDocument &doc )
{
  KadasGlobeVectorLayerConfig *config = KadasGlobeVectorLayerConfig::getConfig( mapLayer );
  if ( !config )
  {
    return;
  }

  QDomElement globeElem = doc.createElement( "globe" );

  QDomElement renderingModeElem = doc.createElement( "renderingMode" );
  renderingModeElem.setAttribute( "mode", config->renderingMode );
  globeElem.appendChild( renderingModeElem );

  QDomElement modelRenderingElem = doc.createElement( "modelRendering" );

  QDomElement altitudeElem = doc.createElement( "altitude" );
  altitudeElem.setAttribute( "clamping", config->altitudeClamping );
  altitudeElem.setAttribute( "technique", config->altitudeTechnique );
  altitudeElem.setAttribute( "binding", config->altitudeBinding );
  altitudeElem.setAttribute( "verticalOffset", config->verticalOffset );
  altitudeElem.setAttribute( "verticalScale", config->verticalScale );
  altitudeElem.setAttribute( "clampingResolution", config->clampingResolution );
  modelRenderingElem.appendChild( altitudeElem );

  QDomElement extrusionElem = doc.createElement( "extrusion" );
  extrusionElem.setAttribute( "enabled", config->extrusionEnabled );
  extrusionElem.setAttribute( "height", config->extrusionHeight );
  extrusionElem.setAttribute( "flatten", config->extrusionFlatten );
  extrusionElem.setAttribute( "wall-gradient", config->extrusionWallGradient );
  modelRenderingElem.appendChild( extrusionElem );

  QDomElement labelingElem = doc.createElement( "labeling" );
  labelingElem.setAttribute( "enabled", config->labelingEnabled );
  labelingElem.setAttribute( "field", config->labelingField );
  labelingElem.setAttribute( "declutter", config->labelingDeclutter );
  modelRenderingElem.appendChild( labelingElem );

  modelRenderingElem.setAttribute( "lighting", config->lightingEnabled );

  globeElem.appendChild( modelRenderingElem );

  elem.appendChild( globeElem );
}
