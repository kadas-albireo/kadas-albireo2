/***************************************************************************
    kadaskmlexportdialog.cpp
    ------------------------
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

#include <qgis/qgslayertree.h>
#include <qgis/qgsvectorlayer.h>

#include <QPushButton>
#include <QFileDialog>
#include <QSettings>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/app/kml/kadaskmlexportdialog.h>

KadasKMLExportDialog::KadasKMLExportDialog( const QList<QgsMapLayer *> &activeLayers, QWidget *parent, Qt::WindowFlags f )
  : QDialog( parent, f )
{
  setupUi( this );

  // Use layerTreeRoot to get layers ordered as in the layer tree
  for ( QgsLayerTreeLayer *layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer *layer = layerTreeLayer->layer();
    if ( !layer )
      continue;
    QFile file( layer->source() );
    bool largefile = file.exists() && file.size() > 50 * 1024 * 1024; // Local file larger than 50 MB
    bool slow = largefile || layer->source().contains( "url=http" );
    QListWidgetItem *item = new QListWidgetItem( layer->name() );
    item->setCheckState( slow || !activeLayers.contains( layer ) ? Qt::Unchecked : Qt::Checked );
    item->setIcon( slow ? QIcon( ":/images/themes/default/mIconWarn.png" ) : QIcon() );
    item->setData( Qt::UserRole, layer->id() );
    mLayerListWidget->addItem( item );
  }
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
  mComboBoxExportScale->setScale( 1. / 25000 );

  connect( mFileSelectionButton, &QAbstractButton::clicked, this, &KadasKMLExportDialog::selectFile );
}

void KadasKMLExportDialog::selectFile()
{
  QStringList filters;
  filters.append( tr( "KMZ File (*.kmz)" ) );
  filters.append( tr( "KML File (*.kml)" ) );

  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getSaveFileName( 0, tr( "Select Output" ), lastDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  if ( selectedFilter == filters[0] && !filename.endsWith( ".kmz", Qt::CaseInsensitive ) )
  {
    filename += ".kmz";
  }
  else if ( selectedFilter == filters[1] && !filename.endsWith( ".kml", Qt::CaseInsensitive ) )
  {
    filename += ".kml";
  }
  mFileLineEdit->setText( filename );
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( true );

  // Toggle sensitivity of layers depending on whether KML or KMZ is selected
  if ( selectedFilter == filters[0] )
  {
    // KMZ, enable all layers
    for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
    {
      mLayerListWidget->item( i )->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
    }
  }
  else if ( selectedFilter == filters[1] )
  {
    // KML, disable non-vector and non-item layers
    for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
    {
      QString layerId = mLayerListWidget->item( i )->data( Qt::UserRole ).toString();
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
      if ( layer && !dynamic_cast<QgsVectorLayer *>( layer ) && !dynamic_cast<KadasItemLayer *>( layer ) )
      {
        mLayerListWidget->item( i )->setFlags( Qt::NoItemFlags );
        mLayerListWidget->item( i )->setCheckState( Qt::Unchecked );
      }
    }
  }
}

QList<QgsMapLayer *> KadasKMLExportDialog::getSelectedLayers() const
{
  QList<QgsMapLayer *> layerList;
  for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
  {
    QListWidgetItem *item = mLayerListWidget->item( i );
    if ( ( item->flags() & Qt::ItemIsEnabled ) && item->checkState() == Qt::Checked )
    {
      QString id = item->data( Qt::UserRole ).toString();
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( id );
      if ( layer )
      {
        layerList.append( layer );
      }
    }
  }
  return layerList;
}
