/***************************************************************************
    kadas3dintegration.h
    -----------------------
    copyright            : (C) 2024 Matthias Kuhn
    email                : matthias@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADAS3DINTEGRATION_H
#define KADAS3DINTEGRATION_H

#include <QObject>

class QAction;
class Kadas3DMapCanvas;
class Kadas3DMapCanvasWidget;
class QgsMapCanvas;

class Kadas3DIntegration : public QObject
{
    Q_OBJECT

  public:
    Kadas3DIntegration( QAction *action3D, QgsMapCanvas *mapCanvas, QObject *parent = nullptr );

  private:
    QAction *mAction3D = nullptr;
    QgsMapCanvas *mMapCanvas = nullptr;
    Kadas3DMapCanvasWidget *m3DMapCanvasWidget = nullptr;

    Kadas3DMapCanvasWidget *createNewMapCanvas3D( const QString &name );
};

#endif // KADAS3DINTEGRATION_H
