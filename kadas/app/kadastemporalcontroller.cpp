/***************************************************************************
  kadastemporalcontroller.cpp
  --------------------------------------
  Date                 : July 2024
  Copyright            : (C) 2024 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAction>
#include <QMenu>

#include <qgis/qgstemporalcontrollerwidget.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadastemporalcontroller.h"


KadasTemporalController::KadasTemporalController( QgsMapCanvas *canvas )
  : KadasBottomBar( canvas )
{
  setupUi( this );

  mQgsTemporalControllerWidget = new QgsTemporalControllerWidget( this );
  mTemporalControllerVerticalLayout->addWidget( mQgsTemporalControllerWidget );

  canvas->setTemporalController( mQgsTemporalControllerWidget->temporalController() );

  mCloseButton->setIcon( QIcon( ":/kadas/icons/close" ) );
}

KadasTemporalController::~KadasTemporalController()
{
  delete mQgsTemporalControllerWidget;
}

void KadasTemporalController::on_mCloseButton_clicked()
{
  hide();
}
