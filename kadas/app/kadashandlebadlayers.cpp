/***************************************************************************
    kadashandlebadlayers.cpp
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

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>

#include <qgis/qgsproviderregistry.h>
#include <qgis/qgsproject.h>

#include "kadasapplication.h"
#include "kadashandlebadlayers.h"
#include "kadasmainwindow.h"


void KadasHandleBadLayersHandler::handleBadLayers( const QList<QDomNode> &layers )
{
  kApp->setOverrideCursor( Qt::ArrowCursor );
  KadasHandleBadLayers( layers ).exec();
  kApp->restoreOverrideCursor();
}


KadasHandleBadLayers::KadasHandleBadLayers( const QList<QDomNode> &layers )
  : QDialog( kApp->mainWindow() )
  , mLayers( layers )
{
  setWindowTitle( tr( "Invalid project layers" ) );

  setLayout( new QVBoxLayout );

  mLayerList = new QTableWidget();
  layout()->addWidget( mLayerList );
  layout()->setMargin( 2 );
  resize( 640, 240 );

  QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  layout()->addWidget( buttonBox );
  connect( buttonBox, &QDialogButtonBox::accepted, this, &KadasHandleBadLayers::accept );
  connect( buttonBox, &QDialogButtonBox::rejected, this, &KadasHandleBadLayers::reject );

  mLayerList->setSelectionBehavior( QAbstractItemView::SelectRows );
  mLayerList->setColumnCount( 4 );
  mLayerList->setColumnWidth( 3, 24 );
  mLayerList->setWordWrap( false );

  mLayerList->setHorizontalHeaderLabels( QStringList() << tr( "Layer name" ) << tr( "Type" ) << tr( "Datasource" ) << "" );
  mLayerList->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::Stretch );
  connect( mLayerList, &QTableWidget::itemClicked, this, &KadasHandleBadLayers::itemClicked );

  for ( int i = 0; i < mLayers.size(); i++ )
  {
    const QDomNode &node = mLayers[i];

    QString name = node.namedItem( QStringLiteral( "layername" ) ).toElement().text();
    QString type = node.toElement().attribute( QStringLiteral( "type" ) );
    QString datasource = QgsProject::instance()->readPath( node.namedItem( QStringLiteral( "datasource" ) ).toElement().text() );
    QString provider = node.namedItem( QStringLiteral( "provider" ) ).toElement().text();
    bool providerFileBased = ( provider == QStringLiteral( "gdal" ) || provider == QStringLiteral( "ogr" ) || provider == QStringLiteral( "mdal" ) );
    if ( provider == QStringLiteral( "delimitedtext" ) && datasource.startsWith( "file://" ) )
    {
      providerFileBased = true;
    }

    mLayerList->setRowCount( i + 1 );

    QTableWidgetItem *item = nullptr;

    item = new QTableWidgetItem( name );
    item->setData( LayerIndexRole, i );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    mLayerList->setItem( i, 0, item );

    item = new QTableWidgetItem( type );
    item->setData( ProviderRole, provider );
    item->setFlags( item->flags() & ~Qt::ItemIsEditable );
    mLayerList->setItem( i, 1, item );

    item = new QTableWidgetItem( datasource );
    mLayerList->setItem( i, 2, item );

    if ( providerFileBased )
    {
      item = new QTableWidgetItem( QgsApplication::getThemeIcon( "/mActionFileOpen.svg" ), QString() );
      item->setData( FileBasedRole, true );
      mLayerList->setItem( i, 3, item );
    }
    else
    {
      item = new QTableWidgetItem( QString() );
      item->setData( FileBasedRole, false );
      mLayerList->setItem( i, 3, item );
    }
  }
}

void KadasHandleBadLayers::itemClicked( QTableWidgetItem *item )
{
  if ( item->column() != 3 || !item->data( FileBasedRole ).toBool() )
  {
    return;
  }
  QString filename = mLayerList->item( item->row(), 2 )->text();
  QString provider = mLayerList->item( item->row(), 1 )->data( ProviderRole ).toString();
  QUrl url;
  QVariantMap parts;

  QString type = mLayerList->item( item->row(), 1 )->text();
  QString fileFilter;
  if ( provider == QStringLiteral( "delimitedtext" ) )
  {
    url = QUrl( filename );
    filename = url.host() + "/" + url.path();
    fileFilter = tr( "Text files" ) + QStringLiteral( " (*.txt *.csv *.dat *.wkt);;" );
  }
  else
  {
    if ( type == QStringLiteral( "vector" ) )
    {
      fileFilter = QgsProviderRegistry::instance()->fileVectorFilters();
    }
    else
    {
      fileFilter = QgsProviderRegistry::instance()->fileRasterFilters();
    }

    parts = QgsProviderRegistry::instance()->decodeUri( provider, filename );
    filename = parts.isEmpty() ? filename : parts.value( "path" ).toString();
  }

  filename = QFileDialog::getOpenFileName( this, tr( "Select File" ), QFileInfo( filename ).filePath(), fileFilter );
  if ( filename.isEmpty() )
  {
    return;
  }

  if ( !parts.isEmpty() )
  {
    parts["path"] = filename;
    filename = QgsProviderRegistry::instance()->encodeUri( provider, parts );
  }
  if ( !url.isEmpty() )
  {
    QUrl fileNameUrl( QString( "file:%1" ).arg( filename ) );
    url.setHost( fileNameUrl.host() );
    url.setPath( fileNameUrl.path() );
    filename = url.toString();
  }
  mLayerList->item( item->row(), 2 )->setText( filename );
}

void KadasHandleBadLayers::accept()
{
  int unfixed = 0;

  for ( int i = 0; i < mLayerList->rowCount(); i++ )
  {
    int idx = mLayerList->item( i, 0 )->data( LayerIndexRole ).toInt();
    QDomNode &node = const_cast<QDomNode &>( mLayers[ idx ] );

    QString name = mLayerList->item( i, 0 )->text();
    QString datasource = mLayerList->item( i, 2 )->text();

    QString layerId = node.namedItem( "id" ).toElement().text();
    QString provider = node.namedItem( "provider" ).toElement().text();

    QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
    bool dataSourceFixed = false;
    if ( layer )
    {
      QgsDataProvider::ProviderOptions options;
      QString path = QgsProject::instance()->pathResolver().readPath( datasource );
      layer->setDataSource( path, name, provider, options );
      dataSourceFixed  = layer->isValid();
      if ( dataSourceFixed )
      {
        QString errorMsg;
        QgsReadWriteContext context;
        context.setPathResolver( QgsProject::instance()->pathResolver() );
        context.setProjectTranslator( QgsProject::instance() );
        if ( !layer->readSymbology( node, errorMsg, context ) )
        {
          QgsDebugMsgLevel( QStringLiteral( "Failed to restore original layer style from node XML for layer %1: %2" )
                       .arg( layer->name( ) )
                       .arg( errorMsg ),
                       2 );
        }
        mLayerList->removeRow( i-- );
      }
      else
      {
        unfixed += 1;
      }
    }
  }

  if ( unfixed > 0 )
  {
    int reponse = QMessageBox::warning( this, tr( "Invalid layers" ), tr( "There are still %1 invalid layers, proceed anyway?" ).arg( unfixed ), QMessageBox::Yes | QMessageBox::No );
    if ( reponse == QMessageBox::Yes )
    {
      QDialog::accept();
    }
  }
  else
  {
    QDialog::accept();
  }
}
