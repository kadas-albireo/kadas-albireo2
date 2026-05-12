/***************************************************************************
  kadasportaltokenfetcher.h
  -------------------------
  Date                 : May 2026
  Copyright            : (C) 2026 by Sourcepole AG
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASPORTALTOKENFETCHER_H
#define KADASPORTALTOKENFETCHER_H

#include <QByteArray>
#include <QString>

class QUrl;

/**
 * Result of a portal token fetch.
 */
struct KadasPortalTokenFetchResult
{
    bool ok = false;      //!< True if the HTTP exchange succeeded (regardless of token content)
    QByteArray body;      //!< Response body (expected to be JSON on success)
    QString errorMessage; //!< Human-readable error message on failure
    int httpStatus = 0;   //!< HTTP status code, 0 if no response was received
};

/**
 * Fetches the portal token URL using single sign-on with the current OS user
 * credentials when supported by the platform.
 *
 * On Windows this uses WinHTTP with WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW so
 * that the request silently negotiates NTLM/Kerberos with the logged-in user's
 * Windows session, restoring the pre-Qt6 behavior of QNetworkAccessManager.
 *
 * On other platforms (and when WinHTTP is not available) it falls back to
 * QgsNetworkAccessManager::blockingGet, which has no built-in SSO support.
 *
 * The Referer header is set from the URL's `referer` query parameter when
 * present, matching the behavior expected by ESRI's `generateToken?client=referer`.
 *
 * \param url        Token URL to fetch
 * \param timeoutMs  Maximum time to wait for the response, in milliseconds
 */
KadasPortalTokenFetchResult kadasFetchPortalTokenWithSso( const QUrl &url, int timeoutMs );

#endif // KADASPORTALTOKENFETCHER_H
