/***************************************************************************
    kadasmilxlayerpropertiespage.cpp
    --------------------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

#include <qgis/qgscolorbutton.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsmaplayer.h>

#include "kadas/gui/milx/kadasmilxclient.h"
#include "kadas/gui/milx/kadasmilxlayer.h"
#include "kadas/gui/milx/kadasmilxlayerpropertiespage.h"


KadasMilxLayerPropertiesPage::KadasMilxLayerPropertiesPage( KadasMilxLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : QgsMapLayerConfigWidget( layer, canvas, parent )
  , mLayer( layer )
{
  mSymbolSizeSlider = new QSlider( Qt::Horizontal );
  mSymbolSizeSlider->setRange( KadasMilxSymbolSettings::MinSymbolSize, KadasMilxSymbolSettings::MaxSymbolSize );
  mSymbolSizeSlider->setValue( layer->milxSymbolSize() );

  mLineWidthSlider = new QSlider( Qt::Horizontal );
  mLineWidthSlider->setRange( KadasMilxSymbolSettings::MinLineWidth, KadasMilxSymbolSettings::MaxLineWidth );
  mLineWidthSlider->setValue( layer->milxLineWidth() );

  mWorkModeCombo = new QComboBox();
  mWorkModeCombo->addItem( tr( "International" ), QVariant::fromValue( KadasMilxSymbolSettings::WorkMode::WorkModeInternational ) );
  mWorkModeCombo->addItem( tr( "CH" ), QVariant::fromValue( KadasMilxSymbolSettings::WorkMode::WorkModeCH ) );
  mWorkModeCombo->setCurrentIndex( static_cast<int>( layer->milxWorkMode() ) );

  mLeaderLineWidthSpin = new QSpinBox();
  mLeaderLineWidthSpin->setRange( 1, 10 );
  mLeaderLineWidthSpin->setSuffix( "px" );
  mLeaderLineWidthSpin->setValue( layer->milxLeaderLineWidth() );
  mLeaderLineColorButton = new QgsColorButton();
  mLeaderLineColorButton->setColor( layer->milxLeaderLineColor() );

  QGridLayout *gridlayout = new QGridLayout();
  gridlayout->addWidget( new QLabel( tr( "Symbol size:" ) ), 0, 0 );
  gridlayout->addWidget( mSymbolSizeSlider, 0, 1, 1, 2 );
  gridlayout->addWidget( new QLabel( tr( "Line width:" ) ), 1, 0 );
  gridlayout->addWidget( mLineWidthSlider, 1, 1, 1, 2 );
  gridlayout->addWidget( new QLabel( tr( "Work mode:" ) ), 2, 0 );
  gridlayout->addWidget( mWorkModeCombo, 2, 1, 1, 2 );
  gridlayout->addWidget( new QLabel( tr( "Leader lines:" ) ), 3, 0 );
  gridlayout->addWidget( mLeaderLineWidthSpin, 3, 1 );
  gridlayout->addWidget( mLeaderLineColorButton, 3, 2 );

  mGroupBox = new QGroupBox( tr( "Override global symbol settings" ) );
  mGroupBox->setCheckable( true );
  mGroupBox->setLayout( gridlayout );
  mGroupBox->setChecked( layer->overrideMilxSymbolSettings() );

  setLayout( new QVBoxLayout() );
  layout()->addWidget( mGroupBox );
  layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
}

void KadasMilxLayerPropertiesPage::apply()
{
  mLayer->setOverrideMilxSymbolSettings( mGroupBox->isChecked() );
  mLayer->setMilxSymbolSize( mSymbolSizeSlider->value() );
  mLayer->setMilxLineWidth( mLineWidthSlider->value() );
  mLayer->setMilxWorkMode( mWorkModeCombo->currentData().value<KadasMilxSymbolSettings::WorkMode>() );
  mLayer->setMilxLeaderLineWidth( mLeaderLineWidthSpin->value() );
  mLayer->setMilxLeaderLineColor( mLeaderLineColorButton->color() );
}

///////////////////////////////////////////////////////////////////////////////

KadasMilxLayerPropertiesPageFactory::KadasMilxLayerPropertiesPageFactory( QObject *parent )
  : QObject( parent )
{
  connect( QgsProject::instance(), &QgsProject::readMapLayer, this, &KadasMilxLayerPropertiesPageFactory::readLayerConfig );
  connect( QgsProject::instance(), &QgsProject::writeMapLayer, this, &KadasMilxLayerPropertiesPageFactory::writeLayerConfig );
}

QgsMapLayerConfigWidget *KadasMilxLayerPropertiesPageFactory::createWidget( QgsMapLayer *layer, QgsMapCanvas *canvas, bool dockWidget, QWidget *parent ) const
{
  Q_UNUSED( dockWidget )
  return new KadasMilxLayerPropertiesPage( qobject_cast<KadasMilxLayer *>( layer ), canvas, parent );
}

QIcon KadasMilxLayerPropertiesPageFactory::icon() const
{
  return QIcon( ":/kadas/icons/mss" );
}

QString KadasMilxLayerPropertiesPageFactory::title() const
{
  return tr( "MSS" );
}

bool KadasMilxLayerPropertiesPageFactory::supportsLayer( QgsMapLayer *layer ) const
{
  return qobject_cast<KadasMilxLayer *>( layer );
}

void KadasMilxLayerPropertiesPageFactory::readLayerConfig( QgsMapLayer *mapLayer, const QDomElement &elem )
{
  KadasMilxLayer *milxLayer = qobject_cast<KadasMilxLayer *>( mapLayer );
  if ( !milxLayer )
  {
    return;
  }
  milxLayer->setOverrideMilxSymbolSettings( elem.attribute( "milx_override_symbol_settings" ).toInt() );
  milxLayer->setMilxSymbolSize( elem.attribute( "milx_symbol_size", QString::number( KadasMilxSymbolSettings::DefaultSymbolSize ) ).toInt() );
  milxLayer->setMilxLineWidth( elem.attribute( "milx_line_width", QString::number( KadasMilxSymbolSettings::DefaultLineWidth ) ).toInt() );
  milxLayer->setMilxWorkMode( static_cast<KadasMilxSymbolSettings::WorkMode>( elem.attribute( "milx_work_mode", QString::number( static_cast<int>( KadasMilxSymbolSettings::DefaultWorkMode ) ) ).toInt() ) );
  milxLayer->setMilxLeaderLineWidth( elem.attribute( "milx_leader_line_width" ).toInt() );
  milxLayer->setMilxLeaderLineColor( elem.attribute( "milx_leader_line_color" ) );
}

void KadasMilxLayerPropertiesPageFactory::writeLayerConfig( QgsMapLayer *mapLayer, QDomElement &elem, QDomDocument &doc )
{
  KadasMilxLayer *milxLayer = qobject_cast<KadasMilxLayer *>( mapLayer );
  if ( !milxLayer )
  {
    return;
  }
  elem.setAttribute( "milx_override_symbol_settings", milxLayer->overrideMilxSymbolSettings() );
  elem.setAttribute( "milx_symbol_size", milxLayer->milxSymbolSize() );
  elem.setAttribute( "milx_line_width", milxLayer->milxLineWidth() );
  elem.setAttribute( "milx_work_mode", static_cast<int>( milxLayer->milxWorkMode() ) );
  elem.setAttribute( "milx_leader_line_width", milxLayer->milxLeaderLineWidth() );
  elem.setAttribute( "milx_leader_line_color", milxLayer->milxLeaderLineColor().name() );
}
