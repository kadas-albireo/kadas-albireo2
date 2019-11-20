/***************************************************************************
    kadasgpsintegration.h
    ---------------------
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

#ifndef KADASGPSINTEGRATION_H
#define KADASGPSINTEGRATION_H

#include <QObject>

#include <qgis/qgsgpsconnection.h>

class QAction;
class QToolButton;
class KadasCanvasGPSDisplay;
class KadasMainWindow;

class KadasGpsIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasGpsIntegration( KadasMainWindow *mainWindow, QToolButton *gpsToolButton, QAction *actionEnableGps );
    void initGui();

  public slots:
    void enableGPS( bool enabled );
    void moveWithGPS( bool enabled );

  private:
    void initGPSDisplay();
    void setGPSIcon( const QColor &color );

    KadasMainWindow *mMainWindow;
    KadasCanvasGPSDisplay *mCanvasGPSDisplay = nullptr;
    QToolButton *mGpsToolButton = nullptr;
    QAction *mActionEnableGps = nullptr;

  private slots:
    void gpsDetected();
    void gpsDisconnected();
    void gpsConnectionFailed();
    void gpsFixChanged( QgsGpsInformation::FixStatus fixStatus );
};

#endif // KADASGPSINTEGRATION_H
