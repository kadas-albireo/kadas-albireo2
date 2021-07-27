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

#include <qgis/qgsproject.h>
#include <qgis/qgsmaplayer.h>

#include <kadas/gui/milx/kadasmilxclient.h>
#include <kadas/gui/milx/kadasmilxlayer.h>
#include <kadas/gui/milx/kadasmilxlayerpropertiespage.h>


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
  mWorkModeCombo->addItem( tr( "International" ), KadasMilxSymbolSettings::WorkModeInternational );
  mWorkModeCombo->addItem( tr( "CH" ), KadasMilxSymbolSettings::WorkModeCH );
  mWorkModeCombo->setCurrentIndex( layer->milxWorkMode() );

  QGridLayout *gridlayout = new QGridLayout();
  gridlayout->addWidget( new QLabel( tr( "Symbol size:" ) ), 0, 0 );
  gridlayout->addWidget( mSymbolSizeSlider, 0, 1 );
  gridlayout->addWidget( new QLabel( tr( "Line width:" ) ), 1, 0 );
  gridlayout->addWidget( mLineWidthSlider, 1, 1 );
  gridlayout->addWidget( new QLabel( tr( "Work mode:" ) ), 2, 0 );
  gridlayout->addWidget( mWorkModeCombo, 2, 1 );

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
  mLayer->setMilxWorkMode( static_cast<KadasMilxSymbolSettings::WorkMode>( mWorkModeCombo->currentIndex() ) );
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
  milxLayer->setMilxWorkMode( static_cast<KadasMilxSymbolSettings::WorkMode>( elem.attribute( "milx_work_mode", QString::number( KadasMilxSymbolSettings::DefaultWorkMode ) ).toInt() ) );
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
  elem.setAttribute( "milx_work_mode", milxLayer->milxWorkMode() );
}
