/***************************************************************************
    kadasmaptoolmapgrid.cpp
    -----------------------
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

#include <QAction>
#include <QPushButton>

#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/kadaslayerselectionwidget.h"
#include <mapgrid/kadasmapgridlayer.h>
#include <mapgrid/kadasmaptoolmapgrid.h>

KadasMapToolMapGrid::KadasMapToolMapGrid( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
  : QgsMapTool( canvas )
{
  if ( !dynamic_cast<KadasMapGridLayer *>( layer ) )
  {
    for ( QgsMapLayer *projLayer : QgsProject::instance()->mapLayers() )
    {
      if ( dynamic_cast<KadasMapGridLayer *>( projLayer ) )
      {
        layer = projLayer;
        break;
      }
    }
  }

  mWidget = new KadasMapGridWidget( canvas, layerTreeView, layer );
  setCursor( Qt::ArrowCursor );
  connect( mWidget, &KadasMapGridWidget::close, this, &KadasMapToolMapGrid::close );

  mWidget->show();
}

KadasMapToolMapGrid::~KadasMapToolMapGrid()
{
  delete mWidget;
}

void KadasMapToolMapGrid::close()
{
  canvas()->unsetMapTool( this );
}

void KadasMapToolMapGrid::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void KadasMapToolMapGrid::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
}


KadasMapGridWidget::KadasMapGridWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
  : KadasBottomBar( canvas )
{
  setLayout( new QHBoxLayout );
  layout()->setSpacing( 10 );

  QWidget *base = new QWidget();
  ui.setupUi( base );
  layout()->addWidget( base );

  QPushButton *closeButton = new QPushButton();
  closeButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, &KadasMapGridWidget::close );
  layout()->addWidget( closeButton );
  layout()->setAlignment( closeButton, Qt::AlignTop );

  auto layerFilter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasMapGridLayer *>( layer ) != nullptr; };
  auto layerCreator = [this]( const QString & name ) { return createLayer( name ); };
  mLayerSelectionWidget = new KadasLayerSelectionWidget( canvas, layerTreeView, layerFilter, layerCreator );
  ui.layerSelectionWidgetHolder->addWidget( mLayerSelectionWidget );

  ui.comboBoxGridType->addItem( "LV03", KadasMapGridLayer::GridLV03 );
  ui.comboBoxGridType->addItem( "LV95", KadasMapGridLayer::GridLV95 );
  ui.comboBoxGridType->addItem( "DD", KadasMapGridLayer::GridDD );
  ui.comboBoxGridType->addItem( "DM", KadasMapGridLayer::GridDM );
  ui.comboBoxGridType->addItem( "DMS", KadasMapGridLayer::GridDMS );
  ui.comboBoxGridType->addItem( "UTM ", KadasMapGridLayer::GridUTM );
  ui.comboBoxGridType->addItem( "MGRS", KadasMapGridLayer::GridMGRS );

  mCellSizeLabel = new QLabel( tr( "Cell size:" ) );
  mCellSizeLabel->setVisible( false );
  static_cast<QGridLayout *>( ui.widgetLayerSetup->layout() )->addWidget( mCellSizeLabel, 0, 2, 1, 2 );
  mCellSizeCombo = new QComboBox( );
  mCellSizeCombo->addItem( tr( "Dynamic" ), 0 );
  mCellSizeCombo->addItem( tr( "1 m" ), 1 );
  mCellSizeCombo->addItem( tr( "10 m" ), 10 );
  mCellSizeCombo->addItem( tr( "100 m" ), 100 );
  mCellSizeCombo->addItem( tr( "1 km" ), 1000 );
  mCellSizeCombo->addItem( tr( "10 km" ), 10000 );
  mCellSizeCombo->addItem( tr( "100 km" ), 100000 );
  mCellSizeCombo->setVisible( false );
  static_cast<QGridLayout *>( ui.widgetLayerSetup->layout() )->addWidget( mCellSizeCombo, 0, 4, 1, 2 );

  ui.comboBoxLabeling->addItem( tr( "Disabled" ), KadasMapGridLayer::LabelingDisabled );
  ui.comboBoxLabeling->addItem( tr( "Enabled" ), KadasMapGridLayer::LabelingEnabled );

  connect( ui.comboBoxGridType, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int idx ) { updateType( idx, true ); } );
  connect( ui.spinBoxIntervalX, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasMapGridWidget::updateGrid );
  connect( ui.spinBoxIntervalY, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasMapGridWidget::updateGrid );
  connect( mCellSizeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMapGridWidget::updateGrid );

  connect( ui.toolButtonColor, &QgsColorButton::colorChanged, this, &KadasMapGridWidget::updateColor );
  connect( ui.spinBoxFontSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasMapGridWidget::updateFontSize );
  connect( ui.comboBoxLabeling, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMapGridWidget::updateLabeling );

  connect( mLayerSelectionWidget, &KadasLayerSelectionWidget::selectedLayerChanged, this, &KadasMapGridWidget::setCurrentLayer );

  mLayerSelectionWidget->setSelectedLayer( layer );
  mLayerSelectionWidget->createLayerIfEmpty( tr( "Map grid" ) );
}

QgsMapLayer *KadasMapGridWidget::createLayer( QString layerName )
{
  KadasMapGridLayer *guideGridLayer = new KadasMapGridLayer( layerName );
  guideGridLayer->setup( KadasMapGridLayer::GridLV95, 10000., 10000., 0 );
  return guideGridLayer;
}

void KadasMapGridWidget::setCurrentLayer( QgsMapLayer *layer )
{
  if ( layer == mCurrentLayer )
  {
    return;
  }
  mCurrentLayer = dynamic_cast<KadasMapGridLayer *>( layer );
  if ( !mCurrentLayer )
  {
    ui.widgetLayerSetup->setEnabled( false );
    return;
  }

  ui.comboBoxGridType->blockSignals( true );
  ui.comboBoxGridType->setCurrentIndex( ui.comboBoxGridType->findData( mCurrentLayer->gridType() ) );
  ui.comboBoxGridType->blockSignals( false );
  ui.spinBoxIntervalX->blockSignals( true );
  ui.spinBoxIntervalX->setValue( mCurrentLayer->intervalX() );
  ui.spinBoxIntervalX->blockSignals( false );
  ui.spinBoxIntervalY->blockSignals( true );
  ui.spinBoxIntervalY->setValue( mCurrentLayer->intervalY() );
  ui.spinBoxIntervalY->blockSignals( false );
  mCellSizeCombo->blockSignals( true );
  mCellSizeCombo->setCurrentIndex( mCellSizeCombo->findData( mCurrentLayer->cellSize() ) );
  mCellSizeCombo->blockSignals( false );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.comboBoxLabeling->setCurrentIndex( ui.comboBoxLabeling->findData( mCurrentLayer->labelingMode() ) );
  ui.widgetLayerSetup->setEnabled( true );
  updateType( ui.comboBoxGridType->currentIndex(), false );
}

void KadasMapGridWidget::updateGrid()
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setup( static_cast<KadasMapGridLayer::GridType>( ui.comboBoxGridType->currentData().toInt() ), ui.spinBoxIntervalX->value(), ui.spinBoxIntervalY->value(), mCellSizeCombo->currentData().toInt() );
  mCurrentLayer->triggerRepaint();
}

void KadasMapGridWidget::updateType( int idx, bool updateValues )
{
  KadasMapGridLayer::GridType type = static_cast<KadasMapGridLayer::GridType>( ui.comboBoxGridType->itemData( idx ).toInt() );

  struct IntervalConfig
  {
    bool spinEnabled = false;
    double lower = 0, upper = 0, decimals = 0, value = 0;
    QString suffix;
  } config;
  switch ( type )
  {
    case KadasMapGridLayer::GridLV03:
    case KadasMapGridLayer::GridLV95:
      config = {true, 1, 100000, 0, 10000, QString( " m" )};
      break;
    case KadasMapGridLayer::GridDD:
    case KadasMapGridLayer::GridDM:
    case KadasMapGridLayer::GridDMS:
      config = {true, 0.001, 100, 3, 1, QString( " deg" )};
      break;
    case KadasMapGridLayer::GridUTM:
    case KadasMapGridLayer::GridMGRS:
      config = {false, 0, 0, 0, 0, QString()};
      break;
  }

  ui.spinBoxIntervalX->setVisible( config.spinEnabled );
  ui.spinBoxIntervalY->setVisible( config.spinEnabled );
  mCellSizeCombo->setVisible( !config.spinEnabled );
  ui.labelIntervalX->setVisible( config.spinEnabled );
  ui.labelIntervalY->setVisible( config.spinEnabled );
  mCellSizeLabel->setVisible( !config.spinEnabled );

  if ( updateValues )
  {
    ui.spinBoxIntervalX->blockSignals( true );
    ui.spinBoxIntervalY->blockSignals( true );
    mCellSizeCombo->blockSignals( true );
    ui.spinBoxIntervalX->setRange( config.lower, config.upper );
    ui.spinBoxIntervalY->setRange( config.lower, config.upper );
    ui.spinBoxIntervalX->setDecimals( config.decimals );
    ui.spinBoxIntervalY->setDecimals( config.decimals );
    ui.spinBoxIntervalX->setSuffix( config.suffix );
    ui.spinBoxIntervalY->setSuffix( config.suffix );
    ui.spinBoxIntervalX->setValue( config.value );
    ui.spinBoxIntervalY->setValue( config.value );
    mCellSizeCombo->setCurrentIndex( 0 );
    ui.spinBoxIntervalX->blockSignals( false );
    ui.spinBoxIntervalY->blockSignals( false );
    mCellSizeCombo->blockSignals( false );
  }
  updateGrid();
}

void KadasMapGridWidget::updateColor( const QColor &color )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setColor( color );
  mCurrentLayer->triggerRepaint();
}

void KadasMapGridWidget::updateFontSize( int fontSize )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setFontSize( fontSize );
  mCurrentLayer->triggerRepaint();
}

void KadasMapGridWidget::updateLabeling( int labelingMode )
{
  if ( !mCurrentLayer )
  {
    return;
  }
  mCurrentLayer->setLabelingMode( static_cast<KadasMapGridLayer::LabelingMode>( labelingMode ) );
  mCurrentLayer->triggerRepaint();
}
