/***************************************************************************
    kadascoordinateinput.cpp
    ------------------------
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
#include <QLineEdit>
#include <QRegExp>

#include <qgis/qgsproject.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/gui/kadascoordinateinput.h>

KadasCoordinateInput::KadasCoordinateInput( QWidget *parent )
  : QWidget( parent )
{
  setLayout( new QHBoxLayout() );
  layout()->setMargin( 0 );
  layout()->setSpacing( 0 );

  mLineEdit = new QLineEdit( this );
  layout()->addWidget( mLineEdit );
  connect( mLineEdit, &QLineEdit::textEdited, this, &KadasCoordinateInput::entryChanged );
  connect( mLineEdit, &QLineEdit::editingFinished, this, &KadasCoordinateInput::entryEdited );

  mCrsCombo = new QComboBox( this );
  mCrsCombo->addItem( "LV03" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:21781", sAuthidRole );
  mCrsCombo->addItem( "LV95" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:2056", sAuthidRole );
  mCrsCombo->addItem( "DMS" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::DegMinSec, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "DM" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::DegMin, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "DD" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::Default, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "UTM" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::UTM, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->addItem( "MGRS" );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, KadasCoordinateFormat::MGRS, sFormatRole );
  mCrsCombo->setItemData( mCrsCombo->count() - 1, "EPSG:4326", sAuthidRole );
  mCrsCombo->setCurrentIndex( QgsProject::instance()->readNumEntry( "crsdisplay", "format" ) );
  layout()->addWidget( mCrsCombo );
  connect( mCrsCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &KadasCoordinateInput::crsChanged );

  mCrs = QgsCoordinateReferenceSystem( mCrsCombo->currentData( sAuthidRole ).toString() );
}

void KadasCoordinateInput::setCoordinate( const QgsPointXY &coo, const QgsCoordinateReferenceSystem &crs )
{
  mEmpty = false;
  mCoo = QgsCoordinateTransform( crs, mCrs, QgsProject::instance() ).transform( coo );
  KadasCoordinateFormat::Format format = static_cast<KadasCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
  QString authId = mCrsCombo->currentData( sAuthidRole ).toString();
  mLineEdit->setText( KadasCoordinateFormat::instance()->getDisplayString( mCoo, mCrs, format, authId ) );
  mLineEdit->setStyleSheet( "" );
  emit coordinateChanged();
}

void KadasCoordinateInput::entryChanged()
{
  mLineEdit->setStyleSheet( "" );
}

void KadasCoordinateInput::entryEdited()
{
  QString text = mLineEdit->text();
  if ( text.isEmpty() )
  {
    mCoo = QgsPoint();
    mEmpty = true;
  }
  else
  {
    KadasCoordinateFormat::Format format = static_cast<KadasCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
    bool valid = false;
    mCoo = KadasCoordinateFormat::instance()->parseCoordinate( text, format, valid );
    mEmpty = !valid;
    if ( !valid )
    {
      mLineEdit->setStyleSheet( "background: #FF7777; color: #FFFFFF;" );
    }
  }
  emit coordinateEdited();
  emit coordinateChanged();
}

void KadasCoordinateInput::crsChanged()
{
  KadasCoordinateFormat::Format format = static_cast<KadasCoordinateFormat::Format>( mCrsCombo->currentData( sFormatRole ).toInt() );
  QString authId = mCrsCombo->currentData( sAuthidRole ).toString();
  if ( !mEmpty )
  {
    mCoo = QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( authId ), QgsProject::instance() ).transform( mCoo );
  }
  mCrs = QgsCoordinateReferenceSystem( authId );
  if ( !mEmpty )
  {
    mLineEdit->setText( KadasCoordinateFormat::instance()->getDisplayString( mCoo, mCrs, format, authId ) );
  }
  else
  {
    mLineEdit->setText( "" );
  }
  mLineEdit->setStyleSheet( "" );
}
