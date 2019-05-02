/***************************************************************************
    kadascrssselection.cpp
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QStatusBar>
#include <QToolButton>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsprojectionselectiondialog.h>

#include <kadas/gui/kadascrsselection.h>


KadasCrsSelection::KadasCrsSelection( QWidget *parent )
    : QToolButton( parent ), mMapCanvas( 0 )
{
  QMenu* crsSelectionMenu = new QMenu( this );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:21781" ).description(), this, &KadasCrsSelection::setMapCrs )->setData( "EPSG:21781" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:2056" ).description(), this, &KadasCrsSelection::setMapCrs )->setData( "EPSG:2056" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:4326" ).description(), this, &KadasCrsSelection::setMapCrs )->setData( "EPSG:4326" );
  crsSelectionMenu->addAction( QgsCoordinateReferenceSystem( "EPSG:3857" ).description(), this, &KadasCrsSelection::setMapCrs )->setData( "EPSG:3857" );
  crsSelectionMenu->addSeparator();
  crsSelectionMenu->addAction( tr( "More..." ), this, &KadasCrsSelection::selectMapCrs );

  setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  setMenu( crsSelectionMenu );
  setPopupMode( QToolButton::InstantPopup );
}

void KadasCrsSelection::setMapCanvas( QgsMapCanvas* canvas )
{
  mMapCanvas = canvas;
  if ( mMapCanvas )
  {
    syncCrsButton();

    connect( mMapCanvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasCrsSelection::syncCrsButton );
    connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasCrsSelection::syncCrsButton );
  }
}

void KadasCrsSelection::syncCrsButton()
{
  if ( mMapCanvas )
  {
    QString authid = mMapCanvas->mapSettings().destinationCrs().authid();
    setText( QgsCoordinateReferenceSystem( authid ).description() );
  }
}

void KadasCrsSelection::selectMapCrs()
{
  if ( !mMapCanvas )
  {
    return;
  }
  QgsProjectionSelectionDialog projSelector;
  projSelector.setCrs( mMapCanvas->mapSettings().destinationCrs() );
  if ( projSelector.exec() != QDialog::Accepted )
  {
    return;
  }
  mMapCanvas->setDestinationCrs( projSelector.crs() );
  setText( mMapCanvas->mapSettings().destinationCrs().description() );
}

void KadasCrsSelection::setMapCrs()
{
  if ( !mMapCanvas )
  {
    return;
  }
  QAction* action = qobject_cast<QAction*>( QObject::sender() );
  QgsCoordinateReferenceSystem crs( action->data().toString() );
  mMapCanvas->setDestinationCrs( crs );
  setText( crs.description() );
}
