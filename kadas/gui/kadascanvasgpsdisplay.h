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

#ifndef KADASCANVASGPSDISPLAY_H
#define KADASCANVASGPSDISPLAY_H

#include <QObject>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsgpsconnection.h>
#include <qgis/qgspoint.h>

#include <kadas/gui/kadas_gui.h>

class QgsGpsMarker;
class QgsMapCanvas;


/**Manages display of canvas gps marker and moving of map extent with changing GPS position*/
class KADAS_GUI_EXPORT KadasCanvasGPSDisplay : public QObject
{
    Q_OBJECT
  public:

    enum RecenterMode
    {
      Never,
      Always,
      WhenNeeded
    };

    KadasCanvasGPSDisplay( QgsMapCanvas *canvas, QObject *parent = nullptr );
    ~KadasCanvasGPSDisplay();

    void connectGPS();
    void disconnectGPS();

    void setMapCanvas( QgsMapCanvas *canvas ) { mCanvas = canvas; }

    RecenterMode recenterMap() const { return mRecenterMap; }
    void setRecenterMap( RecenterMode m ) { mRecenterMap = m; }

    bool showMarker() const { return mShowMarker; }
    void setShowMarker( bool showMarker ) { mShowMarker = showMarker; removeMarker(); }

    int markerSize() const { return mMarkerSize; }
    void setMarkerSize( int size ) { mMarkerSize = size; }

    double spinMapExtentMultiplier() const { return mSpinMapExtentMultiplier; }
    void setSpinMapExtentMultiplier( double value ) { mSpinMapExtentMultiplier = value; }

    QString port() const { return mPort; }
    /**Sets the port for the GPS connection. Empty string (default) means autodetect. For gpsd connections, use '<host>:<port>:<device>'.
    To use an integrated gps (e.g. on tablet or mobile), set 'internalGPS'*/
    void setPort( const QString &port ) { mPort = port; }

    QgsGpsInformation currentGPSInformation() const;
    QgsGpsInformation::FixStatus currentFixStatus() const { return mCurFixStatus; }

  private slots:
    void gpsDetected( QgsGpsConnection *conn );
    void gpsDetectionFailed();
    void updateGPSInformation( const QgsGpsInformation &info );

  signals:
    void gpsConnected();
    void gpsDisconnected();
    void gpsConnectionFailed();
    void gpsFixStatusChanged( QgsGpsInformation::FixStatus status );

    void gpsInformationReceived( const QgsGpsInformation &info );
    void nmeaSentenceReceived( const QString &substring );

  private:
    QgsMapCanvas *mCanvas = nullptr;
    bool mShowMarker = true;
    QgsGpsMarker *mMarker = nullptr;
    int mMarkerSize = 24;
    int mSpinMapExtentMultiplier;
    QgsPoint mLastGPSPosition;
    QgsCoordinateReferenceSystem mWgs84CRS;
    QgsGpsInformation::FixStatus mCurFixStatus = QgsGpsInformation::NoFix;

    /**Port for the GPS connectio*/
    QString mPort;
    QgsGpsConnection *mConnection = nullptr;
    RecenterMode mRecenterMap = Never;

    void closeGPSConnection();

    void removeMarker();

    //read default settings
    static int defaultMarkerSize();
    static int defaultSpinMapExtentMultiplier();
    static RecenterMode defaultRecenterMode();
};

#endif // KADASCANVASGPSDISPLAY_H
