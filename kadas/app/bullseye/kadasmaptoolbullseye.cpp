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
#include <qgis/qgsunittypes.h>

#include <kadas/gui/kadaslayerselectionwidget.h>
#include <bullseye/kadasbullseyelayer.h>
#include <bullseye/kadasmaptoolbullseye.h>

KadasMapToolBullseye::KadasMapToolBullseye( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
  : QgsMapTool( canvas )
{
  if ( !dynamic_cast<KadasBullseyeLayer *>( layer ) )
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

  mWidget = new KadasBullseyeWidget( canvas, layerTreeView, layer );
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


KadasBullseyeWidget::KadasBullseyeWidget( QgsMapCanvas *canvas, QgsLayerTreeView *layerTreeView, QgsMapLayer *layer )
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
  connect( closeButton, &QPushButton::clicked, this, &KadasBullseyeWidget::close );
  layout()->addWidget( closeButton );
  layout()->setAlignment( closeButton, Qt::AlignTop );

  auto layerFilter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasBullseyeLayer *>( layer ) != nullptr; };
  auto layerCreator = [this]( const QString & name ) { return createLayer( name ); };
  mLayerSelectionWidget = new KadasLayerSelectionWidget( mCanvas, layerTreeView, layerFilter, layerCreator );
  ui.layerSelectionWidgetHolder->addWidget( mLayerSelectionWidget );

  ui.comboBoxRingIntervalUnit->addItem( "m", QVariant::fromValue( Qgis::DistanceUnit::Meters ) );
  ui.comboBoxRingIntervalUnit->addItem( "ft", QVariant::fromValue( Qgis::DistanceUnit::Feet ) );
  ui.comboBoxRingIntervalUnit->addItem( "mi", QVariant::fromValue( Qgis::DistanceUnit::Miles ) );
  ui.comboBoxRingIntervalUnit->addItem( "nm", QVariant::fromValue( Qgis::DistanceUnit::NauticalMiles ) );

  connect( ui.inputCenter, &KadasCoordinateInput::coordinateChanged, this, &KadasBullseyeWidget::updateLayer );
  connect( ui.toolButtonPickCenter, &QToolButton::clicked, this, [this] { emit requestPickCenter( true ); } );
  connect( ui.spinBoxRings, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );

  connect( ui.spinBoxRingInterval, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );
  connect( ui.spinBoxAxesInterval, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLayer );
  connect( ui.comboBoxRingIntervalUnit, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasBullseyeWidget::ringUnitChanged );

  connect( ui.toolButtonColor, &QgsColorButton::colorChanged, this, &KadasBullseyeWidget::updateColor );
  connect( ui.spinBoxFontSize, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateFontSize );
  connect( ui.checkBoxLabelAxes, &QCheckBox::toggled, this, &KadasBullseyeWidget::updateLabeling );
  connect( ui.checkBoxLabelQuadrants, &QCheckBox::toggled, this, &KadasBullseyeWidget::updateLabeling );
  connect( ui.checkBoxLabelRings, &QCheckBox::toggled, this, &KadasBullseyeWidget::updateLabeling );
  connect( ui.spinBoxLineWidth, qOverload<int>( &QSpinBox::valueChanged ), this, &KadasBullseyeWidget::updateLineWidth );
  connect( mLayerSelectionWidget, &KadasLayerSelectionWidget::selectedLayerChanged, this, &KadasBullseyeWidget::setCurrentLayer );

  mLayerSelectionWidget->setSelectedLayer( layer );
  mLayerSelectionWidget->createLayerIfEmpty( tr( "Bullseye" ) );
}

KadasBullseyeLayer *KadasBullseyeWidget::createLayer( QString layerName )
{
  QgsDistanceArea da;
  da.setEllipsoid( "WGS84" );
  da.setSourceCrs( mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  QgsRectangle extent = mCanvas->extent();
  double extentHeight = da.measureLine( QgsPoint( extent.center().x(), extent.yMinimum() ), QgsPoint( extent.center().x(), extent.yMaximum() ) );
  double interval = 0.5 * extentHeight * QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, Qgis::DistanceUnit::NauticalMiles ) / 6; // Half height divided by nr rings+1, in nm
  KadasBullseyeLayer *bullseyeLayer = new KadasBullseyeLayer( layerName );
  bullseyeLayer->setup( mCanvas->extent().center(), mCanvas->mapSettings().destinationCrs(), 5, interval,  Qgis::DistanceUnit::NauticalMiles, 45 );
  return bullseyeLayer;
}

void KadasBullseyeWidget::setCurrentLayer( QgsMapLayer *layer )
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
  ui.comboBoxRingIntervalUnit->blockSignals( true );
  ui.comboBoxRingIntervalUnit->setCurrentIndex( ui.comboBoxRingIntervalUnit->findData( QVariant::fromValue( mCurrentLayer->ringIntervalUnit() ) ) ) ;
  ui.comboBoxRingIntervalUnit->blockSignals( false );
  ui.spinBoxLineWidth->blockSignals( true );
  ui.spinBoxLineWidth->setValue( mCurrentLayer->lineWidth() );
  ui.spinBoxLineWidth->blockSignals( false );
  ui.toolButtonColor->setColor( mCurrentLayer->color() );
  ui.spinBoxFontSize->setValue( mCurrentLayer->fontSize() );
  ui.checkBoxLabelAxes->setChecked( mCurrentLayer->labelAxes() );
  ui.checkBoxLabelQuadrants->setChecked( mCurrentLayer->labelQuadrants() );
  ui.checkBoxLabelRings->setChecked( mCurrentLayer->labelRings() );
  ui.widgetLayerSetup->setEnabled( true );
}

void KadasBullseyeWidget::centerPicked( const QgsPointXY &pos )
{
  ui.inputCenter->setCoordinate( pos, mCanvas->mapSettings().destinationCrs() );
}

void KadasBullseyeWidget::ringUnitChanged()
{
  Qgis::DistanceUnit intervalUnit = static_cast<Qgis::DistanceUnit>( ui.comboBoxRingIntervalUnit->currentData().toInt() );
  double conv = QgsUnitTypes::fromUnitToUnitFactor( mCurrentLayer->ringIntervalUnit(), intervalUnit );
  ui.spinBoxRingInterval->setValue( ui.spinBoxRingInterval->value() * conv ); // invokes updateLayer()
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
  Qgis::DistanceUnit intervalUnit = static_cast<Qgis::DistanceUnit>( ui.comboBoxRingIntervalUnit->currentData().toInt() );
  int axes = ui.spinBoxAxesInterval->value();
  mCurrentLayer->setup( center, crs, rings, ui.spinBoxRingInterval->value(), intervalUnit, axes );
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

void KadasBullseyeWidget::updateLabeling()
{
  if ( mCurrentLayer )
  {
    mCurrentLayer->setLabelAxes( ui.checkBoxLabelAxes->isChecked() );
    mCurrentLayer->setLabelQuadrants( ui.checkBoxLabelQuadrants->isChecked() );
    mCurrentLayer->setLabelRings( ui.checkBoxLabelRings->isChecked() );
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
