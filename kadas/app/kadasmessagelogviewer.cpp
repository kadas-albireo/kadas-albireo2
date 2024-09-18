/***************************************************************************
    kadasmessagelogviewer.cpp
    -------------------------
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

#include <kadasmessagelogviewer.h>

KadasMessageLogViewer::KadasMessageLogViewer( QWidget *parent, Qt::WindowFlags fl )
  : QgsMessageLogViewer( parent, fl )
{
  setWindowTitle( tr( "KADAS Message Log" ) );
}

void KadasMessageLogViewer::closeEvent( QCloseEvent *e )
{
  QDialog::closeEvent( e );
}

void KadasMessageLogViewer::reject()
{
  QDialog::reject();
}
