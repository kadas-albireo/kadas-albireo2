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

#include <QPainter>
#include <QSvgRenderer>

#include <qgis/qgsapplication.h>
#include <qgis/qgsgpsconnectionregistry.h>
#include <qgis/qgsgpsdetector.h>
#include <qgis/qgsnmeaconnection.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapcanvasitem.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>

#include "kadasgpsintegration.h"
#include "kadasgpssimulator.h"
#include "kadasmainwindow.h"

bool KadasGpsIntegration::sSimulatorEnabled = false;

//! Transient canvas overlay showing the GPS position as a rotatable arrow
class KadasGpsMarker : public QgsMapCanvasItem
{
  public:
    explicit KadasGpsMarker( QgsMapCanvas *canvas )
      : QgsMapCanvasItem( canvas )
      , mSvg( QStringLiteral( ":/kadas/icons/gpsarrow" ) )
    {
      setZValue( 200 );
    }

    void setGpsPosition( const QgsPointXY &mapPos, double direction )
    {
      mMapPos = mapPos;
      if ( !std::isnan( direction ) )
        mDirection = direction;
      updatePosition();
    }

    void updatePosition() override
    {
      prepareGeometryChange();
      setPos( toCanvasCoordinates( mMapPos ) );
      update();
    }

    QRectF boundingRect() const override
    {
      // half diagonal of the rotated icon
      const double r = SIZE * M_SQRT1_2;
      return QRectF( -r, -r, 2 * r, 2 * r );
    }

    void paint( QPainter *painter ) override
    {
      painter->save();
      painter->setRenderHint( QPainter::Antialiasing );
      painter->setRenderHint( QPainter::SmoothPixmapTransform );
      painter->rotate( mDirection );
      mSvg.render( painter, QRectF( -SIZE / 2., -SIZE / 2., SIZE, SIZE ) );
      painter->restore();
    }

  private:
    static constexpr double SIZE = 92;
    QSvgRenderer mSvg;
    QgsPointXY mMapPos;
    double mDirection = 0;
};

KadasGpsIntegration::KadasGpsIntegration( KadasMainWindow *mainWindow, QToolButton *gpsToolButton, QAction *actionEnableGps, QAction *actionMoveWithGps )
  : QObject( mainWindow )
  , mMainWindow( mainWindow )
  , mGpsToolButton( gpsToolButton )
  , mActionEnableGps( actionEnableGps )
  , mActionMoveWithGps( actionMoveWithGps )
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

  if ( sSimulatorEnabled )
  {
    QgsGpsConnection *connection = new QgsNmeaConnection( new KadasGpsSimulator() );
    if ( connection->connect() )
    {
      gpsConnected( connection );
    }
    else
    {
      delete connection;
      gpsConnectionFailed();
    }
    return;
  }

  QString port = QgsSettings().value( "/kadas/gps_port", "" ).toString();
  QgsGpsDetector *gpsDetector = new QgsGpsDetector( port, false ); // deletes itself automatically
  connect( gpsDetector, &QgsGpsDetector::connectionDetected, this, [this, gpsDetector]() {
    QgsGpsConnection *connection = gpsDetector->takeConnection();
    if ( connection )
      gpsConnected( connection );
  } );
  connect( gpsDetector, &QgsGpsDetector::detectionFailed, this, &KadasGpsIntegration::gpsConnectionFailed );
  gpsDetector->advance();
}

void KadasGpsIntegration::gpsConnected( QgsGpsConnection *connection )
{
  QgsApplication::gpsConnectionRegistry()->registerConnection( connection );
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
    QgsApplication::gpsConnectionRegistry()->unregisterConnection( mConnection );
    mConnection->close();
    delete mConnection;
    mConnection = nullptr;
    mMainWindow->messageBar()->pushMessage( tr( "GPS connection closed" ), QString(), Qgis::Info, mMainWindow->messageTimeout() );
  }
  setGPSIcon( Qt::black );
  mCurFixStatus = Qgis::GpsFixStatus::NoFix;
  delete mMarker;
  mMarker = nullptr;
}

void KadasGpsIntegration::gpsStateChanged( const QgsGpsInformation &info )
{
  Qgis::GnssConstellation constellation;
  Qgis::GpsFixStatus fixStatus = info.bestFixStatus( constellation );

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
    mMarker = new KadasGpsMarker( mMainWindow->mapCanvas() );
  }
  // Reproject WGS84 position to canvas CRS
  const QgsCoordinateTransform t( QgsCoordinateReferenceSystem( "EPSG:4326" ), mMainWindow->mapCanvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  QgsPointXY canvasPos;
  try
  {
    canvasPos = t.transform( position );
  }
  catch ( const QgsCsException & )
  {
    canvasPos = position;
  }
  mMarker->setGpsPosition( canvasPos, info.direction );
}

void KadasGpsIntegration::updateGpsFixIcon()
{
  switch ( mCurFixStatus )
  {
    case Qgis::GpsFixStatus::NoData:
      setGPSIcon( Qt::white );
      break;
    case Qgis::GpsFixStatus::NoFix:
      setGPSIcon( Qt::red );
      break;
    case Qgis::GpsFixStatus::Fix2D:
      setGPSIcon( Qt::yellow );
      break;
    case Qgis::GpsFixStatus::Fix3D:
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
