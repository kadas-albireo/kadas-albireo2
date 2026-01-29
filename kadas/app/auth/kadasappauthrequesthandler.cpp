/***************************************************************************
  kadasappauthrequesthandler.cpp
  ------------------------------
  Date                 : January 2026
  Copyright            : (C) 2026 by Damiano Lombardi
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

#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"

KadasAppAuthRequestHandler::KadasAppAuthRequestHandler( QObject *parent )
  : QObject( parent )
  , QgsNetworkAuthenticationHandler()
{
}

void KadasAppAuthRequestHandler::handleAuthRequestOpenBrowser( const QUrl &url )
{
  QDesktopServices::openUrl( url );
  QgsDebugMsgLevel( QString( "Opening OAuth2 URL in system browser: %1" ).arg( url.toString() ), 1 );

  emit browserOpened();
}

void KadasAppAuthRequestHandler::handleAuthRequestCloseBrowser()
{
  QgsDebugMsgLevel( QString( "OAuth2 handle close browser" ), 1 );

  emit browserClosed();

  // Bring focus back to KADAS app
  if ( KadasApplication::instance() )
  {
    KadasMainWindow *mainWindow = KadasApplication::instance()->mainWindow();
    mainWindow->raise();
    mainWindow->activateWindow();
    mainWindow->show();
  }
}

void KadasAppAuthRequestHandler::abortAuth()
{
  QgsDebugMsgLevel( QString( "OAuth2 authentication canceled by user" ), 1 );
  QgsNetworkAccessManager::instance()->abortAuthBrowser();
}
