/***************************************************************************
  qgsshadowrenderingsettingswidget.cpp
  --------------------------------------
  Date                 : September 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb uderscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadasshadowrenderingsettingswidget.h"

#include "qgis.h"
#include "qgs3d.h"
#include "kadaslightswidget.h"
#include "qgssettingsentryenumflag.h"

#include <QCheckBox>
#include <QLineEdit>


KadasShadowRenderingSettingsWidget::KadasShadowRenderingSettingsWidget( QWidget *parent )
  : QWidget( parent )
{
  setupUi( this );

  shadowRenderinMaximumDistanceSpinBox->setClearValue( 1500.00 );
  shadowBiasSpinBox->setClearValue( 0.000010 );
}

void KadasShadowRenderingSettingsWidget::setLightSourceModel( QgsLightsModel *model )
{
  mLightsModel = model;
  mLightsProxyModel = new QgsLightsProxyModel( this );
  mLightsProxyModel->setSourceModel( mLightsModel );
  mLightsProxyModel->setAllowedLightTypes( { Qgis::LightSourceType::Directional, Qgis::LightSourceType::Sun } );
  mLightSourceComboBox->setModel( mLightsProxyModel );
}

void KadasShadowRenderingSettingsWidget::setShadowSettings( const QgsShadowSettings &shadowSettings )
{
  if ( mLightsModel )
  {
    const QModelIndex sourceIndex = mLightsModel->indexFromLightId( shadowSettings.lightSource() );
    const QModelIndex proxyIndex = mLightsProxyModel->mapFromSource( sourceIndex );
    if ( proxyIndex.isValid() )
    {
      mLightSourceComboBox->setCurrentIndex( proxyIndex.row() );
    }
  }
  shadowRenderinMaximumDistanceSpinBox->setValue( shadowSettings.maximumShadowRenderingDistance() );
  shadowBiasSpinBox->setValue( shadowSettings.shadowBias() );
}

QgsShadowSettings KadasShadowRenderingSettingsWidget::toShadowSettings()
{
  QgsShadowSettings settings;
  settings.setLightSource( mLightSourceComboBox->currentData( QgsLightsModel::LightId ).toString() );
  settings.setMaximumShadowRenderingDistance( shadowRenderinMaximumDistanceSpinBox->value() );
  settings.setShadowBias( shadowBiasSpinBox->value() );
  settings.setShadowQuality( Qgs3D::settingShadowQuality->value() );
  return settings;
}
