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

#include <QHBoxLayout>

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadasbottombar.h"

KadasBottomBar::KadasBottomBar( QgsMapCanvas *canvas, const QString &color )
  : QgsFloatingWidget( canvas )
{
  setAnchorWidget( canvas );
  setAnchorPoint( QgsFloatingWidget::BottomMiddle );
  setAnchorWidgetPoint( QgsFloatingWidget::BottomMiddle );

  QHBoxLayout *containerLayout = new QHBoxLayout();
  containerLayout->setContentsMargins( 0, 0, 0, 0 );
  setObjectName( "QgsBottomBar" );
  setStyleSheet( QString( "QWidget#QgsBottomBar { background-color: %1; }" ).arg( color ) );
  setCursor( Qt::ArrowCursor );
}
