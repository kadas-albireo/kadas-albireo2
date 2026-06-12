/***************************************************************************
    kadasgpssimulator.h
    -------------------
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

#ifndef KADASGPSSIMULATOR_H
#define KADASGPSSIMULATOR_H

#include <QIODevice>
#include <QTimer>
#include <QVector>

#include <qgis/qgspointxy.h>

/**
 * A QIODevice which emits NMEA sentences simulating a GPS receiver moving
 * along a route over Switzerland. Intended to be fed to a QgsNmeaConnection
 * to test the GPS integration without real hardware (--gps-simulator).
 */
class KadasGpsSimulator : public QIODevice
{
    Q_OBJECT
  public:
    explicit KadasGpsSimulator( QObject *parent = nullptr );

    bool isSequential() const override { return true; }
    bool open( OpenMode mode ) override;
    void close() override;
    qint64 bytesAvailable() const override;

  protected:
    qint64 readData( char *data, qint64 maxSize ) override;
    qint64 writeData( const char *data, qint64 size ) override;

  private:
    void emitSentences();
    void advancePosition( double dtSecs );
    static QByteArray nmeaSentence( const QString &payload );

    QTimer mTimer;
    QByteArray mBuffer;
    QVector<QgsPointXY> mRoute; // WGS84 lon/lat waypoints
    QgsPointXY mPos;            // current WGS84 lon/lat
    int mNextWaypoint = 1;
    double mCourse = 0;       // degrees, clockwise from north
    double mSpeedMps = 40;    // ~144 km/h
    double mAltitude = 555.0; // m AMSL
};

#endif // KADASGPSSIMULATOR_H
