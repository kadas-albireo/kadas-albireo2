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
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadascircleitem.h>
#include <kadas/gui/maptools/kadasmaptoolmeasure.h>

KadasMeasureWidget::KadasMeasureWidget( KadasMapItem *item, bool measureAzimuth )
  : KadasMapItemEditor( item ), mMeasureAzimuth( measureAzimuth )
{
  setLayout( new QHBoxLayout() );
  layout()->setMargin( 2 );
  layout()->setSpacing( 2 );

  mMeasurementLabel = new QLabel();
  mMeasurementLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  layout()->addWidget( mMeasurementLabel );

  static_cast<QHBoxLayout *>( layout() )->addStretch( 1 );

  mUnitComboBox = new QComboBox();
  if ( !measureAzimuth )
  {
    mUnitComboBox->addItem( tr( "Metric" ), static_cast<int>( QgsUnitTypes::DistanceMeters ) );
    mUnitComboBox->addItem( tr( "Imperial" ), static_cast<int>( QgsUnitTypes::DistanceFeet ) );
    mUnitComboBox->addItem( tr( "Nautical" ), static_cast<int>( QgsUnitTypes::DistanceNauticalMiles ) );
    int defUnit = QSettings().value( "/Qgis/measure/last_measure_unit", QgsUnitTypes::DistanceMeters ).toInt();
    mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
    connect( mUnitComboBox, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setDistanceUnit );
  }
  else
  {
    mUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( QgsUnitTypes::AngleDegrees ) );
    mUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( QgsUnitTypes::AngleRadians ) );
    mUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( QgsUnitTypes::AngleGon ) );
    mUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( QgsUnitTypes::AngleMilNATO ) );
    int defUnit = QgsSettings().value( "/Qgis/measure/last_azimuth_unit", static_cast<int>( QgsUnitTypes::AngleMilNATO ) ).toInt();
    mUnitComboBox->setCurrentIndex( defUnit );
    connect( mUnitComboBox, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setAngleUnit );
  }

  layout()->addWidget( mUnitComboBox );

  if ( measureAzimuth )
  {
    layout()->addWidget( new QLabel( tr( "North:" ) ) );
    mNorthComboBox = new QComboBox();
    mNorthComboBox->addItem( tr( "Geographic" ), static_cast<int>( AzimuthGeoNorth ) );
    mNorthComboBox->addItem( tr( "Map" ), static_cast<int>( AzimuthMapNorth ) );
    int defNorth = QSettings().value( "/Qgis/measure/last_azimuth_north", static_cast<int>( AzimuthGeoNorth ) ).toInt();
    mNorthComboBox->setCurrentIndex( defNorth );
    connect( mNorthComboBox, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasMeasureWidget::setAzimuthNorth );
    layout()->addWidget( mNorthComboBox );
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

  if ( !measureAzimuth )
  {
    connect( mItem, &KadasMapItem::changed, this, &KadasMeasureWidget::updateTotal );
  }

  show();
  setFixedWidth( 400 );
}

void KadasMeasureWidget::syncWidgetToItem()
{
  if ( mMeasureAzimuth )
  {
    setAngleUnit( mUnitComboBox->currentIndex() );
    setAzimuthNorth( mNorthComboBox->currentIndex() );
  }
  else
  {
    setDistanceUnit( mUnitComboBox->currentIndex() );
  }
}

void KadasMeasureWidget::setDistanceUnit( int index )
{
  QgsUnitTypes::DistanceUnit unit = static_cast<QgsUnitTypes::DistanceUnit>( mUnitComboBox->itemData( index ).toInt() );
  QgsSettings().setValue( "/Qgis/measure/last_measure_unit", unit );
  if ( dynamic_cast<KadasGeometryItem *>( mItem ) )
  {
    static_cast<KadasGeometryItem *>( mItem )->setMeasurementsEnabled( true, unit );
  }
}

void KadasMeasureWidget::setAngleUnit( int index )
{
  QgsUnitTypes::AngleUnit unit = static_cast<QgsUnitTypes::AngleUnit>( mUnitComboBox->itemData( index ).toInt() );
  QgsSettings().setValue( "/Qgis/measure/last_angle_unit", unit );
  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    KadasLineItem *lineItem = static_cast<KadasLineItem *>( mItem );
    lineItem->setMeasurementMode( lineItem->measurementMode(), unit );
  }
}

void KadasMeasureWidget::setAzimuthNorth( int index )
{
  AzimuthNorth north = static_cast<AzimuthNorth>( mNorthComboBox->itemData( index ).toInt() );
  QgsSettings().setValue( "/Qgis/measure/last_azimuth_north", north );
  if ( dynamic_cast<KadasLineItem *>( mItem ) )
  {
    KadasLineItem *lineItem = static_cast<KadasLineItem *>( mItem );
    lineItem->setMeasurementMode( north == AzimuthGeoNorth ? KadasLineItem::MeasureAzimuthGeoNorth : KadasLineItem::MeasureAzimuthMapNorth, lineItem->angleUnit() );
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


KadasMapToolMeasure::KadasMapToolMeasure( QgsMapCanvas *canvas, MeasureMode measureMode )
  : KadasMapToolCreateItem( canvas, itemFactory( canvas, measureMode ) )
{
  setMultipart( measureMode != MeasureAzimuth );
  setSnappingEnabled( true );
}

KadasMapToolCreateItem::ItemFactory KadasMapToolMeasure::itemFactory( QgsMapCanvas *canvas, MeasureMode measureMode ) const
{
  switch ( measureMode )
  {
    case MeasureLine:
      return [ = ] { return setupItem( new KadasLineItem( canvas->mapSettings().destinationCrs(), true ), false ); };
    case MeasurePolygon:
      return [ = ] { return setupItem( new KadasPolygonItem( canvas->mapSettings().destinationCrs(), true ), false ); };
    case MeasureCircle:
      return [ = ] { return setupItem( new KadasCircleItem( canvas->mapSettings().destinationCrs(), true ), false ); };
    case MeasureAzimuth:
      return [ = ] { return setupItem( new KadasLineItem( canvas->mapSettings().destinationCrs(), true ), true ); };
  }
  return nullptr;
}

KadasGeometryItem *KadasMapToolMeasure::setupItem( KadasGeometryItem *item, bool measureAzimut ) const
{
  item->setIconType( KadasGeometryItem::ICON_CIRCLE );
  item->setEditorFactory( [ = ]( KadasMapItem * mapItem )
  {
    KadasMeasureWidget *widget = new KadasMeasureWidget( mapItem, measureAzimut );
    connect( widget, &KadasMeasureWidget::clearRequested, this, &KadasMapToolMeasure::clear );
    connect( widget, &KadasMeasureWidget::pickRequested, this, &KadasMapToolMeasure::requestPick );
    return widget;
  } );
  return item;
}

void KadasMapToolMeasure::activate()
{
  mPickFeature = false;
  setCursor( Qt::ArrowCursor );
  KadasMapToolCreateItem::activate();
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
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, e->pos(), toMapCoordinates( e->pos() ), ( mMeasureMode == MeasureLine || mMeasureMode == MeasureAzimuth ) ? QgsWkbTypes::LineGeometry : QgsWkbTypes::PolygonGeometry );
    if ( pickResult.feature.isValid() )
    {
      addPartFromGeometry( pickResult.feature.geometry().constGet(), pickResult.layer->crs() );
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
