/***************************************************************************
    kadasgpssimulator.cpp
    ---------------------
    copyright            : (C) 2026 by Sourcepole AG
    email                : info at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cmath>

#include <QDateTime>

#include "kadasgpssimulator.h"

KadasGpsSimulator::KadasGpsSimulator( QObject *parent )
  : QIODevice( parent )
{
  // Scenic loop over Switzerland (WGS84 lon/lat)
  mRoute = {
    QgsPointXY( 7.4474, 46.9480 ), // Bern
    QgsPointXY( 7.6280, 46.7580 ), // Thun
    QgsPointXY( 7.8632, 46.6863 ), // Interlaken
    QgsPointXY( 8.0306, 46.7549 ), // Brienz
    QgsPointXY( 8.2457, 46.8961 ), // Sarnen
    QgsPointXY( 8.3093, 47.0502 ), // Lucerne
    QgsPointXY( 8.5156, 47.1662 ), // Zug
    QgsPointXY( 8.5417, 47.3769 ), // Zurich
    QgsPointXY( 8.0457, 47.3925 ), // Aarau
    QgsPointXY( 7.5323, 47.2088 ), // Solothurn
  };
  mPos = mRoute[0];

  mTimer.setInterval( 1000 );
  connect( &mTimer, &QTimer::timeout, this, &KadasGpsSimulator::emitSentences );
}

bool KadasGpsSimulator::open( OpenMode mode )
{
  if ( !QIODevice::open( mode ) )
    return false;
  mTimer.start();
  emitSentences();
  return true;
}

void KadasGpsSimulator::close()
{
  mTimer.stop();
  QIODevice::close();
}

qint64 KadasGpsSimulator::bytesAvailable() const
{
  return mBuffer.size() + QIODevice::bytesAvailable();
}

qint64 KadasGpsSimulator::readData( char *data, qint64 maxSize )
{
  const qint64 n = std::min<qint64>( maxSize, mBuffer.size() );
  std::memcpy( data, mBuffer.constData(), n );
  mBuffer.remove( 0, n );
  return n;
}

qint64 KadasGpsSimulator::writeData( const char *, qint64 size )
{
  return size;
}

void KadasGpsSimulator::advancePosition( double dtSecs )
{
  // Smoothly vary speed between ~5 and ~55 m/s via two incommensurate sine waves.
  mElapsedSecs += dtSecs;
  mSpeedMps = 30. + 20. * std::sin( 2 * M_PI * mElapsedSecs / 120. ) + 5. * std::sin( 2 * M_PI * mElapsedSecs / 47. );

  double remaining = mSpeedMps * dtSecs;
  while ( remaining > 0 )
  {
    const QgsPointXY target = mRoute[mNextWaypoint];
    // Local equirectangular approximation
    const double mPerDegLat = 111320.;
    const double mPerDegLon = mPerDegLat * std::cos( mPos.y() * M_PI / 180. );
    const double dxM = ( target.x() - mPos.x() ) * mPerDegLon;
    const double dyM = ( target.y() - mPos.y() ) * mPerDegLat;
    const double dist = std::hypot( dxM, dyM );
    if ( dist <= remaining )
    {
      mPos = target;
      mNextWaypoint = ( mNextWaypoint + 1 ) % mRoute.size();
      remaining -= dist;
      continue;
    }
    mCourse = std::fmod( std::atan2( dxM, dyM ) * 180. / M_PI + 360., 360. );
    mPos.setX( mPos.x() + dxM / dist * remaining / mPerDegLon );
    mPos.setY( mPos.y() + dyM / dist * remaining / mPerDegLat );
    remaining = 0;
  }
}

QByteArray KadasGpsSimulator::nmeaSentence( const QString &payload )
{
  int checksum = 0;
  for ( const QChar &c : payload )
    checksum ^= c.toLatin1();
  return QStringLiteral( "$%1*%2\r\n" ).arg( payload, QStringLiteral( "%1" ).arg( checksum, 2, 16, QLatin1Char( '0' ) ).toUpper() ).toLatin1();
}

void KadasGpsSimulator::emitSentences()
{
  advancePosition( mTimer.interval() / 1000. );

  const QDateTime now = QDateTime::currentDateTimeUtc();
  const QString time = now.toString( QStringLiteral( "hhmmss.zzz" ) ).left( 9 );
  const QString date = now.toString( QStringLiteral( "ddMMyy" ) );

  const double lat = mPos.y(), lon = mPos.x();
  const int latDeg = static_cast<int>( std::fabs( lat ) );
  const int lonDeg = static_cast<int>( std::fabs( lon ) );
  const QString latStr = QStringLiteral( "%1%2" ).arg( latDeg, 2, 10, QLatin1Char( '0' ) ).arg( ( std::fabs( lat ) - latDeg ) * 60., 7, 'f', 4, QLatin1Char( '0' ) );
  const QString lonStr = QStringLiteral( "%1%2" ).arg( lonDeg, 3, 10, QLatin1Char( '0' ) ).arg( ( std::fabs( lon ) - lonDeg ) * 60., 7, 'f', 4, QLatin1Char( '0' ) );
  const QString latHemi = lat >= 0 ? QStringLiteral( "N" ) : QStringLiteral( "S" );
  const QString lonHemi = lon >= 0 ? QStringLiteral( "E" ) : QStringLiteral( "W" );
  const double speedKnots = mSpeedMps * 1.94384;

  mBuffer += nmeaSentence( QStringLiteral( "GPGGA,%1,%2,%3,%4,%5,1,08,0.9,%6,M,48.0,M,," ).arg( time, latStr, latHemi, lonStr, lonHemi ).arg( mAltitude, 0, 'f', 1 ) );
  mBuffer += nmeaSentence( QStringLiteral( "GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,1.5,0.9,1.2" ) );
  mBuffer += nmeaSentence( QStringLiteral( "GPRMC,%1,A,%2,%3,%4,%5,%6,%7,%8,,,A" ).arg( time, latStr, latHemi, lonStr, lonHemi ).arg( speedKnots, 0, 'f', 1 ).arg( mCourse, 0, 'f', 1 ).arg( date ) );

  emit readyRead();
}
