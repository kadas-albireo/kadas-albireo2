/***************************************************************************
  kadasportalauth.cpp
  -------------------
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

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPushButton>
#include <QTimer>
#include <QUrlQuery>

#include <qgis/qgsapplication.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsauthmethod.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgssettingstreenode.h>

#include "kadas/core/kadaslogger.h"
#include "kadasportalauth.h"

#include "kadas/app/kadasapplication.h"
#include "kadas/app/auth/kadasappauthrequesthandler.h"
#include "kadas/core/kadasjsonutils.h"


const QgsSettingsEntryString *KadasPortalAuth::settingsPortalTokenUrl
  = new QgsSettingsEntryString( QStringLiteral( "token-url" ), KadasSettingsTree::sTreePortal, QString(), QStringLiteral( "URL to retrieve ESRI portal TOKEN from." ) );
const QgsSettingsEntryStringList *KadasPortalAuth::settingsPortalCookieUrls
  = new QgsSettingsEntryStringList( QStringLiteral( "cookie-urls" ), KadasSettingsTree::sTreePortal, {}, QStringLiteral( "URLs for which the ERSI portal TOKEN will be set in a cookie." ) );
const QgsSettingsEntryBool *KadasPortalAuth::settingsTokenCreateCookies
  = new QgsSettingsEntryBool( QStringLiteral( "token-create-cookies" ), KadasSettingsTree::sTreePortal, true, QStringLiteral( "Create cookies using the ESRI token." ) );
const QgsSettingsEntryBool *KadasPortalAuth::settingsTokenUseEsriAuth
  = new QgsSettingsEntryBool( QStringLiteral( "token-use-esri-auth" ), KadasSettingsTree::sTreePortal, true, QStringLiteral( "Create cookies using the ESRI token." ) );

const QgsSettingsEntryBool *KadasPortalAuth::settingsOAuth2Enabled
  = new QgsSettingsEntryBool( QStringLiteral( "enabled" ), KadasPortalAuth::sTreePortalOAuth2, false, QStringLiteral( "If enabled use OAuth2 authentication." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2RequestUrl
  = new QgsSettingsEntryString( QStringLiteral( "request-url" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Request URL." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2TokenUrl
  = new QgsSettingsEntryString( QStringLiteral( "token-url" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Token URL." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientId
  = new QgsSettingsEntryString( QStringLiteral( "client-id" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Client ID." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientIdUrl
  = new QgsSettingsEntryString( QStringLiteral( "client-id-url" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "URL to retrieve ESRI portal TOKEN from. Used only if client-id settings is empty." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientIdJsonPath
  = new QgsSettingsEntryString( QStringLiteral( "client-id-json-path" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "JSON-Path to find the ClientId inside the file returned by client-id-url. Example: $.features[0].attributes.client_id" ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientSecret
  = new QgsSettingsEntryString( QStringLiteral( "client-secret" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Client Secret." ) );

const QString KadasPortalAuth::ESRI_AUTH_CFG_ID = QStringLiteral( "kadas_esri_token" );

KadasPortalAuth::KadasPortalAuth( QObject *parent )
  : QObject { parent }
  , mRequestRunningMessageBox(
      QMessageBox::Icon::Information,
      tr( "OAuth2 Authentication" ),
      tr(
        "Your browser has been opened for authentication.\n\n"
        "Please complete the authentication in your browser.\n"
        "This dialog will close automatically when authentication is complete."
      ),
      QMessageBox::StandardButton::Cancel
    )
{
  mRequestRunningMessageBox.setModal( true );

  // Keep raw pointer before transferring ownership
  auto handler = std::make_unique<KadasAppAuthRequestHandler>( this );
  mAppAuthRequestHandler = handler.get();
  QgsNetworkAccessManager::instance()->setAuthHandler( std::move( handler ) );

  connect( mAppAuthRequestHandler, &KadasAppAuthRequestHandler::browserOpened, this, &KadasPortalAuth::authRequestHandlerBrowserOpened );
  connect( mAppAuthRequestHandler, &KadasAppAuthRequestHandler::browserClosed, this, &KadasPortalAuth::authRequestHandlerBrowserClosed );
  connect( &mRequestRunningMessageBox, &QMessageBox::rejected, mAppAuthRequestHandler, &KadasAppAuthRequestHandler::abortAuth, Qt::QueuedConnection );
}

void KadasPortalAuth::setupAuthentication()
{
  mAuthConfigAvailable = false;
  mPortalUrl.clear();

  if ( !settingsOAuth2Enabled->value() && settingsPortalTokenUrl->value().isEmpty() )
  {
    KadasDebugMsgLevel( QString( "No TOKEN url or OAuth2 configured for portal" ), 1 );
    removeAuthConfig();
    return;
  }

  // Replace the auth config of the previous session by one without token. It is overwritten
  // below on success, so every failure path ends with a config that leaves the requests of the
  // catalog and the layers unauthenticated instead of starting the interactive login.
  if ( !storeEsriTokenAuth( QString() ) )
  {
    removeAuthConfig();
  }

  if ( settingsOAuth2Enabled->value() )
  {
    setupAuthenticationOAuth2();
  }
  else
  {
    setupAuthenticationEsriToken();
  }
}

void KadasPortalAuth::authRequestHandlerBrowserOpened()
{
  mRequestRunningMessageBox.show();
}

void KadasPortalAuth::authRequestHandlerBrowserClosed()
{
  mRequestRunningMessageBox.hide();
}

void KadasPortalAuth::setupAuthenticationOAuth2()
{
  QString clientId = settingsOAuth2ClientId->value();

  // ClientId retrival by URL
  if ( clientId.isEmpty() )
  {
    clientId = retrieveOAuthClientId( settingsOAuth2ClientIdUrl->value(), settingsOAuth2ClientIdJsonPath->value() );

    if ( clientId.isEmpty() )
      return;
  }

  // Authentication via OAuth2
  if ( createOAuth2Auth( settingsOAuth2RequestUrl->value(), settingsOAuth2TokenUrl->value(), clientId, settingsOAuth2ClientSecret->value() ) )
  {
    mAuthConfigAvailable = true;
    mPortalUrl = QUrl( settingsOAuth2TokenUrl->value() );
  }
}

bool KadasPortalAuth::authenticateRequest( QNetworkRequest &request ) const
{
  if ( !mAuthConfigAvailable )
  {
    return false;
  }

  // Only send the portal token to the portal itself, and never unencrypted
  const QUrl url = request.url();
  if ( mPortalUrl.host().isEmpty() || url.scheme() != QLatin1String( "https" ) || url.host().compare( mPortalUrl.host(), Qt::CaseInsensitive ) != 0 || url.port( 443 ) != mPortalUrl.port( 443 ) )
  {
    return false;
  }

  if ( !QgsApplication::authManager()->updateNetworkRequest( request, ESRI_AUTH_CFG_ID ) )
  {
    KadasDebugError( QString( "Failed to authenticate request with auth config %1: %2" ).arg( ESRI_AUTH_CFG_ID, request.url().toString( QUrl::RemoveQuery ) ) );
    return false;
  }
  return true;
}

void KadasPortalAuth::login()
{
  if ( !settingsOAuth2Enabled->value() || !mAuthConfigAvailable )
  {
    return;
  }

  // Login now, so that the catalog and the layers find a valid token instead of
  // each triggering the interactive login while loading
  if ( loginOAuth2() )
  {
    verifyOAuth2Token();
  }
}

bool KadasPortalAuth::loginOAuth2()
{
  while ( true )
  {
    // Nothing is sent: updateNetworkRequest only needs a request to add the token to.
    // Without a valid cached token it runs the interactive login and blocks until it is finished.
    QNetworkRequest request( ( QUrl( settingsOAuth2TokenUrl->value() ) ) );
    if ( QgsApplication::authManager()->updateNetworkRequest( request, ESRI_AUTH_CFG_ID ) )
    {
      KadasDebugMsgLevel( QString( "OAuth2 login succeeded for auth config %1" ).arg( ESRI_AUTH_CFG_ID ), 1 );
      return true;
    }

    KadasDebugError( QString( "OAuth2 login failed or was canceled for auth config %1" ).arg( ESRI_AUTH_CFG_ID ) );

    // On timeout the browser close request is never sent
    mRequestRunningMessageBox.hide();

    QMessageBox
      messageBox( QMessageBox::Icon::Warning, tr( "OAuth2 Authentication" ), tr( "The authentication failed or was canceled." ), QMessageBox::StandardButton::Retry | QMessageBox::StandardButton::Ignore );
    messageBox.button( QMessageBox::StandardButton::Ignore )->setText( tr( "Continue without authentication" ) );
    messageBox.setDefaultButton( QMessageBox::StandardButton::Retry );
    if ( messageBox.exec() != QMessageBox::StandardButton::Retry )
    {
      break;
    }
  }

  // Replace the OAuth2 config by one without token, otherwise every request using it (catalog, layers)
  // would start the interactive login again. The OAuth2 config is created again at the next start.
  if ( !storeEsriTokenAuth( QString() ) )
  {
    removeAuthConfig();
  }
  mAuthConfigAvailable = false;
  KadasDebugError( QString( "Continuing without authentication, auth config %1 reset" ).arg( ESRI_AUTH_CFG_ID ) );
  return false;
}

void KadasPortalAuth::verifyOAuth2Token()
{
  // A freshly issued token might not be accepted immediately by the portal. Check it once
  // before the catalog is loaded and wait a bit if it is rejected.
  // The portal REST endpoint is derived from the OAuth2 URLs (<portal>/sharing/rest/oauth2/...).
  QString restUrl;
  for ( const QString &url : { settingsOAuth2RequestUrl->value(), settingsOAuth2TokenUrl->value() } )
  {
    const int pos = url.indexOf( QStringLiteral( "/sharing/rest/" ), 0, Qt::CaseInsensitive );
    if ( pos >= 0 )
    {
      restUrl = url.left( pos ) + QStringLiteral( "/sharing/rest" );
      break;
    }
  }
  if ( restUrl.isEmpty() )
  {
    KadasDebugMsgLevel( QString( "OAuth2 token verification skipped: no portal REST URL found in the OAuth2 URLs" ), 1 );
    return;
  }

  const QList<int> retryDelaysMs = { 1000, 2000, 4000 };
  for ( int attempt = 0;; ++attempt )
  {
    QNetworkRequest request( QUrl( restUrl + QStringLiteral( "/community/self?f=json" ) ) );
    const QgsNetworkReplyContent content = QgsNetworkAccessManager::blockingGet( request, ESRI_AUTH_CFG_ID );
    const int httpStatus = content.attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    const QJsonObject response = QJsonDocument::fromJson( content.content() ).object();

    if ( content.error() == QNetworkReply::NoError && response.contains( QStringLiteral( "username" ) ) )
    {
      KadasDebugMsgLevel( QString( "OAuth2 token accepted by the portal, user %1" ).arg( response.value( QStringLiteral( "username" ) ).toString() ), 1 );
      return;
    }

    // ArcGIS usually returns errors with HTTP status 200 and an error object
    const QJsonObject error = response.value( QStringLiteral( "error" ) ).toObject();
    const int errorCode = error.isEmpty() ? httpStatus : error.value( QStringLiteral( "code" ) ).toInt();
    const QString errorMessage = error.isEmpty() ? content.errorString() : error.value( QStringLiteral( "message" ) ).toString();

    // Only retry for errors meaning that the token was not accepted (yet)
    const bool tokenRejected = errorCode == 401 || errorCode == 403 || errorCode == 498 || errorCode == 499;
    if ( !tokenRejected || attempt >= retryDelaysMs.size() )
    {
      KadasDebugError( QString( "OAuth2 token verification failed (HTTP status %1, error %2: %3)" ).arg( httpStatus ).arg( errorCode ).arg( errorMessage ) );
      return;
    }

    KadasDebugError( QString( "OAuth2 token not accepted by the portal yet (error %1: %2), retrying in %3 ms" ).arg( errorCode ).arg( errorMessage ).arg( retryDelaysMs[attempt] ) );
    QEventLoop loop;
    QTimer::singleShot( retryDelaysMs[attempt], &loop, &QEventLoop::quit );
    loop.exec();
  }
}

void KadasPortalAuth::setupAuthenticationEsriToken()
{
  const QString tokenUrl = settingsPortalTokenUrl->value();

  // Authentication via token. On Windows, Qt's QNetworkAccessManager handles
  // Negotiate/NTLM transparently via SSPI when speaking HTTP/1.1 (HTTP/2 is
  // globally disabled via KadasApplication::settingsDisableHttp2 to work
  // around QTBUG-146829), so the logged-in Windows user's credentials are
  // used silently without prompting.
  KadasDebugMsgLevel( QStringLiteral( "Extracting portal TOKEN from %1" ).arg( tokenUrl ), 1 );

  QNetworkRequest req = QNetworkRequest( QUrl( tokenUrl ) );
  QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( req );

  if ( content.error() != QNetworkReply::NoError )
  {
    KadasDebugMsgLevel( QString( "error fetching token: %1" ).arg( content.errorString() ), 1 );
    return;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson( content.content(), &err );
  if ( doc.isNull() )
  {
    KadasDebugMsgLevel( QString( "Could not parse JSON from response: %1" ).arg( err.errorString() ), 1 );
    return;
  }

  QJsonObject obj = doc.object();
  if ( !obj.contains( QStringLiteral( "token" ) ) )
  {
    KadasDebugMsgLevel( QString( "Could not read TOKEN from response: %1" ).arg( err.errorString() ), 1 );
    return;
  }

  QString token = obj[QStringLiteral( "token" )].toString();
  KadasDebugMsgLevel( QString( "ESRI Token found" ), 1 );
  if ( settingsTokenCreateCookies->value() )
  {
    // Populate the cookie jar synchronously: the catalog browser starts
    // issuing requests as soon as the main window is constructed, and a
    // deferred singleShot would let those first requests go out without
    // the agstoken cookie.
    createCookies( token );
  }
  if ( settingsTokenUseEsriAuth->value() && storeEsriTokenAuth( token ) )
  {
    mAuthConfigAvailable = true;
    mPortalUrl = QUrl( settingsPortalTokenUrl->value() );
  }
}

void KadasPortalAuth::createCookies( const QString &token )
{
  QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
  const QStringList cookieUrls = settingsPortalCookieUrls->value();
  for ( const QString &url : cookieUrls )
  {
    KadasDebugMsgLevel( QString( "Setting cookie for url %1" ).arg( url ), 1 );
    QNetworkCookie cookie = QNetworkCookie( QByteArray( "agstoken" ), token.toLocal8Bit() );
    jar->setCookiesFromUrl( QList<QNetworkCookie>() << cookie, url.trimmed() );
  }
}

bool KadasPortalAuth::storeAuthConfig( QgsAuthMethodConfig &config )
{
  QgsAuthManager *authManager = QgsApplication::authManager();
  if ( !authManager->masterPasswordHashInDatabase() && authManager->passwordHelperEnabled() )
  {
    // if no master password set by user yet, just generate a new one and store it in the system keychain
    authManager->createAndStoreRandomMasterPasswordInKeyChain();
  }

  // Create or update the authentication configuration in QgsAuthManager
  config.setId( ESRI_AUTH_CFG_ID );
  if ( authManager->storeAuthenticationConfig( config, true /* overwrite */ ) )
  {
    KadasDebugMsgLevel( QString( "Created %1 auth config with id %2" ).arg( config.method(), ESRI_AUTH_CFG_ID ), 1 );
    return true;
  }

  KadasDebugError( QString( "Failed to create %1 auth config with id %2" ).arg( config.method(), ESRI_AUTH_CFG_ID ) );
  return false;
}

void KadasPortalAuth::removeAuthConfig()
{
  QgsAuthManager *authManager = QgsApplication::authManager();
  if ( authManager->configIds().contains( ESRI_AUTH_CFG_ID ) )
  {
    authManager->removeAuthenticationConfig( ESRI_AUTH_CFG_ID );
  }
}

bool KadasPortalAuth::storeEsriTokenAuth( const QString &token )
{
  QgsAuthMethodConfig config;
  config.setName( QStringLiteral( "kadas_esri_token" ) );
  config.setMethod( QStringLiteral( "EsriToken" ) );
  config.setConfig( QStringLiteral( "token" ), token );
  return storeAuthConfig( config );
}

bool KadasPortalAuth::createOAuth2Auth( const QString &requestUrl, const QString &tokenUrl, const QString &clientId, const QString &clientSecret )
{
  // Create OAuth2 configuration JSON
  // Note: Using manual JSON approach because qgsauthoauth2config.h is not exported in QGIS public API
  QVariantMap oauth2Config;
  oauth2Config[QStringLiteral( "grantFlow" )] = 0; // AuthCode
  oauth2Config[QStringLiteral( "requestUrl" )] = requestUrl;
  oauth2Config[QStringLiteral( "tokenUrl" )] = tokenUrl;
  oauth2Config[QStringLiteral( "clientId" )] = clientId;
  oauth2Config[QStringLiteral( "clientSecret" )] = clientSecret;

  QJsonDocument doc = QJsonDocument::fromVariant( oauth2Config );
  QString oauth2ConfigJson = QString::fromUtf8( doc.toJson( QJsonDocument::Compact ) );

  QgsAuthMethodConfig config;
  config.setName( QStringLiteral( "kadas_oauth2" ) );
  config.setMethod( QStringLiteral( "OAuth2" ) );
  config.setConfig( QStringLiteral( "oauth2config" ), oauth2ConfigJson );
  return storeAuthConfig( config );
}

QString KadasPortalAuth::retrieveOAuthClientId( const QString &clientIdUrl, const QString &clientIdJsonPath )
{
  KadasDebugMsgLevel( QStringLiteral( "Extracting OAuth2 ClientId from %1" ).arg( clientIdUrl ), 1 );

  QNetworkRequest req = QNetworkRequest( QUrl( clientIdUrl ) );
  QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( req );

  if ( content.error() != QNetworkReply::NoError )
  {
    KadasDebugMsgLevel( QString( "Error fetching token: %1" ).arg( content.errorString() ), 1 );
    return QString();
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson( content.content(), &err );
  if ( doc.isNull() )
  {
    KadasDebugMsgLevel( QString( "Could not parse JSON from response: %1" ).arg( err.errorString() ), 1 );
    return QString();
  }

  // Extract ClientId from JSON
  const QString clientId = KadasJsonUtils::jsonPathToString( doc, clientIdJsonPath );
  if ( clientId.isEmpty() )
  {
    KadasDebugMsgLevel( QStringLiteral( "Could not extract ClientId using JSON path %1" ).arg( clientIdJsonPath ), 1 );
    return QString();
  }

  return clientId;
}
