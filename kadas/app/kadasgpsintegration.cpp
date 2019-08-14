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

#include <qgis/qgsmessagebar.h>

#include <kadas/gui/kadascanvasgpsdisplay.h>
#include <kadas/app/kadasgpsintegration.h>
#include <kadas/app/kadasmainwindow.h>

KadasGpsIntegration::KadasGpsIntegration(KadasMainWindow *mainWindow)
  : QObject(mainWindow), mMainWindow(mainWindow)
{
  mCanvasGPSDisplay = new KadasCanvasGPSDisplay( mMainWindow->mapCanvas(), this );

  connect( mMainWindow->mGpsToolButton, &QToolButton::toggled, this, &KadasGpsIntegration::enableGPS );
  connect( mCanvasGPSDisplay, &KadasCanvasGPSDisplay::gpsConnected, this, &KadasGpsIntegration::gpsDetected );
  connect( mCanvasGPSDisplay, &KadasCanvasGPSDisplay::gpsDisconnected, this, &KadasGpsIntegration::gpsDisconnected );
  connect( mCanvasGPSDisplay, &KadasCanvasGPSDisplay::gpsConnectionFailed, this, &KadasGpsIntegration::gpsConnectionFailed );
  connect( mCanvasGPSDisplay, &KadasCanvasGPSDisplay::gpsFixStatusChanged, this, &KadasGpsIntegration::gpsFixChanged );
}

void KadasGpsIntegration::initGui()
{
  setGPSIcon( Qt::black );
}

void KadasGpsIntegration::enableGPS( bool enabled )
{
  mMainWindow->mGpsToolButton->blockSignals( true );
  mMainWindow->mActionEnableGPS->blockSignals( true );
  mMainWindow->mGpsToolButton->setChecked( enabled );
  mMainWindow->mActionEnableGPS->setChecked( enabled );
  mMainWindow->mGpsToolButton->blockSignals( false );
  mMainWindow->mActionEnableGPS->blockSignals( false );
  if ( enabled )
  {
    setGPSIcon( Qt::blue );
    mMainWindow->messageBar()->pushMessage( tr( "Connecting to GPS device..." ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
    mCanvasGPSDisplay->connectGPS();
  }
  else
  {
    setGPSIcon( Qt::black );
    mMainWindow->messageBar()->pushMessage( tr( "GPS connection closed" ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
    mCanvasGPSDisplay->disconnectGPS();
  }
}

void KadasGpsIntegration::gpsDetected()
{
  mMainWindow->messageBar()->pushMessage( tr( "GPS device successfully connected" ), Qgis::Info, mMainWindow->messageTimeout() );
  setGPSIcon( Qt::white );
}

void KadasGpsIntegration::gpsDisconnected()
{
  enableGPS( false );
}

void KadasGpsIntegration::gpsConnectionFailed()
{
  mMainWindow->messageBar()->pushMessage( tr( "Connection to GPS device failed" ), Qgis::Critical, mMainWindow->messageTimeout() );
  mMainWindow->mGpsToolButton->blockSignals( true );
  mMainWindow->mActionEnableGPS->blockSignals( true );
  mMainWindow->mGpsToolButton->setChecked( false );
  mMainWindow->mActionEnableGPS->setChecked( false );
  mMainWindow->mGpsToolButton->blockSignals( false );
  mMainWindow->mActionEnableGPS->blockSignals( false );
  setGPSIcon( Qt::black );
}

void KadasGpsIntegration::gpsFixChanged( QgsGpsInformation::FixStatus fixStatus )
{
  switch ( fixStatus )
  {
    case QgsGpsInformation::NoData:
      setGPSIcon( Qt::white ); break;
    case QgsGpsInformation::NoFix:
      setGPSIcon( Qt::red ); break;
    case QgsGpsInformation::Fix2D:
      setGPSIcon( Qt::yellow ); break;
    case QgsGpsInformation::Fix3D:
      setGPSIcon( Qt::green ); break;
  }
}

void KadasGpsIntegration::moveWithGPS( bool enabled )
{
  mCanvasGPSDisplay->setRecenterMap( enabled ? KadasCanvasGPSDisplay::WhenNeeded : KadasCanvasGPSDisplay::Never );
}

void KadasGpsIntegration::setGPSIcon( const QColor &color )
{
  QPixmap pixmap( mMainWindow->mGpsToolButton->size() );
  pixmap.fill( color );
  mMainWindow->mGpsToolButton->setIcon( QIcon( pixmap ) );
}
