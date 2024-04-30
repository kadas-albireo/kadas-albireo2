/***************************************************************************
  qgs3dmaptool.h
  --------------------------------------
  Date                 : Sep 2018
  Copyright            : (C) 2018 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADAS3DMAPTOOL_H
#define KADAS3DMAPTOOL_H

#include <QObject>


class Kadas3DMapCanvas;
class QMouseEvent;
class QKeyEvent;

/**
 * Base class for map tools operating on 3D map canvas.
 */
class Kadas3DMapTool : public QObject
{
    Q_OBJECT

  public:
    Kadas3DMapTool( Kadas3DMapCanvas *canvas );

    virtual void mousePressEvent( QMouseEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *event );
    virtual void mouseMoveEvent( QMouseEvent *event );
    virtual void keyPressEvent( QKeyEvent *event );

    //! Called when set as currently active map tool
    virtual void activate();

    //! Called when map tool is being deactivated
    virtual void deactivate();

    //! Mouse cursor to be used when the tool is active
    virtual QCursor cursor() const;

    /**
     * Whether the default mouse controls to zoom/pan/rotate camera can stay enabled
     * while the tool is active. This may be useful for some basic tools using just
     * mouse clicks (e.g. identify, measure), but it could be creating conflicts when used
     * with more advanced tools. Default implementation returns TRUE.
     */
    virtual bool allowsCameraControls() const { return true; }

    Kadas3DMapCanvas *canvas();

  private slots:
    //! Called when canvas's map setting is changed
    virtual void onMapSettingsChanged();

  protected:
    Kadas3DMapCanvas *mCanvas = nullptr;
};

#endif // KADAS3DMAPTOOL_H
