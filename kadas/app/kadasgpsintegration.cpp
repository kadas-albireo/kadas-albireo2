/***************************************************************************
    kadasgpsintegration.cpp
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

#include <qgis/qgsgpsdetector.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/app/kadasgpsintegration.h>
#include <kadas/app/kadasmainwindow.h>

KadasGpsIntegration::KadasGpsIntegration( KadasMainWindow *mainWindow, QToolButton *gpsToolButton, QAction *actionEnableGps, QAction *actionMoveWithGps )
  : QObject( mainWindow ), mMainWindow( mainWindow ), mGpsToolButton( gpsToolButton ), mActionEnableGps( actionEnableGps ), mActionMoveWithGps( actionMoveWithGps )
{
  connect( mActionEnableGps, &QAction::triggered, this, &KadasGpsIntegration::enableGPS );
  connect( mGpsToolButton, &QToolButton::toggled, this, &KadasGpsIntegration::enableGPS );
}

KadasGpsIntegration::~KadasGpsIntegration()
{
  if ( mConnection )
  {
    mConnection->close();
    delete mConnection;
  }
  delete mMarker;
}

void KadasGpsIntegration::initGui()
{
  setGPSIcon( Qt::black );
}

void KadasGpsIntegration::enableGPS( bool enabled )
{
  mGpsToolButton->blockSignals( true );
  mActionEnableGps->blockSignals( true );
  mGpsToolButton->setChecked( enabled );
  mActionEnableGps->setChecked( enabled );
  mGpsToolButton->blockSignals( false );
  mActionEnableGps->blockSignals( false );
  if ( enabled )
  {
    connectGPS();
  }
  else
  {
    disconnectGPS();
  }
}

void KadasGpsIntegration::connectGPS()
{
  setGPSIcon( Qt::blue );
  mMainWindow->messageBar()->pushMessage( tr( "Connecting to GPS device..." ), QString(), Qgis::Info, mMainWindow->messageTimeout() );

  disconnectGPS(); // cleanup
  QString port = QgsSettings().value( "/kadas/gps_port", "" ).toString();
  QgsGpsDetector *gpsDetector = new QgsGpsDetector( port ); // deletes itself automatically
  connect( gpsDetector, qOverload<QgsGpsConnection *> ( &QgsGpsDetector::detected ), this, &KadasGpsIntegration::gpsConnected );
  connect( gpsDetector, &QgsGpsDetector::detectionFailed, this, &KadasGpsIntegration::gpsConnectionFailed );
  gpsDetector->advance();
}

void KadasGpsIntegration::gpsConnected( QgsGpsConnection *connection )
{
  setGPSIcon( Qt::white );
  mMainWindow->messageBar()->pushMessage( tr( "GPS device successfully connected" ), Qgis::Info, mMainWindow->messageTimeout() );

  mConnection = connection;
  connect( connection, &QgsGpsConnection::stateChanged, this, &KadasGpsIntegration::gpsStateChanged );
}

void KadasGpsIntegration::gpsConnectionFailed()
{
  mMainWindow->messageBar()->pushMessage( tr( "Connection to GPS device failed" ), Qgis::Critical, mMainWindow->messageTimeout() );
  enableGPS( false );
}

void KadasGpsIntegration::disconnectGPS()
{
  if ( mConnection )
  {
    mConnection->close();
    delete mConnection;
    mConnection = nullptr;
    mMainWindow->messageBar()->pushMessage( tr( "GPS connection closed" ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
  }
  setGPSIcon( Qt::black );
  mCurFixStatus = QgsGpsInformation::NoFix;
  delete mMarker;
  mMarker = nullptr;
}

void KadasGpsIntegration::gpsStateChanged( const QgsGpsInformation &info )
{
  QgsGpsInformation::FixStatus fixStatus = info.fixStatus();

  if ( fixStatus != mCurFixStatus )
  {
    mCurFixStatus = fixStatus;
    updateGpsFixIcon();
  }

  if ( !info.isValid() )
  {
    return;
  }

  QgsPointXY position( info.longitude, info.latitude );

  // Move with GPS if requested
  if ( mActionMoveWithGps->isChecked() )
  {
    //recenter map
    QgsCoordinateReferenceSystem destCRS = mMainWindow->mapCanvas()->mapSettings().destinationCrs();
    QgsCoordinateTransform myTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), destCRS, QgsProject::instance() );

    QgsPointXY centerPoint = myTransform.transform( position );
    QgsRectangle myRect( centerPoint, centerPoint );

    // testing if position is outside some proportion of the map extent
    // this is a user setting - useful range: 5% to 100% (0.05 to 1.0)
    QgsRectangle extentLimit( mMainWindow->mapCanvas()->extent() );
    int extentTolerance = QgsSettings().value( "/kadas/gps_autocenter_tolerance", 10 ).toInt();
    extentLimit.scale( extentTolerance / 100. );

    if ( !extentLimit.contains( centerPoint ) )
    {
      mMainWindow->mapCanvas()->setExtent( myRect );
    }
  }

  // Update marker
  if ( !mMarker )
  {
    mMarker = new KadasSymbolItem( QgsCoordinateReferenceSystem( "EPSG:4326" ) );
    mMarker->setup( ":/kadas/icons/gpsarrow", 0.5, 0.5, 92, 92 );
    KadasMapCanvasItemManager::addItem( mMarker );
  }
  mMarker->setPosition( KadasItemPos::fromPoint( position ) );
  mMarker->setAngle( -info.direction );

}

void KadasGpsIntegration::updateGpsFixIcon()
{
  switch ( mCurFixStatus )
  {
    case QgsGpsInformation::NoData:
      setGPSIcon( Qt::white );
      break;
    case QgsGpsInformation::NoFix:
      setGPSIcon( Qt::red );
      break;
    case QgsGpsInformation::Fix2D:
      setGPSIcon( Qt::yellow );
      break;
    case QgsGpsInformation::Fix3D:
      setGPSIcon( QColor( 87, 175, 87 ) );
      break;
  }
}

void KadasGpsIntegration::setGPSIcon( const QColor &color )
{
  QPixmap pixmap( mGpsToolButton->size() );
  pixmap.fill( color );
  mGpsToolButton->setIcon( QIcon( pixmap ) );
}
