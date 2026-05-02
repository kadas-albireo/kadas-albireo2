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
#include <QVBoxLayout>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgscolorbutton.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasmilxlayersettings.h"
#include "kadas/gui/milx/kadasmilxclient.h"
#include "kadas/gui/milx/kadasmilxlayer.h"
#include "kadas/gui/milx/kadasmilxlayerpropertiespage.h"


namespace
{
  bool isMilxAnnotationLayer( const QgsMapLayer *layer )
  {
    const auto *anno = qobject_cast<const QgsAnnotationLayer *>( layer );
    if ( !anno )
      return false;
    const QMap<QString, QgsAnnotationItem *> items = anno->items();
    for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
    {
      if ( it.value() && it.value()->type() == QStringLiteral( "kadas:milx" ) )
        return true;
    }
    return false;
  }
} // namespace


KadasMilxLayerPropertiesPage::KadasMilxLayerPropertiesPage( QgsMapLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : QgsMapLayerConfigWidget( layer, canvas, parent )
  , mLayer( layer )
{
  const KadasMilxSymbolSettings settings = KadasMilxLayerSettings::layerSettings( mLayer );

  mSymbolSizeSlider = new QSlider( Qt::Horizontal );
  mSymbolSizeSlider->setRange( KadasMilxSymbolSettings::MinSymbolSize, KadasMilxSymbolSettings::MaxSymbolSize );
  mSymbolSizeSlider->setValue( settings.symbolSize );

  mLineWidthSlider = new QSlider( Qt::Horizontal );
  mLineWidthSlider->setRange( KadasMilxSymbolSettings::MinLineWidth, KadasMilxSymbolSettings::MaxLineWidth );
  mLineWidthSlider->setValue( settings.lineWidth );

  mWorkModeCombo = new QComboBox();
  mWorkModeCombo->addItem( tr( "International" ), QVariant::fromValue( KadasMilxSymbolSettings::WorkMode::WorkModeInternational ) );
  mWorkModeCombo->addItem( tr( "CH" ), QVariant::fromValue( KadasMilxSymbolSettings::WorkMode::WorkModeCH ) );
  mWorkModeCombo->setCurrentIndex( static_cast<int>( settings.workMode ) );

  mLeaderLineWidthSpin = new QSpinBox();
  mLeaderLineWidthSpin->setRange( 1, 10 );
  mLeaderLineWidthSpin->setSuffix( "px" );
  mLeaderLineWidthSpin->setValue( settings.leaderLineWidth );
  mLeaderLineColorButton = new QgsColorButton();
  mLeaderLineColorButton->setColor( settings.leaderLineColor );

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
  mGroupBox->setChecked( KadasMilxLayerSettings::overrideEnabled( mLayer ) );

  setLayout( new QVBoxLayout() );
  layout()->addWidget( mGroupBox );
  layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
}

void KadasMilxLayerPropertiesPage::apply()
{
  KadasMilxSymbolSettings settings;
  settings.symbolSize = mSymbolSizeSlider->value();
  settings.lineWidth = mLineWidthSlider->value();
  settings.workMode = mWorkModeCombo->currentData().value<KadasMilxSymbolSettings::WorkMode>();
  settings.leaderLineWidth = mLeaderLineWidthSpin->value();
  settings.leaderLineColor = mLeaderLineColorButton->color();
  KadasMilxLayerSettings::setLayerSettings( mLayer, settings );
  KadasMilxLayerSettings::setOverrideEnabled( mLayer, mGroupBox->isChecked() );

  if ( mLayer )
    mLayer->triggerRepaint();
}

///////////////////////////////////////////////////////////////////////////////

KadasMilxLayerPropertiesPageFactory::KadasMilxLayerPropertiesPageFactory( QObject *parent )
  : QObject( parent )
{
  // Legacy `KadasMilxLayer` projects stored MilX overrides as DOM
  // attributes on the maplayer element. Translate those onto the
  // legacy layer at read time so `KadasItemLayerMigration` can copy
  // them onto the annotation layer's custom properties. New annotation
  // layers persist the same information via `QgsMapLayer::customProperty`
  // (round-tripped automatically by Qgs).
  connect( QgsProject::instance(), &QgsProject::readMapLayer, this, &KadasMilxLayerPropertiesPageFactory::readLayerConfig );
}

QgsMapLayerConfigWidget *KadasMilxLayerPropertiesPageFactory::createWidget( QgsMapLayer *layer, QgsMapCanvas *canvas, bool dockWidget, QWidget *parent ) const
{
  Q_UNUSED( dockWidget )
  return new KadasMilxLayerPropertiesPage( layer, canvas, parent );
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
  return isMilxAnnotationLayer( layer );
}

void KadasMilxLayerPropertiesPageFactory::readLayerConfig( QgsMapLayer *mapLayer, const QDomElement &elem )
{
  KadasMilxLayer *milxLayer = qobject_cast<KadasMilxLayer *>( mapLayer );
  if ( !milxLayer )
    return;

  milxLayer->setOverrideMilxSymbolSettings( elem.attribute( "milx_override_symbol_settings" ).toInt() );
  milxLayer->setMilxSymbolSize( elem.attribute( "milx_symbol_size", QString::number( KadasMilxSymbolSettings::DefaultSymbolSize ) ).toInt() );
  milxLayer->setMilxLineWidth( elem.attribute( "milx_line_width", QString::number( KadasMilxSymbolSettings::DefaultLineWidth ) ).toInt() );
  milxLayer->setMilxWorkMode(
    static_cast<KadasMilxSymbolSettings::WorkMode>( elem.attribute( "milx_work_mode", QString::number( static_cast<int>( KadasMilxSymbolSettings::DefaultWorkMode ) ) ).toInt() )
  );
  milxLayer->setMilxLeaderLineWidth( elem.attribute( "milx_leader_line_width" ).toInt() );
  milxLayer->setMilxLeaderLineColor( elem.attribute( "milx_leader_line_color" ) );
}
