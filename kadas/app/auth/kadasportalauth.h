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

#include "kadas/core/kadassettingstree.h"

class QgsSettingsEntryBool;
class QgsSettingsEntryString;
class QgsSettingsEntryStringList;
class QgsSettingsTreeNode;
class KadasAppAuthRequestHandler;

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
    static const QgsSettingsEntryString *settingsOAuth2ClientSecret;

    static const QString ESRI_AUTH_CFG_ID;

    explicit KadasPortalAuth( QObject *parent = nullptr );

    void setupAuthentication();

  private slots:
    void authRequestHandlerBrowserOpened();
    void authRequestHandlerBrowserClosed();

  private:
    void createCookies( const QString &token );
    void createEsriAuth( const QString &token );
    void createOAuth2Auth( const QString &requestUrl, const QString &tokenUrl, const QString &clientId, const QString &clientSecret );

    QMessageBox mRequestRunningMessageBox;
    KadasAppAuthRequestHandler *mAppAuthRequestHandler;
};

#endif // KADASPORTALAUTH_H
