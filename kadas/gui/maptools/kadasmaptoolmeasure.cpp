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

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvectorlayer.h>

#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/maptools/kadasmaptoolmeasure.h"

KadasMeasureWidget::KadasMeasureWidget( KadasMapItem *item )
  : KadasMapItemEditor( item )
{
  setLayout( new QHBoxLayout() );
  layout()->setMargin( 2 );
  layout()->setSpacing( 2 );


  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->setMargin( 0 );
  gridLayout->setSpacing( 2 );
  QWidget *grid = new QWidget();
  grid->setLayout( gridLayout );
  layout()->addWidget( grid );

  QString toolLabel;
  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    toolLabel = tr( "Measure line" );
  }
  else if ( dynamic_cast<KadasPolygonItem *>( mItem ) )
  {
    toolLabel = tr( "Measure polygon" );
  }
  else if ( dynamic_cast<KadasCircleItem *>( mItem ) )
  {
    toolLabel = tr( "Measure circle" );
  }
  gridLayout->addWidget( new QLabel( QString( "<b>%1</b>" ).arg( toolLabel ) ), 0, 0 );

  mMeasurementLabel = new QLabel();
  mMeasurementLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  mMeasurementLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
  gridLayout->addWidget( mMeasurementLabel, 0, 1 );

  mUnitComboBox = new QComboBox();
  mUnitComboBox->addItem( tr( "Metric" ), static_cast<int>( Qgis::DistanceUnit::Meters ) );
  mUnitComboBox->addItem( tr( "Imperial" ), static_cast<int>( Qgis::DistanceUnit::Feet ) );
  mUnitComboBox->addItem( tr( "Nautical" ), static_cast<int>( Qgis::DistanceUnit::NauticalMiles ) );
  int defUnit = QgsSettings().value( "/kadas/last_measure_unit", static_cast<int>( Qgis::DistanceUnit::Meters ) ).toInt();
  mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
  connect( mUnitComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setDistanceUnit );
  gridLayout->addWidget( mUnitComboBox, 0, 2 );

  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    mAzimuthCheckbox = new QCheckBox( tr( "Azimuth" ) );
    mAzimuthCheckbox->setChecked( QgsSettings().value( "kadas/last_azimuth_enabled", true ).toBool() );
    connect( mAzimuthCheckbox, &QCheckBox::toggled, this, &KadasMeasureWidget::setAzimutEnabled );
    gridLayout->addWidget( mAzimuthCheckbox, 1, 0 );

    mNorthComboBox = new QComboBox();
    mNorthComboBox->addItem( tr( "Geographic north" ), QVariant::fromValue( AzimuthNorth::AzimuthGeoNorth ) );
    mNorthComboBox->addItem( tr( "Map north" ), QVariant::fromValue( AzimuthNorth::AzimuthMapNorth ) );
    mNorthComboBox->setCurrentIndex( mNorthComboBox->findData( QVariant::fromValue( settingsLastAzimuthNorth->value() ) ) );
    mNorthComboBox->setEnabled( mAzimuthCheckbox->isChecked() );
    connect( mNorthComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setAzimuthNorth );
    gridLayout->addWidget( mNorthComboBox, 1, 1 );

    mAngleUnitComboBox = new QComboBox();
    mAngleUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( Qgis::AngleUnit::Degrees ) );
    mAngleUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( Qgis::AngleUnit::Radians ) );
    mAngleUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( Qgis::AngleUnit::Gon ) );
    mAngleUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( Qgis::AngleUnit::MilNATO ) );
    int defUnit = std::max( 0, QgsSettings().value( "/kadas/last_azimuth_unit", static_cast<int>( Qgis::AngleUnit::MilNATO ) ).toInt() );
    mAngleUnitComboBox->setCurrentIndex( mAngleUnitComboBox->findData( defUnit ) );
    mAngleUnitComboBox->setEnabled( mAzimuthCheckbox->isChecked() );
    connect( mAngleUnitComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setAngleUnit );
    gridLayout->addWidget( mAngleUnitComboBox, 1, 2 );
  }

  QToolButton *pickButton = new QToolButton();
  pickButton->setIcon( QIcon( ":/kadas/icons/select" ) );
  pickButton->setToolTip( tr( "Pick existing geometry" ) );
  connect( pickButton, &QToolButton::clicked, this, &KadasMeasureWidget::pickRequested );
  layout()->addWidget( pickButton );

  QToolButton *clearButton = new QToolButton();
  clearButton->setIcon( QIcon( ":/kadas/icons/clear" ) );
  clearButton->setToolTip( tr( "Clear" ) );
  connect( clearButton, &QToolButton::clicked, this, &KadasMeasureWidget::clearRequested );
  layout()->addWidget( clearButton );

  connect( mItem, &KadasMapItem::changed, this, &KadasMeasureWidget::updateTotal );

  setFixedWidth( 350 );
}

void KadasMeasureWidget::syncWidgetToItem()
{
  if ( mAzimuthCheckbox && mAzimuthCheckbox->isChecked() )
  {
    setAngleUnit( mAngleUnitComboBox->currentIndex() );
    setAzimuthNorth( mNorthComboBox->currentIndex() );
  }
  else if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    KadasLineItem *lineItem = static_cast<KadasLineItem *>( mItem );
    lineItem->setMeasurementMode( KadasLineItem::MeasurementMode::MeasureLineAndSegments, lineItem->angleUnit() );
  }
  setDistanceUnit( mUnitComboBox->currentIndex() );
}

void KadasMeasureWidget::setItem( KadasMapItem *item )
{
  KadasMapItemEditor::setItem( item );
  connect( mItem, &KadasMapItem::changed, this, &KadasMeasureWidget::updateTotal );
}

void KadasMeasureWidget::setDistanceUnit( int index )
{
  Qgis::DistanceUnit unit = static_cast<Qgis::DistanceUnit>( mUnitComboBox->itemData( index ).toInt() );
  QgsSettings().setValue( "/kadas/last_measure_unit", static_cast<int>( unit ) );
  if ( dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    static_cast<KadasGeometryItem *>( mItem )->setMeasurementsEnabled( true, unit );
  }
}

void KadasMeasureWidget::setAngleUnit( int index )
{
  Qgis::AngleUnit unit = static_cast<Qgis::AngleUnit>( mAngleUnitComboBox->itemData( index ).toInt() );
  QgsSettings().setValue( "/kadas/last_azimuth_unit", static_cast<int>( unit ) );
  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    KadasLineItem *lineItem = static_cast<KadasLineItem *>( mItem );
    lineItem->setMeasurementMode( lineItem->measurementMode(), unit );
  }
}

void KadasMeasureWidget::setAzimutEnabled( bool enabled )
{
  QgsSettings().setValue( "/kadas/last_azimuth_enabled", enabled );
  mNorthComboBox->setEnabled( enabled );
  mAngleUnitComboBox->setEnabled( enabled );
  syncWidgetToItem();
}

void KadasMeasureWidget::setAzimuthNorth( int index )
{
  AzimuthNorth north = mNorthComboBox->itemData( index ).value<AzimuthNorth>();
  settingsLastAzimuthNorth->setValue( north );
  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    KadasLineItem *lineItem = static_cast<KadasLineItem *>( mItem );
    lineItem->setMeasurementMode( north == AzimuthNorth::AzimuthGeoNorth ? KadasLineItem::MeasurementMode::MeasureLineAndSegmentsAndAzimuthGeoNorth : KadasLineItem::MeasurementMode::MeasureLineAndSegmentsAndAzimuthMapNorth, lineItem->angleUnit() );
  }
}

void KadasMeasureWidget::updateTotal()
{
  if ( dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    QString total = static_cast<KadasGeometryItem *>( mItem )->getTotalMeasurement();
    mMeasurementLabel->setText( QString( "<b>%1</b>" ).arg( total ) );
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasMapItem *KadasMapToolMeasureItemInterface::createItem() const
{
  KadasMapItem *item = nullptr;
  switch ( mMeasureMode )
  {
    case KadasMapToolMeasure::MeasureMode::MeasureLine:
      item = new KadasLineItem( true );
    case KadasMapToolMeasure::MeasureMode::MeasurePolygon:
      item = new KadasPolygonItem( true );
    case KadasMapToolMeasure::MeasureMode::MeasureCircle:
      item = new KadasCircleItem( true );
  }
  if ( item )
  {
    item->setCrs( mCanvas->mapSettings().destinationCrs() );
    item->setEditor( "KadasMeasureWidget" );
  }
  return item;
}


KadasMapToolMeasure::KadasMapToolMeasure( QgsMapCanvas *canvas, MeasureMode measureMode )
  : KadasMapToolCreateItem( canvas, std::move( std::make_unique<KadasMapToolMeasureItemInterface>( KadasMapToolMeasureItemInterface( canvas, measureMode ) ) ) )
  , mMeasureMode( measureMode )
{
  setMultipart( true );
  setSnappingEnabled( true );
}


void KadasMapToolMeasure::activate()
{
  mPickFeature = false;
  setCursor( Qt::ArrowCursor );
  KadasMapToolCreateItem::activate();

  const KadasMeasureWidget *widget = dynamic_cast<const KadasMeasureWidget *>( currentEditor() );
  if ( widget )
  {
    connect( widget, &KadasMeasureWidget::clearRequested, this, &KadasMapToolMeasure::clear );
    connect( widget, &KadasMeasureWidget::pickRequested, this, &KadasMapToolMeasure::requestPick );
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
    KadasMapToolCreateItem::canvasPressEvent( e );
  }
}

void KadasMapToolMeasure::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    KadasMapToolCreateItem::canvasMoveEvent( e );
  }
}

void KadasMapToolMeasure::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    KadasMapToolCreateItem::canvasReleaseEvent( e );
  }
  else
  {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), mMeasureMode == MeasureMode::MeasureLine ? Qgis::GeometryType::Line : Qgis::GeometryType::Polygon );
    if ( pickResult.geom )
    {
      addPartFromGeometry( *pickResult.geom, pickResult.crs );
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
  else
  {
    KadasMapToolCreateItem::keyReleaseEvent( e );
  }
}
