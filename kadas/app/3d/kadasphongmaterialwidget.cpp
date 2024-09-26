/***************************************************************************
  qgsphongmaterialwidget.cpp
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadasphongmaterialwidget.h"

#include "qgsphongmaterialsettings.h"
#include "qgis.h"

KadasPhongMaterialWidget::KadasPhongMaterialWidget( QWidget *parent, bool hasOpacity )
  : QgsMaterialSettingsWidget( parent )
  , mHasOpacity( hasOpacity )
{
  setupUi( this );
  mOpacityWidget->setVisible( mHasOpacity );
  mLblOpacity->setVisible( mHasOpacity );
  spinShininess->setClearValue( 0, tr( "None" ) );

  QgsPhongMaterialSettings defaultMaterial;
  setSettings( &defaultMaterial, nullptr );

  connect( btnDiffuse, &QgsColorButton::colorChanged, this, &KadasPhongMaterialWidget::changed );
  connect( btnAmbient, &QgsColorButton::colorChanged, this, &KadasPhongMaterialWidget::changed );
  connect( btnSpecular, &QgsColorButton::colorChanged, this, &KadasPhongMaterialWidget::changed );
  connect( spinShininess, static_cast<void ( QDoubleSpinBox::* )( double )>( &QDoubleSpinBox::valueChanged ), this, [ = ]
  {
    updateWidgetState();
    emit changed();
  } );
  connect( mAmbientDataDefinedButton, &QgsPropertyOverrideButton::changed, this, &KadasPhongMaterialWidget::changed );
  connect( mDiffuseDataDefinedButton, &QgsPropertyOverrideButton::changed, this, &KadasPhongMaterialWidget::changed );
  connect( mSpecularDataDefinedButton, &QgsPropertyOverrideButton::changed, this, &KadasPhongMaterialWidget::changed );

  mAmbientCoefficientWidget->setToolTip( tr( "Sets the strength of the ambient color contribution" ) );
  mDiffuseCoefficientWidget->setToolTip( tr( "Sets the strength of the diffuse color contribution" ) );
  mSpecularCoefficientWidget->setToolTip( tr( "Sets the strength of the specular color contribution" ) );

  connect( mAmbientCoefficientWidget, &QgsPercentageWidget::valueChanged, this, &KadasPhongMaterialWidget::changed );
  connect( mDiffuseCoefficientWidget, &QgsPercentageWidget::valueChanged, this, &KadasPhongMaterialWidget::changed );
  connect( mSpecularCoefficientWidget, &QgsPercentageWidget::valueChanged, this, &KadasPhongMaterialWidget::changed );

  if ( mHasOpacity )
  {
    connect( mOpacityWidget, &QgsOpacityWidget::opacityChanged, this, &KadasPhongMaterialWidget::changed );
  }
}

QgsMaterialSettingsWidget *KadasPhongMaterialWidget::create()
{
  return new KadasPhongMaterialWidget();
}

void KadasPhongMaterialWidget::setTechnique( QgsMaterialSettingsRenderingTechnique technique )
{
  switch ( technique )
  {
    case QgsMaterialSettingsRenderingTechnique::Triangles:
    case QgsMaterialSettingsRenderingTechnique::TrianglesFromModel:
    case QgsMaterialSettingsRenderingTechnique::InstancedPoints:
    case QgsMaterialSettingsRenderingTechnique::Points:
    {
      lblDiffuse->setVisible( true );
      btnDiffuse->setVisible( true );
      mDiffuseCoefficientWidget->setVisible( true );
      mAmbientDataDefinedButton->setVisible( false );
      mDiffuseDataDefinedButton->setVisible( false );
      mSpecularDataDefinedButton->setVisible( false );
      break;
    }

    case QgsMaterialSettingsRenderingTechnique::TrianglesWithFixedTexture:
    {
      lblDiffuse->setVisible( false );
      btnDiffuse->setVisible( false );
      mDiffuseCoefficientWidget->setVisible( false );
      mAmbientDataDefinedButton->setVisible( false );
      mDiffuseDataDefinedButton->setVisible( false );
      mSpecularDataDefinedButton->setVisible( false );
      break;
    }

    case QgsMaterialSettingsRenderingTechnique::TrianglesDataDefined:
    {
      lblDiffuse->setVisible( true );
      btnDiffuse->setVisible( true );
      mDiffuseCoefficientWidget->setVisible( true );
      mAmbientDataDefinedButton->setVisible( true );
      mDiffuseDataDefinedButton->setVisible( true );
      mSpecularDataDefinedButton->setVisible( true );
      break;
    }

    case QgsMaterialSettingsRenderingTechnique::Lines:
      // not supported
      break;
  }
}

void KadasPhongMaterialWidget::setSettings( const QgsAbstractMaterialSettings *settings, QgsVectorLayer *layer )
{
  const QgsPhongMaterialSettings *phongMaterial = dynamic_cast< const QgsPhongMaterialSettings * >( settings );
  if ( !phongMaterial )
    return;
  btnDiffuse->setColor( phongMaterial->diffuse() );
  btnAmbient->setColor( phongMaterial->ambient() );
  btnSpecular->setColor( phongMaterial->specular() );
  spinShininess->setValue( phongMaterial->shininess() );
  mOpacityWidget->setOpacity( phongMaterial->opacity() );

  mAmbientCoefficientWidget->setValue( phongMaterial->ambientCoefficient() );
  mDiffuseCoefficientWidget->setValue( phongMaterial->diffuseCoefficient() );
  mSpecularCoefficientWidget->setValue( phongMaterial->specularCoefficient() );

  btnSpecular->setEnabled( phongMaterial->shininess() > 0 );

  mPropertyCollection = settings->dataDefinedProperties();

  mDiffuseDataDefinedButton->init( static_cast< int >( QgsAbstractMaterialSettings::Property::Diffuse ), mPropertyCollection, settings->propertyDefinitions(), layer, true );
  mAmbientDataDefinedButton->init( static_cast< int >( QgsAbstractMaterialSettings::Property::Ambient ), mPropertyCollection, settings->propertyDefinitions(), layer, true );
  mSpecularDataDefinedButton->init( static_cast< int >( QgsAbstractMaterialSettings::Property::Specular ), mPropertyCollection, settings->propertyDefinitions(), layer, true );

  updateWidgetState();
}

QgsAbstractMaterialSettings *KadasPhongMaterialWidget::settings()
{
  std::unique_ptr< QgsPhongMaterialSettings > m = std::make_unique< QgsPhongMaterialSettings >();
  m->setDiffuse( btnDiffuse->color() );
  m->setAmbient( btnAmbient->color() );
  m->setSpecular( btnSpecular->color() );
  m->setShininess( static_cast<float>( spinShininess->value() ) );

  m->setAmbientCoefficient( mAmbientCoefficientWidget->value() );
  m->setDiffuseCoefficient( mDiffuseCoefficientWidget->value() );
  m->setSpecularCoefficient( mSpecularCoefficientWidget->value() );

  float opacity = mHasOpacity ? static_cast<float>( mOpacityWidget->opacity() ) : 1.0f;
  m->setOpacity( opacity );

  mPropertyCollection.setProperty( QgsAbstractMaterialSettings::Property::Diffuse, mDiffuseDataDefinedButton->toProperty() );
  mPropertyCollection.setProperty( QgsAbstractMaterialSettings::Property::Ambient, mAmbientDataDefinedButton->toProperty() );
  mPropertyCollection.setProperty( QgsAbstractMaterialSettings::Property::Specular, mSpecularDataDefinedButton->toProperty() );
  m->setDataDefinedProperties( mPropertyCollection );

  return m.release();
}

void KadasPhongMaterialWidget::setHasOpacity( const bool opacity )
{
  if ( mHasOpacity == opacity )
  {
    return;
  }

  mHasOpacity = opacity;
  mOpacityWidget->setVisible( mHasOpacity );
  mLblOpacity->setVisible( mHasOpacity );
  if ( mHasOpacity )
  {
    connect( mOpacityWidget, &QgsOpacityWidget::opacityChanged, this, &KadasPhongMaterialWidget::changed );
  }
  else
  {
    disconnect( mOpacityWidget, &QgsOpacityWidget::opacityChanged, this, &KadasPhongMaterialWidget::changed );
  }
}

void KadasPhongMaterialWidget::updateWidgetState()
{
  if ( spinShininess->value() > 0 )
  {
    btnSpecular->setEnabled( true );
    btnSpecular->setToolTip( QString() );
  }
  else
  {
    btnSpecular->setEnabled( false );
    btnSpecular->setToolTip( tr( "Specular color is disabled because material has no shininess" ) );
  }
}
