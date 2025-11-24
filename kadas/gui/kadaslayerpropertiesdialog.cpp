/***************************************************************************
    kadaslayerpropertiesdialog.cpp
    ------------------------------
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

#include <QPushButton>

#include <qgis/qgsmaplayer.h>
#include <qgis/qgsmaplayerconfigwidget.h>
#include <qgis/qgsmaplayerconfigwidgetfactory.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/kadaslayerpropertiesdialog.h"

KadasLayerPropertiesDialog::KadasLayerPropertiesDialog( QgsMapLayer *layer, QWidget *parent )
  : QDialog( parent )
  , mLayer( layer )
{
  mUi.setupUi( this );

  connect( mUi.mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( mUi.mButtonBox, &QDialogButtonBox::accepted, this, &KadasLayerPropertiesDialog::apply );
  connect( mUi.mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  connect( mUi.mButtonBox->button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &KadasLayerPropertiesDialog::apply );
  connect( this, &QDialog::accepted, this, &KadasLayerPropertiesDialog::apply );
}

void KadasLayerPropertiesDialog::addPropertiesPageFactory( QgsMapLayerConfigWidgetFactory *factory )
{
  if ( !factory->supportsLayer( mLayer ) || !factory->supportLayerPropertiesDialog() )
  {
    return;
  }

  QListWidgetItem *item = new QListWidgetItem();
  item->setIcon( factory->icon() );
  item->setText( factory->title() );
  item->setToolTip( factory->title() );

  mUi.mOptionsListWidget->addItem( item );

  QgsMapLayerConfigWidget *page = factory->createWidget( mLayer, nullptr, false, this );
  mLayerPropertiesPages << page;
  mUi.mOptionsStackedWidget->addWidget( page );
}

void KadasLayerPropertiesDialog::apply()
{
  for ( QgsMapLayerConfigWidget *page : mLayerPropertiesPages )
  {
    page->apply();
  }

  mLayer->triggerRepaint();
  // notify the project we've made a change
  QgsProject::instance()->setDirty( true );
}
