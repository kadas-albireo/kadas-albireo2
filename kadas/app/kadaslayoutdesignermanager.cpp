/***************************************************************************
    kadaslayoutdesignermanager.cpp
    ------------------------------
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

#include <qgis/qgsprintlayout.h>

#include "kadasapplication.h"
#include "kadaslayoutdesignerdialog.h"
#include "kadaslayoutdesignermanager.h"
#include "kadasmainwindow.h"

KadasLayoutDesignerManager *KadasLayoutDesignerManager::instance()
{
  static KadasLayoutDesignerManager i;
  return &i;
}

void KadasLayoutDesignerManager::openDesigner( QgsPrintLayout *layout )
{
  for ( QObject *dialogObj : mOpenDialogs )
  {
    KadasLayoutDesignerDialog *dialog = qobject_cast<KadasLayoutDesignerDialog *>( dialogObj );
    if ( dialog && dialog->currentLayout() == layout )
    {
      dialog->show();
      dialog->raise();
      dialog->activateWindow();
      dialog->setWindowState( Qt::WindowActive );
      return;
    }
  }

  KadasLayoutDesignerDialog *dialog = new KadasLayoutDesignerDialog( kApp->mainWindow() );
  connect( dialog, &QObject::destroyed, this, [this]( QObject *obj ) {
    mOpenDialogs.removeAll( obj );
  } );
  dialog->setCurrentLayout( layout );
  dialog->open();
  mOpenDialogs.append( dialog );
}

void KadasLayoutDesignerManager::closeAllDesigners()
{
  for ( QObject *dialogObj : mOpenDialogs )
  {
    dialogObj->blockSignals( true );
    delete dialogObj;
  }
  mOpenDialogs.clear();
}
