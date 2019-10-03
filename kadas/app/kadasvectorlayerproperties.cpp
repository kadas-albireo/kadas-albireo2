/***************************************************************************
    kadasvectorlayerproperties.cpp
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

#include <qgis/qgsmaplayerconfigwidget.h>
#include <qgis/qgsmaplayerconfigwidgetfactory.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendererpropertiesdialog.h>
#include <qgis/qgsstyle.h>
#include <qgis/qgssymbolwidgetcontext.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasvectorlayerproperties.h>

KadasVectorLayerProperties::KadasVectorLayerProperties( QgsVectorLayer *layer, QWidget *parent )
  : QDialog( parent )
  , mLayer( layer )
{
  setupUi( this );

  mRendererDialog = new QgsRendererPropertiesDialog( layer, QgsStyle::defaultStyle(), true, this );
  mRendererDialog->setDockMode( false );
  QgsSymbolWidgetContext context;
  context.setMapCanvas( kApp->mainWindow()->mapCanvas() );
  context.setMessageBar( kApp->mainWindow()->messageBar() );
  mRendererDialog->setContext( context );

  mOptionsStackedWidget->addWidget( mRendererDialog );
  mOptionsStackedWidget->setCurrentWidget( mRendererDialog );

  mOptionsListWidget->addItem( new QListWidgetItem( QIcon( ":/images/themes/default/propertyicons/symbology.svg" ), tr( "Symbology" ) ) );

  connect( mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( mButtonBox, &QDialogButtonBox::accepted, this, &KadasVectorLayerProperties::apply );
  connect( mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  connect( mButtonBox->button( QDialogButtonBox::Apply ), &QPushButton::clicked, this, &KadasVectorLayerProperties::apply );
  connect( this, &QDialog::accepted, this, &KadasVectorLayerProperties::apply );
}

void KadasVectorLayerProperties::addPropertiesPageFactory( QgsMapLayerConfigWidgetFactory *factory )
{
  if ( !factory->supportsLayer( mLayer ) || !factory->supportLayerPropertiesDialog() )
  {
    return;
  }

  QListWidgetItem *item = new QListWidgetItem();
  item->setIcon( factory->icon() );
  item->setText( factory->title() );
  item->setToolTip( factory->title() );

  mOptionsListWidget->addItem( item );

  QgsMapLayerConfigWidget *page = factory->createWidget( mLayer, nullptr, false, this );
  mLayerPropertiesPages << page;
  mOptionsStackedWidget->addWidget( page );
}

void KadasVectorLayerProperties::apply()
{
  mRendererDialog->apply();

  for ( QgsMapLayerConfigWidget *page : mLayerPropertiesPages )
  {
    page->apply();
  }

  mLayer->triggerRepaint();
  // notify the project we've made a change
  QgsProject::instance()->setDirty( true );
}
