/***************************************************************************
    kadaslayerselectionwidget.cpp
    -----------------------------
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

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QToolButton>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadaslayerselectionwidget.h>


KadasLayerSelectionWidget::KadasLayerSelectionWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, LayerFilter filter, LayerCreator creator, QWidget *parent )
  : QWidget( parent ),  mCanvas( canvas ), mLayerTreeView( layerTreeView ), mFilter( filter ), mCreator( creator )
{
  setLayout( new QHBoxLayout() );
  layout()->setSpacing( 2 );
  layout()->setContentsMargins( 0, 0, 0, 0 );

  mLabel = new QLabel( tr( "Layer:" ) );
  layout()->addWidget( mLabel );

  mLayersCombo = new QComboBox( this );
  mLayersCombo->setFixedWidth( 100 );
  connect( mLayersCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, qOverload<int>( &KadasLayerSelectionWidget::layerSelectionChanged ) );
  layout()->addWidget( mLayersCombo );

  if ( creator )
  {
    QToolButton *newLayerButton = new QToolButton();
    newLayerButton->setIcon( QgsApplication::getThemeIcon( "/mActionAdd.svg" ) );
    connect( newLayerButton, &QToolButton::clicked, this, &KadasLayerSelectionWidget::createLayer );
    layout()->addWidget( newLayerButton );
  }

  connect( QgsProject::instance(), &QgsProject::layersAdded, this, &KadasLayerSelectionWidget::repopulateLayers );
  connect( QgsProject::instance(), &QgsProject::layersRemoved, this, &KadasLayerSelectionWidget::repopulateLayers );
  connect( mCanvas, &QgsMapCanvas::currentLayerChanged, this, qOverload<QgsMapLayer *>( &KadasLayerSelectionWidget::setSelectedLayer ) );

  repopulateLayers();
}

void KadasLayerSelectionWidget::createLayerIfEmpty( const QString &layerName )
{
  if ( mLayersCombo->count() == 0 && mCreator )
  {
    QgsMapLayer *layer = mCreator( layerName );
    QgsProject::instance()->addMapLayer( layer );
    setSelectedLayer( layer );
  }
}

void KadasLayerSelectionWidget::setLabel( const QString &label )
{
  mLabel->setText( label );
}

QgsMapLayer *KadasLayerSelectionWidget::getSelectedLayer() const
{
  QString id = mLayersCombo->itemData( mLayersCombo->currentIndex() ).toString();
  return QgsProject::instance()->mapLayer( id );
}

void KadasLayerSelectionWidget::repopulateLayers()
{
  // Avoid update while updating
  if ( mLayersCombo->signalsBlocked() )
  {
    return;
  }
  mLayersCombo->blockSignals( true );
  mLayersCombo->clear();
  int idx = 0, current = -1;
  QgsMapLayer *currentLayer = nullptr;
  for ( QgsMapLayer *layer : QgsProject::instance()->mapLayers().values() )
  {
    if ( !mFilter || mFilter( layer ) )
    {
      connect( layer, &QgsMapLayer::nameChanged, this, &KadasLayerSelectionWidget::repopulateLayers, Qt::UniqueConnection );
      mLayersCombo->addItem( layer->name(), layer->id() );
      if ( mCanvas->currentLayer() == layer )
      {
        current = idx;
        currentLayer = layer;
      }
      ++idx;
    }
  }
  mLayersCombo->setCurrentIndex( -1 );
  mLayersCombo->setCurrentIndex( current );
  emit selectedLayerChanged( currentLayer );
  mLayersCombo->blockSignals( false );
}

void KadasLayerSelectionWidget::layerSelectionChanged( int idx )
{
  if ( idx >= 0 )
  {
    QgsMapLayer *layer = QgsProject::instance()->mapLayer( mLayersCombo->itemData( idx ).toString() );
    if ( layer && mCanvas->currentLayer() != layer )
    {
      mLayerTreeView->setLayerVisible( layer, true );
      mLayerTreeView->setCurrentLayer( layer );
      mCanvas->setCurrentLayer( layer );
    }
    emit selectedLayerChanged( layer );
  }
  else
  {
    emit selectedLayerChanged( nullptr );
  }
}

void KadasLayerSelectionWidget::setSelectedLayer( QgsMapLayer *layer )
{
  int idx = layer ? mLayersCombo->findData( layer->id() ) : -1;
  mLayersCombo->blockSignals( true );
  mLayersCombo->setCurrentIndex( -1 );
  mLayersCombo->blockSignals( false );
  mLayersCombo->setCurrentIndex( idx );
}

void KadasLayerSelectionWidget::createLayer()
{
  QString layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new layer:" ) );
  if ( !layerName.isEmpty() )
  {
    QgsMapLayer *layer = mCreator( layerName );
    QgsProject::instance()->addMapLayer( layer );
    setSelectedLayer( layer );
  }
}
