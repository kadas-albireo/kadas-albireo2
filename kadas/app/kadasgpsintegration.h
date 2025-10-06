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
class KadasMainWindow;
class KadasSymbolItem;

class KadasGpsIntegration : public QObject {
  Q_OBJECT
public:
  KadasGpsIntegration(KadasMainWindow *mainWindow, QToolButton *gpsToolButton,
                      QAction *actionEnableGps, QAction *actionMoveWithGps);
  ~KadasGpsIntegration();
  void initGui();

private:
  void connectGPS();
  void disconnectGPS();
  void updateGpsFixIcon();
  void setGPSIcon(const QColor &color);

  KadasMainWindow *mMainWindow;
  QToolButton *mGpsToolButton = nullptr;
  QAction *mActionEnableGps = nullptr;
  QAction *mActionMoveWithGps = nullptr;
  QgsGpsConnection *mConnection = nullptr;
  KadasSymbolItem *mMarker = nullptr;
  Qgis::GpsFixStatus mCurFixStatus = Qgis::GpsFixStatus::NoFix;

private slots:
  void enableGPS(bool enabled);
  void gpsConnected(QgsGpsConnection *connection);
  void gpsConnectionFailed();
  void gpsStateChanged(const QgsGpsInformation &info);
};

#endif // KADASGPSINTEGRATION_H
