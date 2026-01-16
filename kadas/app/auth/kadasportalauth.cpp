/***************************************************************************
  kadasportalauth.cpp
  -------------------
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>

#include <qgis/qgsapplication.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsauthmethod.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgssettingstreenode.h>

#include "kadasportalauth.h"


const QgsSettingsEntryString *KadasPortalAuth::settingsPortalTokenUrl = new QgsSettingsEntryString( QStringLiteral( "token-url" ), KadasSettingsTree::sTreePortal, QString(), QStringLiteral( "URL to retrieve ESRI portal TOKEN from." ) );
const QgsSettingsEntryStringList *KadasPortalAuth::settingsPortalCookieUrls = new QgsSettingsEntryStringList( QStringLiteral( "cookie-urls" ), KadasSettingsTree::sTreePortal, {}, QStringLiteral( "URLs for which the ERSI portal TOKEN will be set in a cookie." ) );
const QgsSettingsEntryBool *KadasPortalAuth::settingsTokenCreateCookies = new QgsSettingsEntryBool( QStringLiteral( "token-create-cookies" ), KadasSettingsTree::sTreePortal, true, QStringLiteral( "Create cookies using the ESRI token." ) );
const QgsSettingsEntryBool *KadasPortalAuth::settingsTokenUseEsriAuth = new QgsSettingsEntryBool( QStringLiteral( "token-use-esri-auth" ), KadasSettingsTree::sTreePortal, true, QStringLiteral( "Create cookies using the ESRI token." ) );

const QgsSettingsEntryBool *KadasPortalAuth::settingsOAuth2Enabled = new QgsSettingsEntryBool( QStringLiteral( "enabled" ), KadasPortalAuth::sTreePortalOAuth2, false, QStringLiteral( "If enabled use OAuth2 authentication." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2RequestUrl = new QgsSettingsEntryString( QStringLiteral( "request-url" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Request URL." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2TokenUrl = new QgsSettingsEntryString( QStringLiteral( "token-url" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Token URL." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientId = new QgsSettingsEntryString( QStringLiteral( "client-id" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Client ID." ) );
const QgsSettingsEntryString *KadasPortalAuth::settingsOAuth2ClientSecret = new QgsSettingsEntryString( QStringLiteral( "client-secret" ), KadasPortalAuth::sTreePortalOAuth2, QString(), QStringLiteral( "Client Secret." ) );

const QString KadasPortalAuth::ESRI_AUTH_CFG_ID = QStringLiteral( "kadas_esri_token" );

KadasPortalAuth::KadasPortalAuth( QObject *parent )
  : QObject { parent }
{
}

void KadasPortalAuth::setupAuthentication()
{
  const QString tokenUrl = settingsPortalTokenUrl->value();
  if ( settingsOAuth2Enabled->value() )
  {
    // Authentication via OAuth2
    createOAuth2Auth( settingsOAuth2RequestUrl->value(), settingsOAuth2TokenUrl->value(), settingsOAuth2ClientId->value(), settingsOAuth2ClientSecret->value() );
  }
  else if ( !tokenUrl.isEmpty() )
  {
    // Authentication via token
    QgsDebugMsgLevel( QStringLiteral( "Extracting portal TOKEN from %1" ).arg( tokenUrl ), 1 );

    QNetworkRequest req = QNetworkRequest( QUrl( tokenUrl ) );
    QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( req );
    QString token;
    if ( content.error() == QNetworkReply::NoError )
    {
      QJsonParseError err;
      QJsonDocument doc = QJsonDocument::fromJson( content.content(), &err );
      if ( !doc.isNull() )
      {
        QJsonObject obj = doc.object();
        if ( obj.contains( QStringLiteral( "token" ) ) )
        {
          token = obj[QStringLiteral( "token" )].toString();
          QgsDebugMsgLevel( QString( "ESRI Token found" ), 1 );
          if ( settingsTokenCreateCookies->value() )
          {
            // If we create the cookies directly,
            // it does not work in the same event loop
            // so we need to delay it a bit
            QTimer::singleShot( 1, this, [=]() {
              createCookies( token );
            } );
          }
          if ( settingsTokenUseEsriAuth->value() )
            createEsriAuth( token );
        }
      }
      else
      {
        QgsDebugMsgLevel( QString( "could not read TOKEN from response: %1" ).arg( err.errorString() ), 1 );
      }
    }
    else
    {
      QgsDebugMsgLevel( QString( "error fetching token: %1" ).arg( content.errorString() ), 1 );
    }
  }
  else
  {
    QgsDebugMsgLevel( QString( "No TOKEN url or OAuth2 configured for portal" ), 1 );
  }
}

void KadasPortalAuth::createCookies( const QString &token )
{
  QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
  const QStringList cookieUrls = settingsPortalCookieUrls->value();
  for ( const QString &url : cookieUrls )
  {
    QgsDebugMsgLevel( QString( "Setting cookie for url %1" ).arg( url ), 1 );
    QNetworkCookie cookie = QNetworkCookie( QByteArray( "agstoken" ), token.toLocal8Bit() );
    jar->setCookiesFromUrl( QList<QNetworkCookie>() << cookie, url.trimmed() );
  }
}

void KadasPortalAuth::createEsriAuth( const QString &token )
{
  if ( !QgsApplication::authManager()->masterPasswordHashInDatabase() && QgsApplication::authManager()->passwordHelperEnabled() )
  {
    // if no master password set by user yet, just generate a new one and store it in the system keychain
    QgsApplication::authManager()->createAndStoreRandomMasterPasswordInKeyChain();
  }

  // Create or update an EsriToken authentication configuration in QgsAuthManager
  QgsAuthMethodConfig config;
  config.setId( ESRI_AUTH_CFG_ID );
  config.setName( QStringLiteral( "kadas_esri_token" ) );
  config.setMethod( QStringLiteral( "EsriToken" ) );
  config.setConfig( QStringLiteral( "token" ), token );

  if ( QgsApplication::authManager()->storeAuthenticationConfig( config, true /* overwrite */ ) )
  {
    QgsDebugMsgLevel( QString( "Created EsriToken auth config with id %1" ).arg( ESRI_AUTH_CFG_ID ), 1 );
  }
  else
  {
    QgsDebugMsgLevel( QString( "Failed to create EsriToken auth config with id %1" ).arg( ESRI_AUTH_CFG_ID ), 1 );
  }
}

void KadasPortalAuth::createOAuth2Auth( const QString &requestUrl, const QString &tokenUrl, const QString &clientId, const QString &clientSecret )
{
  if ( !QgsApplication::authManager()->masterPasswordHashInDatabase() && QgsApplication::authManager()->passwordHelperEnabled() )
  {
    // if no master password set by user yet, just generate a new one and store it in the system keychain
    QgsApplication::authManager()->createAndStoreRandomMasterPasswordInKeyChain();
  }

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

  // Create or update an OAuth2 authentication configuration in QgsAuthManager
  QgsAuthMethodConfig config;
  config.setId( ESRI_AUTH_CFG_ID );
  config.setName( QStringLiteral( "kadas_oauth2" ) );
  config.setMethod( QStringLiteral( "OAuth2" ) );
  config.setConfig( QStringLiteral( "oauth2config" ), oauth2ConfigJson );

  if ( QgsApplication::authManager()->storeAuthenticationConfig( config, true /* overwrite */ ) )
  {
    QgsDebugMsgLevel( QString( "Created OAuth2 auth config with id %1" ).arg( ESRI_AUTH_CFG_ID ), 1 );
  }
  else
  {
    QgsDebugMsgLevel( QString( "Failed to create OAuth2 auth config with id %1" ).arg( ESRI_AUTH_CFG_ID ), 1 );
  }
}
