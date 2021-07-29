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
#include <QMainWindow>
#include <QToolBar>
#include <QVBoxLayout>

#include <qgis/qgsapplication.h>
#include <qgis/qgsattributetableview.h>
#include <qgis/qgsattributetablefiltermodel.h>
#include <qgis/qgsattributetablemodel.h>
#include <qgis/qgsexpressionselectiondialog.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectorlayercache.h>
#include <qgis/qgsvectorlayerselectionmanager.h>

#include <kadas/gui/kadasattributetabledialog.h>

KadasAttributeTableDialog::KadasAttributeTableDialog( QgsVectorLayer *layer, QgsMapCanvas *canvas, QgsMessageBar *messageBar, QMainWindow *parent )
  : QDockWidget( parent ), mCanvas( canvas ), mMessageBar( messageBar )
{
  QToolButton *closeButton = new QToolButton( this );
  closeButton->setAutoRaise( true );
  closeButton->setIcon( QgsApplication::getThemeIcon( "/mActionRemove.svg" ) );
  closeButton->setIconSize( QSize( 12, 12 ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QToolButton::clicked, this, &KadasAttributeTableDialog::close );

  QWidget *titleWidget = new QWidget();
  titleWidget->setObjectName( "dockTitleWidget" );
  titleWidget->setLayout( new QHBoxLayout );
  titleWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
  titleWidget->layout()->addWidget( new QLabel( tr( "Layer Attributes: %1" ).arg( layer->name() ) ) );
  static_cast<QHBoxLayout *>( titleWidget->layout() )->addWidget( new QWidget( this ), 1 );   // spacer
  titleWidget->layout()->addWidget( closeButton );
  setTitleBarWidget( titleWidget );
  resize( 800, 480 );

  QWidget *widget = new QWidget( this );
  widget->setObjectName( "dockContentsWidget" );
  widget->setLayout( new QVBoxLayout );
  widget->layout()->setMargin( 2 );
  widget->layout()->setSpacing( 2 );

  QToolBar *toolbar = new QToolBar( this );
  toolbar->setIconSize( QSize( 16, 16 ) );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mIconExpressionSelect.svg" ), tr( "Select features using an expression" ), this, &KadasAttributeTableDialog::selectByExpression );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mActionSelectAll.svg" ), tr( "Select all" ), this, &KadasAttributeTableDialog::selectAll );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mActionInvertSelection.svg" ), tr( "Deselect all features from the layer" ), this, &KadasAttributeTableDialog::invertSelection );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mActionDeselectActiveLayer.svg" ), tr( "Deselect all features from the layer" ), this, &KadasAttributeTableDialog::deselectAll );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mActionPanToSelected.svg" ), tr( "Pan map to the selected rows" ), this, &KadasAttributeTableDialog::panToSelected );
  toolbar->addAction( QgsApplication::getThemeIcon( "/mActionZoomToSelected.svg" ), tr( "Zoom map to the selected rows" ), this, &KadasAttributeTableDialog::zoomToSelected );
  widget->layout()->addWidget( toolbar );

  QgsVectorLayerCache *layerCache = new QgsVectorLayerCache( layer, 10000, this );
  QgsAttributeTableModel *model = new QgsAttributeTableModel( layerCache );
  model->loadLayer();
  QgsAttributeTableFilterModel *filterModel = new QgsAttributeTableFilterModel( canvas, model, this );

  QgsAttributeTableView *view = new QgsAttributeTableView();
  view->horizontalHeader()->setStretchLastSection( true );
  view->setModel( filterModel );
  widget->layout()->addWidget( view );

  mFeatureSelectionManager = new QgsVectorLayerSelectionManager( layer, this );
  view->setFeatureSelectionManager( mFeatureSelectionManager );

  setAttribute( Qt::WA_DeleteOnClose );
  setWidget( widget );
  connect( this, &QDockWidget::dockLocationChanged, this, &KadasAttributeTableDialog::storeDockLocation );

  Qt::DockWidgetArea area = static_cast<Qt::DockWidgetArea>( QgsSettings().value( "kadas/attributedocklocation", Qt::NoDockWidgetArea ).toInt() );
  if ( area == Qt::NoDockWidgetArea )
  {
    setFloating( true );
  }
  else
  {
    parent->addDockWidget( area, this );
  }
}

KadasAttributeTableDialog::~KadasAttributeTableDialog()
{
  deselectAll();
}

void KadasAttributeTableDialog::storeDockLocation( Qt::DockWidgetArea area )
{
  QgsSettings().setValue( "kadas/attributedocklocation", area );
}

void KadasAttributeTableDialog::showEvent( QShowEvent *ev )
{
  auto g = geometry();
  g.moveCenter( parentWidget()->geometry().center() );
  setGeometry( g );
  QDockWidget::showEvent( ev );
}

void KadasAttributeTableDialog::deselectAll()
{
  mFeatureSelectionManager->layer()->removeSelection();
}

void KadasAttributeTableDialog::invertSelection()
{
  mFeatureSelectionManager->layer()->invertSelection();
}

void KadasAttributeTableDialog::panToSelected()
{
  mCanvas->panToSelected( mFeatureSelectionManager->layer() );
}

void KadasAttributeTableDialog::selectAll()
{
  mFeatureSelectionManager->layer()->selectAll();
}

void KadasAttributeTableDialog::selectByExpression()
{
  QgsExpressionSelectionDialog *dlg = new QgsExpressionSelectionDialog( mFeatureSelectionManager->layer() );
  dlg->setMessageBar( mMessageBar );
  dlg->setAttribute( Qt::WA_DeleteOnClose );
  dlg->show();
}

void KadasAttributeTableDialog::zoomToSelected()
{
  mCanvas->zoomToSelected( mFeatureSelectionManager->layer() );
}
