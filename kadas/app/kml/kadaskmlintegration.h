/***************************************************************************
    kadaskmlintegration.h
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

#ifndef KADASKMLINTEGRATION_H
#define KADASKMLINTEGRATION_H

#include <QObject>

#include <qgis/qgscustomdrophandler.h>

class QToolButton;

class KadasKmlDropHandler : public QgsCustomDropHandler {
  Q_OBJECT

public:
  bool canHandleMimeData(const QMimeData *data) override;
  bool handleMimeDataV2(const QMimeData *data) override;
};

class KadasKmlIntegration : public QObject {
  Q_OBJECT

public:
  KadasKmlIntegration(QToolButton *kmlButton, QObject *parent = nullptr);
  ~KadasKmlIntegration();

private:
  void exportToKml();
  void importFromKml();

  QToolButton *mKMLButton = nullptr;
  KadasKmlDropHandler mDropHandler;
};

#endif // KADASKMLINTEGRATION_H
