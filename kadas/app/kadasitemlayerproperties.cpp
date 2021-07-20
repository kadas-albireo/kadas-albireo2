/***************************************************************************
    kadasitemlayerproperties.cpp
    ----------------------------
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

#include <qgis/qgsapplication.h>
#include <qgis/qgsrendererpropertiesdialog.h>
#include <qgis/qgsscalerangewidget.h>
#include <qgis/qgsstyle.h>
#include <qgis/qgssymbolwidgetcontext.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/app/kadasitemlayerproperties.h>


KadasItemLayerRenderingPropertiesWidget::KadasItemLayerRenderingPropertiesWidget( KadasItemLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : QgsMapLayerConfigWidget( layer, canvas, parent )
{
  mScaleRangeWidget = new QgsScaleRangeWidget( this );
  mScaleRangeWidget->setMapCanvas( canvas );
  mScaleRangeWidget->setEnabled( layer->hasScaleBasedVisibility() );
  mScaleRangeWidget->setMinimumScale( layer->minimumScale() );
  mScaleRangeWidget->setMaximumScale( layer->maximumScale() );

  mGroupBox = new QGroupBox( tr( "Scale Dependent Visibility" ) );
  mGroupBox->setCheckable( true );
  mGroupBox->setChecked( layer->hasScaleBasedVisibility() );
  mGroupBox->setLayout( new QVBoxLayout );
  mGroupBox->layout()->addWidget( mScaleRangeWidget );
  connect( mGroupBox, &QGroupBox::toggled, mScaleRangeWidget, &QgsScaleRangeWidget::setEnabled );

  setLayout( new QVBoxLayout );
  layout()->addWidget( mGroupBox );
  layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding ) );
}

void KadasItemLayerRenderingPropertiesWidget::apply()
{
  mLayer->setScaleBasedVisibility( mGroupBox->isChecked() );
  mLayer->setMinimumScale( mScaleRangeWidget->minimumScale() );
  mLayer->setMaximumScale( mScaleRangeWidget->maximumScale() );
}

KadasItemLayerProperties::KadasItemLayerProperties( KadasItemLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : KadasLayerPropertiesDialog( layer, parent )
{
  QListWidgetItem *item = new QListWidgetItem();
  item->setIcon( QgsApplication::getThemeIcon( "/propertyicons/rendering.svg" ) );
  item->setText( tr( "Rendering" ) );
  item->setToolTip( tr( "Rendering" ) );

  mOptionsListWidget->addItem( item );

  QgsMapLayerConfigWidget *itemLayerRenderingWidget = new KadasItemLayerRenderingPropertiesWidget( layer, canvas, this );
  mLayerPropertiesPages << itemLayerRenderingWidget;
  mOptionsStackedWidget->addWidget( itemLayerRenderingWidget );
}
