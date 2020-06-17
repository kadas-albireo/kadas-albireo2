/***************************************************************************
    kadasiamauth.h
    --------------
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

#ifndef KADASIAMAUTH_H
#define KADASIAMAUTH_H

#include <QObject>

class QToolButton;
class StackedDialog;
struct IDispatch;


class KadasIamAuth : public QObject
{
    Q_OBJECT
  public:
    KadasIamAuth( QToolButton *loginButton, QToolButton *logoutButton, QToolButton *refreshButton, QObject *parent );
    ~KadasIamAuth();

  private:
    QToolButton *mLoginButton = nullptr;
    QToolButton *mLogoutButton = nullptr;
    QToolButton *mRefreshButton = nullptr;
    StackedDialog *mLoginDialog = nullptr;
    QString mPreprocessorId;

  private slots:
    void performLogin();
    void performLogout();
    void checkLoginComplete( QString );
    void checkLogoutComplete();
    void handleNewWindow( IDispatch **ppDisp, bool &cancel, uint dwFlags, QString bstrUrlContext, QString bstrUrl );
    void handleWindowClose( bool isChild, bool &cancel );
};

#endif // KADASIAMAUTH_H
