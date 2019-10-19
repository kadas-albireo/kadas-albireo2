/***************************************************************************
    kadascoordinatediplayer.cpp
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

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/gui/kadascoordinatedisplayer.h>

KadasCoordinateDisplayer::KadasCoordinateDisplayer( QToolButton *crsButton, QLineEdit *coordLineEdit, QLineEdit *heightLineEdit, QComboBox *heightCombo, QgsMapCanvas *mapCanvas, QWidget *parent )
  : QWidget( parent )
  , mMapCanvas( mapCanvas )
  , mCRSSelectionButton( crsButton )
  , mCoordinateLineEdit( coordLineEdit )
  , mHeightLineEdit( heightLineEdit )
  , mHeightSelectionCombo( heightCombo )
{
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );

  mIconLabel = new QLabel( this );
  mHeightTimer.setSingleShot( true );
  connect( &mHeightTimer, &QTimer::timeout, this, &KadasCoordinateDisplayer::updateHeight );


  QMenu *crsSelectionMenu = new QMenu();
  mCRSSelectionButton->setMenu( crsSelectionMenu );

  mActionDisplayLV03 = crsSelectionMenu->addAction( "LV03" );
  mActionDisplayLV03->setData( static_cast<int>( LV03 ) );
  mActionDisplayLV95 = crsSelectionMenu->addAction( "LV95" );
  mActionDisplayLV95->setData( static_cast<int>( LV95 ) );
  mActionDisplayDMS = crsSelectionMenu->addAction( "DMS" );
  mActionDisplayDMS->setData( static_cast<int>( DMS ) );
  crsSelectionMenu->addAction( "DM" )->setData( static_cast<int>( DM ) );
  crsSelectionMenu->addAction( "DD" )->setData( static_cast<int>( DD ) );
  crsSelectionMenu->addAction( "UTM" )->setData( static_cast<int>( UTM ) );
  crsSelectionMenu->addAction( "MGRS" )->setData( static_cast<int>( MGRS ) );

  QFont font = mCoordinateLineEdit->font();
  font.setPointSize( 9 );
  mCoordinateLineEdit->setFont( font );
  mCoordinateLineEdit->setReadOnly( true );
  mCoordinateLineEdit->setAlignment( Qt::AlignCenter );
  mCoordinateLineEdit->setFixedWidth( 200 );

  mHeightLineEdit->setFont( font );
  mHeightLineEdit->setReadOnly( true );
  mHeightLineEdit->setAlignment( Qt::AlignCenter );
  mHeightLineEdit->setFixedWidth( 100 );

  mHeightSelectionCombo->addItem( tr( "Meters" ), static_cast<int>( QgsUnitTypes::DistanceMeters ) );
  mHeightSelectionCombo->addItem( tr( "Feet" ), static_cast<int>( QgsUnitTypes::DistanceFeet ) );
  mHeightSelectionCombo->setCurrentIndex( -1 );  // to ensure currentIndexChanged is triggered below

  connect( mMapCanvas, &QgsMapCanvas::xyCoordinates, this, &KadasCoordinateDisplayer::displayCoordinates );
  connect( mMapCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasCoordinateDisplayer::syncProjectCrs );
  connect( crsSelectionMenu, &QMenu::triggered, this, &KadasCoordinateDisplayer::displayFormatChanged );
  connect( mHeightSelectionCombo, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasCoordinateDisplayer::heightUnitChanged );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasCoordinateDisplayer::readProjectSettings );

  mHeightSelectionCombo->setCurrentIndex( QgsSettings().value( "/Qgis/heightUnit", 0 ).toInt() );

  syncProjectCrs();
  int displayFormat = QgsProject::instance()->readNumEntry( "crsdisplay", "format" );
  if ( displayFormat < 0 || displayFormat >= crsSelectionMenu->actions().size() )
  {
    displayFormat = 0;
  }
  displayFormatChanged( crsSelectionMenu->actions().at( displayFormat ) );
}

void KadasCoordinateDisplayer::getCoordinateDisplayFormat( KadasCoordinateFormat::Format &format, QString &epsg )
{
  QVariant v = mCRSSelectionButton->defaultAction()->data();
  TargetFormat targetFormat = static_cast<TargetFormat>( v.toInt() );
  epsg = "EPSG:4326";
  switch ( targetFormat )
  {
    case LV03:
      format = KadasCoordinateFormat::Default;
      epsg = "EPSG:21781";
      return;
    case LV95:
      format = KadasCoordinateFormat::Default;
      epsg = "EPSG:2056";
      return;
    case DMS:
      format = KadasCoordinateFormat::DegMinSec;
      return;
    case DM:
      format = KadasCoordinateFormat::DegMin;
      return;
    case DD:
      format = KadasCoordinateFormat::DecDeg;
      return;
    case UTM:
      format = KadasCoordinateFormat::UTM;
      return;
    case MGRS:
      format = KadasCoordinateFormat::MGRS;
      return;
  }
}

QString KadasCoordinateDisplayer::getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs )
{
  QVariant v = mCRSSelectionButton->defaultAction()->data();
  TargetFormat format = static_cast<TargetFormat>( v.toInt() );
  switch ( format )
  {
    case LV03:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::Default, "EPSG:21781" );
    case LV95:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::Default, "EPSG:2056" );
    case DMS:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::DegMinSec, "EPSG:4326" );
    case DM:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::DegMin, "EPSG:4326" );
    case DD:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::DecDeg, "EPSG:4326" );
    case UTM:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::UTM, "EPSG:4326" );
    case MGRS:
      return KadasCoordinateFormat::getDisplayString( p, crs, KadasCoordinateFormat::MGRS, "EPSG:4326" );
  }
  return QString();
}

void KadasCoordinateDisplayer::displayCoordinates( const QgsPointXY &p )
{
  mCoordinateLineEdit->setText( getDisplayString( p, mMapCanvas->mapSettings().destinationCrs() ) );
  mHeightTimer.start( 100 );
  mLastPos = p;
}

void KadasCoordinateDisplayer::updateHeight()
{
  double height = KadasCoordinateFormat::instance()->getHeightAtPos( mLastPos, mMapCanvas->mapSettings().destinationCrs() );
  QString unit = KadasCoordinateFormat::instance()->getHeightDisplayUnit() == QgsUnitTypes::DistanceFeet ? tr( "ft AMSL" ) : tr( "m AMSL" );
  mHeightLineEdit->setText( QString::number( height, 'f', 1 ) + " " + unit );
}

void KadasCoordinateDisplayer::syncProjectCrs()
{
  const QgsCoordinateReferenceSystem &crs = mMapCanvas->mapSettings().destinationCrs();
  if ( crs.srsid() == 4326 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayDMS );
  }
  else if ( crs.srsid() == 21781 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayLV03 );
  }
  else if ( crs.srsid() == 2056 )
  {
    mCRSSelectionButton->setDefaultAction( mActionDisplayLV95 );
  }
}

void KadasCoordinateDisplayer::displayFormatChanged( QAction *action )
{
  QgsProject::instance()->writeEntry( "crsdisplay", "format", mCRSSelectionButton->menu()->actions().indexOf( action ) );
  mCRSSelectionButton->setDefaultAction( action );
  mCoordinateLineEdit->clear();
  KadasCoordinateFormat::Format format;
  QString epsg;
  getCoordinateDisplayFormat( format, epsg );
  KadasCoordinateFormat::instance()->setCoordinateDisplayFormat( format, epsg );
}

void KadasCoordinateDisplayer::heightUnitChanged( int idx )
{
  QgsSettings().setValue( "/Qgis/heightUnit", idx );
  KadasCoordinateFormat::instance()->setHeightDisplayUnit( static_cast<QgsUnitTypes::DistanceUnit>( mHeightSelectionCombo->itemData( idx ).toInt() ) );
  updateHeight();
}

void KadasCoordinateDisplayer::readProjectSettings()
{
  int displayCrs = QgsProject::instance()->readNumEntry( "coodisplay", "crs" );
  if ( displayCrs < 0 || displayCrs >= mCRSSelectionButton->menu()->actions().size() )
  {
    displayCrs = 0;
  }
  displayFormatChanged( mCRSSelectionButton->menu()->actions() [displayCrs] );
}
