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

#include <qgis/qgsrendererpropertiesdialog.h>
#include <qgis/qgsstyle.h>
#include <qgis/qgssymbolwidgetcontext.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/app/kadasvectorlayerproperties.h>

KadasVectorLayerProperties::KadasVectorLayerProperties( QgsVectorLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : KadasLayerPropertiesDialog( layer, parent )
{
  mRendererDialog = new QgsRendererPropertiesDialog( layer, QgsStyle::defaultStyle(), true, this );
  mRendererDialog->setDockMode( false );
  QgsSymbolWidgetContext context;
  context.setMapCanvas( canvas );
  mRendererDialog->setContext( context );

  mOptionsStackedWidget->addWidget( mRendererDialog );
  mOptionsStackedWidget->setCurrentWidget( mRendererDialog );

  mOptionsListWidget->addItem( new QListWidgetItem( QIcon( ":/images/themes/default/propertyicons/symbology.svg" ), tr( "Symbology" ) ) );
}
void KadasVectorLayerProperties::apply()
{
  mRendererDialog->apply();
  KadasLayerPropertiesDialog::apply();
}
