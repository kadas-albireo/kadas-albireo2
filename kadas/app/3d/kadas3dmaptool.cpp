/***************************************************************************
  qgs3dmaptool.cpp
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

#include "kadas/app/3d/kadas3dmaptool.h"
#include "kadas/app/3d/kadas3dmapcanvas.h"

Kadas3DMapTool::Kadas3DMapTool( Kadas3DMapCanvas *canvas )
  : QObject( canvas )
  , mCanvas( canvas )
{
}

void Kadas3DMapTool::mousePressEvent( QMouseEvent *event )
{
  Q_UNUSED( event )
}

void Kadas3DMapTool::mouseReleaseEvent( QMouseEvent *event )
{
  Q_UNUSED( event )
}

void Kadas3DMapTool::mouseMoveEvent( QMouseEvent *event )
{
  Q_UNUSED( event )
}

void Kadas3DMapTool::keyPressEvent( QKeyEvent *event )
{
  Q_UNUSED( event )
}

void Kadas3DMapTool::activate()
{
}

void Kadas3DMapTool::deactivate()
{
}

QCursor Kadas3DMapTool::cursor() const
{
  return Qt::CrossCursor;
}

void Kadas3DMapTool::onMapSettingsChanged()
{

}

Kadas3DMapCanvas *Kadas3DMapTool::canvas()
{
  return mCanvas;
}
