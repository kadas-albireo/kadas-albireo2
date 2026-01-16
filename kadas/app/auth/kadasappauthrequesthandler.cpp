/***************************************************************************
    qgsappauthrequesthandler.cpp
    ---------------------------
    begin                : January 2019
    copyright            : (C) 2019 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadasappauthrequesthandler.h"

#include <QAuthenticator>
#include <QDesktopServices>
#include <QWidget>

#include <qgis/qgsapplication.h>
#include <qgis/qgsauthcertutils.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgscredentials.h>
#include <qgis/qgslogger.h>

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
