/***************************************************************************
  kadasportalauth.h
  -----------------
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

#ifndef KADASPORTALAUTH_H
#define KADASPORTALAUTH_H

#include <QObject>
#include <QMessageBox>
#include <QUrl>

#include "kadas/core/kadassettingstree.h"

class QgsAuthMethodConfig;
class QgsSettingsEntryBool;
class QgsSettingsEntryString;
class QgsSettingsEntryStringList;
class QgsSettingsTreeNode;
class KadasAppAuthRequestHandler;
class QNetworkRequest;

class KadasPortalAuth : public QObject
{
    Q_OBJECT
  public:
    static const QgsSettingsEntryString *settingsPortalTokenUrl;
    static const QgsSettingsEntryBool *settingsTokenCreateCookies;
    static const QgsSettingsEntryBool *settingsTokenUseEsriAuth;
    static const QgsSettingsEntryStringList *settingsPortalCookieUrls;

    static inline QgsSettingsTreeNode *sTreePortalOAuth2 = KadasSettingsTree::sTreePortal->createChildNode( QStringLiteral( "OAuth2" ) );
    static const QgsSettingsEntryBool *settingsOAuth2Enabled;
    static const QgsSettingsEntryString *settingsOAuth2RequestUrl;
    static const QgsSettingsEntryString *settingsOAuth2TokenUrl;
    static const QgsSettingsEntryString *settingsOAuth2ClientId;
    static const QgsSettingsEntryString *settingsOAuth2ClientIdUrl;
    static const QgsSettingsEntryString *settingsOAuth2ClientIdJsonPath;
    static const QgsSettingsEntryString *settingsOAuth2ClientSecret;

    static const QString ESRI_AUTH_CFG_ID;

    explicit KadasPortalAuth( QObject *parent = nullptr );

    //! Creates the authentication configuration. Does not run the interactive OAuth2 login, see login()
    void setupAuthentication();

    /**
     * Runs the OAuth2 login if OAuth2 is configured, and blocks until it is finished.
     */
    void login();

    /**
     * Adds the portal authentication to \a request, if an auth config has been set up in this session
     * (OAuth2 or ESRI token) and has not been removed because the user continued without authentication.
     * The token is only added to https requests targeting the host and port of the portal, i.e. of the
     * OAuth2 token URL or of the ESRI token URL. Other requests are left untouched.
     * Returns TRUE if the request has been authenticated.
     */
    bool authenticateRequest( QNetworkRequest &request ) const;

  private slots:
    void authRequestHandlerBrowserOpened();
    void authRequestHandlerBrowserClosed();

  private:
    void setupAuthenticationOAuth2();
    void setupAuthenticationEsriToken();
    void createCookies( const QString &token );

    //! Stores \a config with id ESRI_AUTH_CFG_ID, overwriting the existing one. Creates the master password if needed.
    bool storeAuthConfig( QgsAuthMethodConfig &config );

    //! Removes the auth config with id ESRI_AUTH_CFG_ID, if it exists
    void removeAuthConfig();

    //! Stores an EsriToken auth config. With an empty \a token requests using it are sent unauthenticated.
    bool storeEsriTokenAuth( const QString &token );
    bool createOAuth2Auth( const QString &requestUrl, const QString &tokenUrl, const QString &clientId, const QString &clientSecret );

    /**
     * Runs the OAuth2 login (interactive if there is no valid cached token) and blocks until it is finished.
     * On failure the user can retry or continue without authentication, in which case the auth config is removed.
     * Returns TRUE if the login succeeded.
     */
    bool loginOAuth2();

    //! Checks that the portal accepts the OAuth2 token and waits a bit if it is not accepted yet
    void verifyOAuth2Token();
    QString retrieveOAuthClientId( const QString &clientIdUrl, const QString &clientIdJsonPath );

    QMessageBox mRequestRunningMessageBox;
    KadasAppAuthRequestHandler *mAppAuthRequestHandler;
    //! TRUE if the auth config has been set up with a token (ESRI token) or for OAuth2 in this session
    bool mAuthConfigAvailable = false;
    //! TRUE if the browser has been opened for the interactive login during login()
    bool mBrowserOpenedDuringLogin = false;
    //! URL of the portal the auth config belongs to, requests to other hosts are not authenticated
    QUrl mPortalUrl;
};

#endif // KADASPORTALAUTH_H
