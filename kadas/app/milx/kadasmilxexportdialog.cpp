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


#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsproject.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadas/gui/annotationitems/kadasmilxannotationitem.h"
#include "milx/kadasmilxexportdialog.h"

KadasMilxExportDialog::KadasMilxExportDialog( QWidget *parent )
  : QDialog( parent )
{
  ui.setupUi( this );

  setExportMode( ExportMode::Milx );

  ui.comboCartouche->addItem( tr( "Don't add" ) );

  const QList<QgsMapLayer *> layers = QgsProject::instance()->mapLayers().values();
  for ( QgsMapLayer *layer : layers )
  {
    auto *annoLayer = qobject_cast<QgsAnnotationLayer *>( layer );
    if ( !annoLayer )
      continue;
    // Only show annotation layers that actually carry MilX symbols.
    bool hasMilx = false;
    const QMap<QString, QgsAnnotationItem *> items = annoLayer->items();
    for ( auto it = items.constBegin(); it != items.constEnd(); ++it )
    {
      if ( it.value() && it.value()->type() == KadasMilxAnnotationItem::itemTypeId() )
      {
        hasMilx = true;
        break;
      }
    }
    if ( !hasMilx )
      continue;
    QListWidgetItem *item = new QListWidgetItem( layer->name() );
    item->setData( Qt::UserRole, layer->id() );
    item->setCheckState( KadasApplication::instance()->mainWindow()->mapCanvas()->layers().contains( layer ) ? Qt::Checked : Qt::Unchecked );
    ui.listWidget->addItem( item );
    ui.comboCartouche->addItem( layer->name(), layer->id() );
  }
}

void KadasMilxExportDialog::setExportMode( ExportMode exportMode )
{
  switch ( exportMode )
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
