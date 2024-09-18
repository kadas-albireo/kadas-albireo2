/***************************************************************************
    kadasbottombar.cpp
    ------------------
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

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadasbottombar.h"

KadasBottomBar::KadasBottomBar( QgsMapCanvas *canvas, const QString &color )
  : QFrame( canvas )
  , mCanvas( canvas )
{
  mCanvas->installEventFilter( this );

  setObjectName( "QgsBottomBar" );
  setStyleSheet( QString( "QFrame#QgsBottomBar { background-color: %1; }" ).arg( color ) );
  setCursor( Qt::ArrowCursor );
}

bool KadasBottomBar::eventFilter( QObject *obj, QEvent *event )
{
  if ( obj == mCanvas && event->type() == QEvent::Resize )
  {
    updatePosition();
  }
  return QObject::eventFilter( obj, event );
}

void KadasBottomBar::showEvent( QShowEvent * /*event*/ )
{
  setFixedSize( size() );
  updatePosition();
}

void KadasBottomBar::updatePosition()
{
  int w = width();
  int h = height();
  QRect canvasGeometry = mCanvas->geometry();
  move( canvasGeometry.x() + 0.5 * ( canvasGeometry.width() - w ),
        canvasGeometry.y() + canvasGeometry.height() - h );
}
