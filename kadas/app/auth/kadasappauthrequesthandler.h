/***************************************************************************
  kadasappauthrequesthandler.h
  ----------------------------
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

#ifndef KADASAPPAUTHREQUESTHANDLER_H
#define KADASAPPAUTHREQUESTHANDLER_H

#include "qgsnetworkaccessmanager.h"

/**
 * \class KadasAppAuthRequestHandler
 * \ingroup app
 * \brief Reimplement open/close browser method for OAuth2 authentication
 * \see QgsAppAuthRequestHandler
 */
class KadasAppAuthRequestHandler : public QObject, public QgsNetworkAuthenticationHandler
{
    Q_OBJECT

  public:
    explicit KadasAppAuthRequestHandler( QObject *parent = nullptr );

    void handleAuthRequestOpenBrowser( const QUrl &url ) override;
    void handleAuthRequestCloseBrowser() override;

  public slots:
    void abortAuth();

  signals:
    void browserOpened();
    void browserClosed();
};


#endif // KADASAPPAUTHREQUESTHANDLER_H
