/***************************************************************************
  kadasappauthrequesthandler.cpp
  ------------------------------
  Date                 : January 2026
  Copyright            : (C) 2025 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "kadasappauthrequesthandler.h"

#include <QDesktopServices>

#include "kadasapplication.h"
#include "kadasmainwindow.h"

void KadasAppAuthRequestHandler::handleAuthRequestOpenBrowser( const QUrl &url )
{
  QDesktopServices::openUrl( url );
}

void KadasAppAuthRequestHandler::handleAuthRequestCloseBrowser()
{
  // Bring focus back to KADAS app
  if ( KadasApplication::instance() )
  {
    KadasMainWindow *mainWindow = KadasApplication::instance()->mainWindow();
    mainWindow->raise();
    mainWindow->activateWindow();
    mainWindow->show();
  }
}
