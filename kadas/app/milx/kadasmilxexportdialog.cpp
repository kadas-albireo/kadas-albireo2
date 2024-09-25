/***************************************************************************
  kadasmilxexportdialog.cpp
  --------------------------------------
  Date                 : September 2024
  Copyright            : (C) 2024 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <qgis/qgsproject.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadas/gui/milx/kadasmilxlayer.h"
#include "milx/kadasmilxexportdialog.h"

KadasMilxExportDialog::KadasMilxExportDialog( QWidget *parent )
  : QDialog( parent )
{
  ui.setupUi( this );

  setExportMode( ExportMode::Milx );

  ui.comboCartouche->addItem( tr( "Don't add" ) );

  for ( QgsMapLayer *layer : QgsProject::instance()->mapLayers().values() )
  {
    if ( qobject_cast<KadasMilxLayer *>( layer ) )
    {
      QListWidgetItem *item = new QListWidgetItem( layer->name() );
      item->setData( Qt::UserRole, layer->id() );
      item->setCheckState( KadasApplication::instance()->mainWindow()->mapCanvas()->layers().contains( layer ) ? Qt::Checked : Qt::Unchecked );
      ui.listWidget->addItem( item );
      ui.comboCartouche->addItem( layer->name(), layer->id() );
    }
  }
}

void KadasMilxExportDialog::setExportMode( ExportMode exportMode )
{
  switch (exportMode)
  {
  case ExportMode::Milx:
    ui.stackedWidgetOptions->setCurrentWidget( ui.stackedWidgetOptionsPageMilx );
    break;
  case ExportMode::Kml:
    ui.stackedWidgetOptions->setCurrentWidget( ui.stackedWidgetOptionsPageKml );
    break;
  }
}

void KadasMilxExportDialog::setVersions( const QStringList &versionNames, const QStringList &versionTags )
{
  ui.comboMilxVersion->clear();

  for ( int i = 0, n = versionTags.size(); i < n; ++i )
  {
    ui.comboMilxVersion->addItem( versionNames[i], versionTags[i] );
  }
  ui.comboMilxVersion->setCurrentIndex( 0 );
}

QStringList KadasMilxExportDialog::selectedLayers() const
{
  QStringList exportLayers;
  for ( int i = 0, n = ui.listWidget->count(); i < n; ++i )
  {
    QListWidgetItem *item = ui.listWidget->item( i );
    if ( item->checkState() == Qt::Checked )
    {
      exportLayers.append( item->data( Qt::UserRole ).toString() );
    }
  }
  return exportLayers;
}

QString KadasMilxExportDialog::selectedCartoucheLayerId() const
{
  return ui.comboCartouche->currentData().toString();
}

QString KadasMilxExportDialog::selectedVersionTag() const
{
  return ui.comboMilxVersion->currentData().toString();
}



