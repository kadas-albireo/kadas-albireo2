/***************************************************************************
    kadascanvasgpsdisplay.h
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

#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsgpsconnection.h>
#include <qgis/qgsgpsconnectionregistry.h>
#include <qgis/qgsgpsdetector.h>
#include <qgis/qgsgpsmarker.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>


#include <kadas/gui/kadascanvasgpsdisplay.h>

KadasCanvasGPSDisplay::KadasCanvasGPSDisplay( QgsMapCanvas *canvas, QObject *parent )
  : QObject( parent )
{
  mMarkerSize = defaultMarkerSize();
  mSpinMapExtentMultiplier = defaultSpinMapExtentMultiplier();
  mRecenterMap = defaultRecenterMode();
  mWgs84CRS.createFromOgcWmsCrs( "EPSG:4326" );
}

KadasCanvasGPSDisplay::~KadasCanvasGPSDisplay()
{
  closeGPSConnection();

  QgsSettings s;
  s.setValue( "/gps/markerSize", mMarkerSize );
  s.setValue( "/gps/mapExtentMultiplier", mSpinMapExtentMultiplier );
  QString panModeString = "none";
  if ( mRecenterMap == KadasCanvasGPSDisplay::Always )
  {
    panModeString = "recenterAlways";
  }
  else if ( mRecenterMap == KadasCanvasGPSDisplay::WhenNeeded )
  {
    panModeString = "recenterWhenNeeded";
  }
  s.setValue( "/gps/panMode", panModeString );
}

void KadasCanvasGPSDisplay::connectGPS()
{
  closeGPSConnection();
  QgsGpsDetector *gpsDetector = new QgsGpsDetector( mPort );
  connect( gpsDetector, qOverload<QgsGpsConnection *> ( &QgsGpsDetector::detected ), this, &KadasCanvasGPSDisplay::gpsDetected );
  connect( gpsDetector, &QgsGpsDetector::detectionFailed, this, &KadasCanvasGPSDisplay::gpsDetectionFailed );
  gpsDetector->advance();
}

void KadasCanvasGPSDisplay::disconnectGPS()
{
  closeGPSConnection();
  mCurFixStatus = QgsGpsInformation::NoFix;
}

void KadasCanvasGPSDisplay::closeGPSConnection()
{
  if ( mConnection )
  {
    QgsApplication::gpsConnectionRegistry()->unregisterConnection( mConnection );
    mConnection->close();
    delete mConnection;
    mConnection = 0;
    emit gpsDisconnected();
  }
  removeMarker();
}

void KadasCanvasGPSDisplay::gpsDetected( QgsGpsConnection *conn )
{
  mConnection = conn;
  QgsApplication::gpsConnectionRegistry()->registerConnection( mConnection );
  connect( conn, &QgsGpsConnection::stateChanged, this, &KadasCanvasGPSDisplay::updateGPSInformation );
  connect( conn, &QgsGpsConnection::nmeaSentenceReceived, this, &KadasCanvasGPSDisplay::nmeaSentenceReceived );
  emit gpsConnected();
}

void KadasCanvasGPSDisplay::gpsDetectionFailed()
{
  emit gpsConnectionFailed();
}

void KadasCanvasGPSDisplay::updateGPSInformation( const QgsGpsInformation &info )
{
  QgsGpsInformation::FixStatus fixStatus = info.fixStatus();

  if ( fixStatus != mCurFixStatus )
  {
    emit gpsFixStatusChanged( fixStatus );
  }
  mCurFixStatus = fixStatus;

  emit gpsInformationReceived( info );  //send signal for service who want to do further actions (e.g. satellite position display, digitising, ...)

  if ( !mCanvas || !info.isValid() )
  {
    return;
  }

  QgsPoint position( info.longitude, info.latitude );
  if ( mRecenterMap != Never && mLastGPSPosition != position )
  {
    //recenter map
    QgsCoordinateReferenceSystem destCRS = mCanvas->mapSettings().destinationCrs();
    QgsCoordinateTransform myTransform( mWgs84CRS, destCRS, QgsProject::instance() );  // use existing WGS84 CRS

    QgsPointXY centerPoint = myTransform.transform( position );
    QgsRectangle myRect( centerPoint, centerPoint );

    // testing if position is outside some proportion of the map extent
    // this is a user setting - useful range: 5% to 100% (0.05 to 1.0)
    QgsRectangle extentLimit( mCanvas->extent() );
    extentLimit.scale( mSpinMapExtentMultiplier * 0.01 );

    if ( mRecenterMap == Always || !extentLimit.contains( centerPoint ) )
    {
      mCanvas->setExtent( myRect );
      mCanvas->refresh();
    }
    mLastGPSPosition = position;
  }

  if ( mShowMarker )
  {
    if ( ! mMarker )
    {
      mMarker = new QgsGpsMarker( mCanvas );
    }
    mMarker->setSize( mMarkerSize );
    mMarker->setCenter( position );
    mMarker->setDirection( info.direction );
  }
}

int KadasCanvasGPSDisplay::defaultMarkerSize()
{
  return QgsSettings().value( "/gps/markerSize", "24" ).toInt();
}

int KadasCanvasGPSDisplay::defaultSpinMapExtentMultiplier()
{
  return QgsSettings().value( "/gps/mapExtentMultiplier", "50" ).toInt();
}

void KadasCanvasGPSDisplay::removeMarker()
{
  delete mMarker;
  mMarker = 0;
}

QgsGpsInformation KadasCanvasGPSDisplay::currentGPSInformation() const
{
  if ( mConnection )
  {
    return mConnection->currentGPSInformation();
  }

  return QgsGpsInformation();
}

KadasCanvasGPSDisplay::RecenterMode KadasCanvasGPSDisplay::defaultRecenterMode()
{
  QString panMode = QgsSettings().value( "/gps/panMode", "none" ).toString();
  if ( panMode == "recenterAlways" )
  {
    return KadasCanvasGPSDisplay::Always;
  }
  else if ( panMode == "none" )
  {
    return KadasCanvasGPSDisplay::Never;
  }
  else     //recenterWhenNeeded
  {
    return KadasCanvasGPSDisplay::WhenNeeded;
  }
}
