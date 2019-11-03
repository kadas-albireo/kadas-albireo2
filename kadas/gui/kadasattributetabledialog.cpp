/***************************************************************************
    kadasattributetabledialog.cpp
    ---------------------------
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
#include <QHeaderView>
#include <QVBoxLayout>

#include <qgis/qgsattributetableview.h>
#include <qgis/qgsattributetablefiltermodel.h>
#include <qgis/qgsattributetablemodel.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasattributetabledialog.h>

KadasAttributeTableDialog::KadasAttributeTableDialog( QgsVectorLayer *layer, QgsMapCanvas *canvas, QWidget *parent )
  : QDialog( parent )
{
  setWindowTitle( tr( "Layer Attributes: %1" ).arg( layer->name() ) );
  resize( 800, 480 );

  setLayout( new QVBoxLayout );
  layout()->setMargin( 2 );

  QgsVectorLayerCache *layerCache = new QgsVectorLayerCache( layer, 10000, this );
  QgsAttributeTableModel *model = new QgsAttributeTableModel( layerCache );
  model->loadLayer();
  QgsAttributeTableFilterModel *filterModel = new QgsAttributeTableFilterModel( canvas, model, this );

  QgsAttributeTableView *view = new QgsAttributeTableView();
  view->horizontalHeader()->setStretchLastSection( true );
  view->setModel( filterModel );
  layout()->addWidget( view );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close );
  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  layout()->addWidget( bbox );

  setAttribute( Qt::WA_DeleteOnClose );
}
