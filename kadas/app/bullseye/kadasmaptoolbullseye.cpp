/***************************************************************************
    kadasmaptoolbullseye.h
    ----------------------
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

#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/app/bullseye/kadasbullseyelayer.h>
#include <kadas/app/bullseye/kadasmaptoolbullseye.h>

KadasMapToolBullseye::KadasMapToolBullseye( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
  : QgsMapTool( canvas )
{
  if ( !layer )
  {
    for ( QgsMapLayer *projLayer : QgsProject::instance()->mapLayers() )
    {
      if ( dynamic_cast<KadasBullseyeLayer *>( projLayer ) )
      {
        layer = projLayer;
        break;
      }
    }
  }

  mWidget = new KadasBullseyeWidget( canvas, layerTreeView );
  if ( layer )
  {
    mWidget->setLayer( layer );
  }
  setCursor( Qt::ArrowCursor );
  connect( mWidget, &KadasBullseyeWidget::requestPickCenter, this, &KadasMapToolBullseye::setPicking );
  connect( mWidget, &KadasBullseyeWidget::close, this, &KadasMapToolBullseye::close );

  mWidget->show();
}

KadasMapToolBullseye::~KadasMapToolBullseye()
{
  delete mWidget;
}

void KadasMapToolBullseye::setPicking( bool picking )
{
  mPicking = picking;
  setCursor( picking ? Qt::CrossCursor : Qt::ArrowCursor );
}

void KadasMapToolBullseye::close()
{
  canvas()->unsetMapTool( this );
}

void KadasMapToolBullseye::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( mPicking )
  {
    mWidget->centerPicked( e->mapPoint() );
    setPicking( false );
  }
  else if ( e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
}

void KadasMapToolBullseye::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( mPicking )
    {
      setPicking( false );
    }
    else
    {
      canvas()->unsetMapTool( this );
    }
  }
}


KadasBullseyeWidget::KadasBullseyeWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView )
  : KadasBottomBar( canvas ), mLayerTreeView( layerTreeView )
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
  connect( closeButton, &QPushButton::clicked, this, &KadasBullseyeWidget::close );
  layout()->addWidget( closeButton );
  layout()->setAlignment( closeButton, Qt::AlignTop );

  auto layerFilter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasBullseyeLayer *>( layer ) != nullptr; };
  auto layerCreator = [this]( const QString & name ) { return createLayer( name ); };
  mLayerSelectionWidget = new KadasLayerSelectionWidget( mCanvas, layerFilter, layerCreator );
  mLayerSelectionWidget->createLayerIfEmpty( tr( "Bullseye" ) );
  ui.layerSelectionWidgetHolder->addWidget( mLayerSelectionWidget );

  ui.comboBoxLabels->addItem( tr( "Disabled" ), static_cast<int>( KadasBullseyeLayer::NO_LABELS ) );
  ui.comboBoxLabels->addItem( tr( "Axes" ), static_cast<int>( KadasBullseyeLayer::LABEL_AXES ) );
  ui.comboBoxLabels->addItem( tr( "Rings" ), static_cast<int>( KadasBullseyeLayer::LABEL_RINGS ) );
  ui.comboBoxLabels->addItem( tr( "Axes and rings" ), static_cast<int>( KadasBullseyeLayer::LABEL_AXES_RINGS ) );

  connect( ui.inputCenter, &KadasCoordinateInput::coordinateChanged, this, &KadasBullseyeWidget::updateLayer );
  connect( ui.toolButtonPickCenter, &QToolButton::clicked, this, [this] { emit requestPickCenter( true ); } );
  connect( ui.spinBoxRings, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );

  connect( ui.spinBoxRingInterval, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );
  connect( ui.spinBoxAxesInterval, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );

  connect( ui.toolButtonColor, &QgsColorButton::colorChanged, this, &KadasBullseyeWidget::updateColor );
  connect( ui.spinBoxFontSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateFontSize );
  connect( ui.comboBoxLabels, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasBullseyeWidget::updateLabeling );
  connect( ui.spinBoxLineWidth, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLineWidth );
  connect( mLayerSelectionWidget, &KadasLayerSelectionWidget::selectedLayerChanged, this, &KadasBullseyeWidget::currentLayerChanged );
}

KadasBullseyeLayer *KadasBullseyeWidget::createLayer( QString layerName )
{
  QgsDistanceArea da;
  da.setEllipsoid( "WGS84" );
  da.setSourceCrs( mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  QgsRectangle extent = mCanvas->extent();
  double extentHeight = da.measureLine( QgsPoint( extent.center().x(), extent.yMinimum() ), QgsPoint( extent.center().x(), extent.yMaximum() ) );
  double interval = 0.5 * extentHeight * QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceMeters, QgsUnitTypes::DistanceNauticalMiles ) / 6; // Half height divided by nr rings+1, in nm
  KadasBullseyeLayer *bullseyeLayer = new KadasBullseyeLayer( layerName );
  bullseyeLayer->setup( mCanvas->extent().center(), mCanvas->mapSettings().destinationCrs(), 5, interval, 45 );
  setLayer( bullseyeLayer );
  return bullseyeLayer;
}

void KadasBullseyeWidget::setLayer( QgsMapLayer *layer )
{
  if ( layer == mCurrentLayer )
  {
    return;
  }
  emit requestPickCenter( false );
  mCurrentLayer = dynamic_cast<KadasBullseyeLayer *>( layer );
  if ( !mCurrentLayer )
  {
    ui.widgetLayerSetup->setEnabled( false );
    return;
  }

  mLayerSelectionWidget->blockSignals( true );
  mLayerSelectionWidget->setSelectedLayer( mCurrentLayer );
  mLayerSelectionWidget->blockSignals( false );
  mLayerTreeView->setLayerVisible( mCurrentLayer, true );
  mLayerTreeView->setCurrentLayer( mCurrentLayer );
  mCanvas->setCurrentLayer( mCurrentLayer );

  ui.inputCenter->blockSignals( true );
  ui.inputCenter->setCoordinate( mCurrentLayer->center(), mCurrentLayer->crs() );
  ui.inputCenter->blockSignals( false );
  ui.spinBoxRings->blockSignals( true );
  ui.spinBoxRings->setValue( mCurrentLayer->rings() );
  ui.spinBoxRings->blockSignals( false );
  ui.spinBoxRingInterval->blockSignals( true );
  ui.spinBoxRingInterval->setValue( mCurrentLayer->ringInterval() );
  ui.spinBoxRingInterval->blockSignals( false );
  ui.spinBoxAxesInterval->blockSignals( true );
  ui.spinBoxAxesInterval->setValue( mCurrentLayer->axesInterval() );
  ui.spinBoxAxesInterval->blockSignals( false );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.comboBoxLabels->setCurrentIndex( ui.comboBoxLabels->findData( static_cast<int>( mCurrentLayer->labellingMode() ) ) );
  ui.widgetLayerSetup->setEnabled( true );
}

void KadasBullseyeWidget::centerPicked( const QgsPointXY &pos )
{
  ui.inputCenter->setCoordinate( pos, mCanvas->mapSettings().destinationCrs() );
}

void KadasBullseyeWidget::updateLayer()
{
  if ( !mCurrentLayer || ui.inputCenter->isEmpty() )
  {
    return;
  }
  QgsPointXY center = ui.inputCenter->getCoordinate();
  const QgsCoordinateReferenceSystem &crs = ui.inputCenter->getCrs();
  int rings = ui.spinBoxRings->value();
  double interval = ui.spinBoxRingInterval->value();
  int axes = ui.spinBoxAxesInterval->value();
  mCurrentLayer->setup( center, crs, rings, interval, axes );
  mCurrentLayer->triggerRepaint();
}

void KadasBullseyeWidget::updateColor( const QColor &color )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setColor( color );
    mCurrentLayer->triggerRepaint();
  }
}

void KadasBullseyeWidget::updateFontSize( int fontSize )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setFontSize( fontSize );
    mCurrentLayer->triggerRepaint();
  }
}

void KadasBullseyeWidget::updateLabeling( int /*index*/ )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setLabellingMode( static_cast<KadasBullseyeLayer::LabellingMode>( ui.comboBoxLabels->currentData().toInt() ) );
    mCurrentLayer->triggerRepaint();
  }
}

void KadasBullseyeWidget::updateLineWidth( int width )
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setLineWidth( width );
    mCurrentLayer->triggerRepaint();
  }
}

void KadasBullseyeWidget::currentLayerChanged( QgsMapLayer *layer )
{
  KadasBullseyeLayer *bullseyeLayer = dynamic_cast<KadasBullseyeLayer *>( layer );
  if ( bullseyeLayer )
  {
    setLayer( bullseyeLayer );
  }
  else
  {
    ui.widgetLayerSetup->setEnabled( false );
  }
}
