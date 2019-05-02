/***************************************************************************
    kadasmaptoolmeasure.cpp
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

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/maptools/kadasmaptoolmeasure.h>
#include <kadas/gui/maptools/kadasmaptooldrawshape.h>

KadasMeasureWidget::KadasMeasureWidget( QgsMapCanvas *canvas, KadasMapToolMeasure::MeasureMode measureMode )
    : KadasBottomBar( canvas ), mNorthComboBox( 0 ), mMeasureMode( measureMode )
{
  bool measureAngle = measureMode == KadasMapToolMeasure::MeasureAngle || measureMode == KadasMapToolMeasure::MeasureAzimuth;
  setLayout( new QHBoxLayout() );

  mMeasurementLabel = new QLabel();
  mMeasurementLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  layout()->addWidget( mMeasurementLabel );

  static_cast<QHBoxLayout*>( layout() )->addStretch( 1 );

  mUnitComboBox = new QComboBox();
  if ( !measureAngle )
  {
    mUnitComboBox->addItem( tr( "Metric" ), static_cast<int>( QgsUnitTypes::DistanceMeters ) );
    mUnitComboBox->addItem( tr( "Imperial" ), static_cast<int>( QgsUnitTypes::DistanceFeet ) );
    mUnitComboBox->addItem( tr( "Nautical" ), static_cast<int>( QgsUnitTypes::DistanceNauticalMiles ) );
    int defUnit = QSettings().value( "/Qgis/measure/last_measure_unit", QgsUnitTypes::DistanceMeters ).toInt();
    mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
  }
  else
  {
    mUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( QgsUnitTypes::AngleDegrees ) );
    mUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( QgsUnitTypes::AngleRadians ) );
    mUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( QgsUnitTypes::AngleGon ) );
    mUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( QgsUnitTypes::AngleMil ) );
    if ( measureMode == KadasMapToolMeasure::MeasureAngle )
    {
      int defUnit = QgsSettings().value( "/Qgis/measure/last_angle_unit", static_cast<int>( QgsUnitTypes::AngleDegrees ) ).toInt();
      mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
    }
    else
    {
      int defUnit = QgsSettings().value( "/Qgis/measure/last_azimuth_unit", static_cast<int>( QgsUnitTypes::AngleMil ) ).toInt();
      mUnitComboBox->setCurrentIndex( defUnit );
    }
  }

  connect( mUnitComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasMeasureWidget::unitsChanged );
  connect( mUnitComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasMeasureWidget::saveDefaultUnits );
  layout()->addWidget( mUnitComboBox );

  if ( measureMode == KadasMapToolMeasure::MeasureAzimuth )
  {
    layout()->addWidget( new QLabel( tr( "North:" ) ) );
    mNorthComboBox = new QComboBox();
    mNorthComboBox->addItem( tr( "Geographic" ), static_cast<int>( KadasGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC ) );
    mNorthComboBox->addItem( tr( "Map" ), static_cast<int>( KadasGeometryRubberBand::AZIMUTH_NORTH_MAP ) );
    int defNorth = QSettings().value( "/Qgis/measure/last_azimuth_north", static_cast<int>( KadasGeometryRubberBand::AZIMUTH_NORTH_GEOGRAPHIC ) ).toInt();
    mNorthComboBox->setCurrentIndex( defNorth );
    connect( mNorthComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasMeasureWidget::unitsChanged );
    connect( mNorthComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &KadasMeasureWidget::saveAzimuthNorth );
    layout()->addWidget( mNorthComboBox );
  }

  if ( measureMode != KadasMapToolMeasure::MeasureAngle )
  {
    QToolButton* pickButton = new QToolButton();
    pickButton->setIcon( QIcon( ":/images/themes/default/mActionSelect.svg" ) );
    pickButton->setToolTip( tr( "Pick existing geometry" ) );
    connect( pickButton, &QToolButton::clicked, this, &KadasMeasureWidget::pickRequested );
    layout()->addWidget( pickButton );
  }

  QToolButton* clearButton = new QToolButton();
  clearButton->setIcon( QIcon( ":/images/themes/default/mIconClear.png" ) );
  clearButton->setToolTip( tr( "Clear" ) );
  connect( clearButton, &QToolButton::clicked, this, &KadasMeasureWidget::clearRequested );
  layout()->addWidget( clearButton );

  QToolButton* closeButton = new QToolButton();
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QToolButton::clicked, this, &KadasMeasureWidget::closeRequested );
  layout()->addWidget( closeButton );

  show();
  if ( !measureAngle )
  {
    setFixedWidth( 400 );
  }
  else
  {
    setFixedWidth( width() );
  }

  updatePosition();
}

void KadasMeasureWidget::saveDefaultUnits( int index )
{
  if ( mMeasureMode == KadasMapToolMeasure::MeasureAzimuth )
  {
    QgsSettings().setValue( "/Qgis/measure/last_azimuth_unit", mUnitComboBox->itemData( index ).toInt() );
  }
  else if ( mMeasureMode == KadasMapToolMeasure::MeasureAngle )
  {
    QgsSettings().setValue( "/Qgis/measure/last_angle_unit", mUnitComboBox->itemData( index ).toInt() );
  }
  else
  {
    QgsSettings().setValue( "/Qgis/measure/last_measure_unit", mUnitComboBox->itemData( index ).toInt() );
  }
}

void KadasMeasureWidget::saveAzimuthNorth( int index )
{
  QgsSettings().setValue( "/Qgis/measure/last_azimuth_north", mNorthComboBox->itemData( index ).toInt() );
}

void KadasMeasureWidget::updateMeasurement( const QString& measurement )
{
  mMeasurementLabel->setText( QString( "<b>%1</b>" ).arg( measurement ) );
}

QgsUnitTypes::DistanceUnit KadasMeasureWidget::currentUnit() const
{
  return static_cast<QgsUnitTypes::DistanceUnit>( mUnitComboBox->itemData( mUnitComboBox->currentIndex() ).toInt() );
}

QgsUnitTypes::AngleUnit KadasMeasureWidget::currentAngleUnit() const
{
  return static_cast<QgsUnitTypes::AngleUnit>( mUnitComboBox->itemData( mUnitComboBox->currentIndex() ).toInt() );
}

KadasGeometryRubberBand::AzimuthNorth KadasMeasureWidget::currentAzimuthNorth() const
{
  return static_cast<KadasGeometryRubberBand::AzimuthNorth>( mNorthComboBox->itemData( mNorthComboBox->currentIndex() ).toInt() );
}

///////////////////////////////////////////////////////////////////////////////


KadasMapToolMeasure::KadasMapToolMeasure( QgsMapCanvas *canvas, MeasureMode measureMode )
    : QgsMapTool( canvas ), mPickFeature( false ), mMeasureMode( measureMode )
{
  if ( mMeasureMode == MeasureAngle )
  {
    mDrawTool = new KadasMapToolDrawCircularSector( canvas );
  }
  else if ( mMeasureMode == MeasureCircle )
  {
    mDrawTool = new KadasMapToolDrawCircle( canvas, true );
  }
  else
  {
    mDrawTool = new KadasMapToolDrawPolyLine( canvas, mMeasureMode == MeasurePolygon, true );
  }
  mDrawTool->setParent( this );
  mDrawTool->setAllowMultipart( mMeasureMode != MeasureAngle && mMeasureMode != MeasureAzimuth );
  mDrawTool->getRubberBand()->setIconType( mMeasureMode != MeasureCircle ? KadasGeometryRubberBand::ICON_CIRCLE : KadasGeometryRubberBand::ICON_NONE );
  mDrawTool->setSnapPoints( true );
  mDrawTool->setParentTool( this );
  mMeasureWidget = 0;
  connect( mDrawTool, &KadasMapToolDrawShape::geometryChanged, this, &KadasMapToolMeasure::updateTotal );
}

KadasMapToolMeasure::~KadasMapToolMeasure()
{
  delete mDrawTool;
}

void KadasMapToolMeasure::addGeometry( const QgsGeometry* geometry, const QgsVectorLayer* layer )
{
  mDrawTool->addGeometry( geometry->constGet(), layer->crs() );
}

void KadasMapToolMeasure::activate()
{
  mPickFeature = false;
  mMeasureWidget = new KadasMeasureWidget( mCanvas, mMeasureMode );
  setUnits();
  connect( mMeasureWidget, &KadasMeasureWidget::unitsChanged, this, &KadasMapToolMeasure::setUnits );
  connect( mMeasureWidget, &KadasMeasureWidget::clearRequested, mDrawTool, &KadasMapToolDrawShape::reset );
  connect( mMeasureWidget, &KadasMeasureWidget::closeRequested, this, &KadasMapToolMeasure::close );
  connect( mMeasureWidget, &KadasMeasureWidget::pickRequested, this, &KadasMapToolMeasure::requestPick );
  setCursor( Qt::ArrowCursor );
  mDrawTool->getRubberBand()->setVisible( true );
  mDrawTool->setShowInputWidget( QSettings().value( "/Qgis/showNumericInput", false ).toBool() );
  mDrawTool->activate();
  QgsMapTool::activate();
}

void KadasMapToolMeasure::deactivate()
{
  delete mMeasureWidget;
  mMeasureWidget = 0;
  mDrawTool->getRubberBand()->setVisible( false );
  mDrawTool->deactivate();
  QgsMapTool::deactivate();
}

void KadasMapToolMeasure::close()
{
  canvas()->unsetMapTool( this );
}

void KadasMapToolMeasure::setUnits()
{
  switch ( mMeasureMode )
  {
    case MeasureLine:
      mDrawTool->setMeasurementMode( KadasGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, mMeasureWidget->currentUnit(), QgsUnitTypes::distanceToAreaUnit(mMeasureWidget->currentUnit()) ); break;
    case MeasurePolygon:
      mDrawTool->setMeasurementMode( KadasGeometryRubberBand::MEASURE_POLYGON, mMeasureWidget->currentUnit(), QgsUnitTypes::distanceToAreaUnit(mMeasureWidget->currentUnit()) ); break;
    case MeasureCircle:
      mDrawTool->setMeasurementMode( KadasGeometryRubberBand::MEASURE_CIRCLE, mMeasureWidget->currentUnit(), QgsUnitTypes::distanceToAreaUnit(mMeasureWidget->currentUnit() ) ); break;
    case MeasureAngle:
      mDrawTool->setMeasurementMode( KadasGeometryRubberBand::MEASURE_ANGLE, QgsUnitTypes::DistanceMeters, QgsUnitTypes::AreaSquareMeters, mMeasureWidget->currentAngleUnit() ); break;
    case MeasureAzimuth:
      mDrawTool->setMeasurementMode( KadasGeometryRubberBand::MEASURE_AZIMUTH,QgsUnitTypes::DistanceMeters, QgsUnitTypes::AreaSquareMeters, mMeasureWidget->currentAngleUnit(), mMeasureWidget->currentAzimuthNorth() ); break;
  }
}

void KadasMapToolMeasure::updateTotal()
{
  if ( mMeasureWidget )
  {
    mMeasureWidget->updateMeasurement( mDrawTool->getRubberBand()->getTotalMeasurement() );
  }
}

void KadasMapToolMeasure::requestPick()
{
  mPickFeature = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void KadasMapToolMeasure::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasPressEvent( e );
  }
}

void KadasMapToolMeasure::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasMoveEvent( e );
  }
}

void KadasMapToolMeasure::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    mDrawTool->canvasReleaseEvent( e );
  }
  else
  {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), ( mMeasureMode == MeasureLine || mMeasureMode == MeasureAzimuth ) ? QgsWkbTypes::LineGeometry : QgsWkbTypes::PolygonGeometry );
    if ( pickResult.feature.isValid() )
    {
      mDrawTool->addGeometry( pickResult.feature.geometry().constGet(), pickResult.layer->crs() );
    }
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
  }
}

void KadasMapToolMeasure::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPickFeature && e->key() == Qt::Key_Escape )
  {
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
  }
  else if ( e->key() == Qt::Key_Escape && mDrawTool->getStatus() == KadasMapToolDrawShape::StatusReady )
  {
    canvas()->unsetMapTool( this );
  }
  else
  {
    mDrawTool->keyReleaseEvent( e );
  }
}
