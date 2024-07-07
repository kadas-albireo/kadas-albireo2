/***************************************************************************
    KadasTemporalController.h
    --------------------
    copyright            : (C) 2019 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
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

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadastemporalcontroller.h>


KadasTemporalController::KadasTemporalController( QgsMapCanvas *canvas )
  : KadasBottomBar( canvas )
{
  setupUi( this );

  mQgsTemporalControllerWidget = new QgsTemporalControllerWidget(this);
  mTemporalControllerVerticalLayout->addWidget(mQgsTemporalControllerWidget);

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




