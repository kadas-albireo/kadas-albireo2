/***************************************************************************
    kadasiamauth.cpp
    ----------------
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

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>

#include <QAxObject>
#include <QAxWidget>
#endif
#include <QDialog>
#include <QNetworkRequest>
#include <QStackedLayout>
#include <QToolButton>
#include <QUrlQuery>

#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/iamauth/kadasiamauth.h>


class StackedDialog : public QDialog
{
  public:
    StackedDialog( QWidget *parent = 0, Qt::WindowFlags flags = 0 ) : QDialog( parent, flags )
    {
      mLayout = new QStackedLayout();
      setLayout( mLayout );
    }
    void pushWidget( QWidget *widget )
    {
      mLayout->setCurrentIndex( mLayout->addWidget( widget ) );
    }
    void popWidget( QWidget *widget )
    {
      if ( mLayout->currentWidget() == widget )
      {
        mLayout->setCurrentIndex( mLayout->currentIndex() - 1 );
      }
      mLayout->removeWidget( widget );
      widget->deleteLater();
    }

  private:
    QStackedLayout *mLayout = nullptr;
};

#ifdef Q_OS_WIN

class WebWidget : public QAxWidget
{
  public:
    WebWidget( QWidget *parent = 0, Qt::WindowFlags f = 0 )
      : QAxWidget( parent, f )
    {
      setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
    }

    void navigate( const QString &location )
    {
      dynamicCall( "Navigate(const QString&)", location );
    }
    QString location()
    {
      return dynamicCall( "LocationURL()" ).toString();
    }
    QStringList cookies()
    {
      QAxObject *document = querySubObject( "Document()" );
      return document->property( "cookie" ).toString().split( QRegExp( "\\s*;\\s*" ) );
    }
  protected:
    virtual bool translateKeyEvent( int message, int keycode ) const
    {
      if ( message >= WM_KEYFIRST && message <= WM_KEYLAST )
        return true;
      else
        return QAxWidget::translateKeyEvent( message, keycode );
    }
};

#else

class WebWidget : public QWidget
{
  public:
    void navigate( const QString &location ) {}
    QString location() { return QString(); }
    QStringList cookies() { return QStringList(); }
};

#endif

KadasIamAuth::KadasIamAuth( QToolButton *loginButton, QToolButton *logoutButton, QToolButton *refreshButton, QObject *parent )
  : QObject( parent ), mLoginButton( loginButton ), mLogoutButton( logoutButton ), mRefreshButton( refreshButton )
{
  mLogoutButton->hide();
#ifdef Q_OS_WIN
  if ( QgsSettings().value( "/iamauth/loginurl", "" ).toString().isEmpty() )
#else
  if ( true )
#endif
  {
    mLoginButton->hide();
  }
  else
  {
    connect( mLoginButton, &QToolButton::clicked, this, &KadasIamAuth::performLogin );
    connect( mLogoutButton, &QToolButton::clicked, this, &KadasIamAuth::performLogout );
  }
}

KadasIamAuth::~KadasIamAuth()
{
  QgsNetworkAccessManager::removeRequestPreprocessor( mPreprocessorId );
}

void KadasIamAuth::performLogin()
{
#ifdef Q_OS_WIN
  mLoginDialog = new StackedDialog( kApp->mainWindow() );
  mLoginDialog->setWindowTitle( tr( "Authentication" ) );
  mLoginDialog->resize( 1000, 480 );

  WebWidget *webWidget = new WebWidget();
  webWidget->navigate( QgsSettings().value( "/iamauth/loginurl", "" ).toString() );
  connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
  connect( webWidget, SIGNAL( NewWindow3( IDispatch **, bool &, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch **, bool &, uint, QString, QString ) ) );
  connect( webWidget, SIGNAL( WindowClosing( bool, bool & ) ), this, SLOT( handleWindowClose( bool, bool & ) ) );
  mLoginDialog->pushWidget( webWidget );
  mLoginDialog->exec();
#endif
}

void KadasIamAuth::checkLoginComplete( QString /*addr*/ )
{
  if ( mLoginDialog )
  {
    WebWidget *webWidget = static_cast<WebWidget *>( QObject::sender() );
    QUrl url( webWidget->location() );
    QUrl baseUrl;
    baseUrl.setScheme( url.scheme() );
    baseUrl.setHost( url.host() );
    QStringList cookies = webWidget->cookies();
    for ( const QString &cookie : cookies )
    {
      QStringList pair = cookie.split( "=" );
      if ( !pair.isEmpty() && pair.first() == "esri_auth" )
      {
//        QMessageBox::information( mLoginDialog, tr( "Authentication successful" ), QString( "Url: %1\nCookie: %2" ).arg( baseUrl.toString() ).arg( cookie ) );
        kApp->mainWindow()->messageBar()->pushMessage( tr( "Authentication successful" ), Qgis::Info, 5 );
        mLoginDialog->accept();
        mLoginDialog->deleteLater();
        mLoginDialog = nullptr;
        QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
        QStringList cookieUrls = QgsSettings().value( "/iamauth/cookieurls", "" ).toString().split( ";" );
        for ( const QString &url : cookieUrls )
        {
          jar->setCookiesFromUrl( QList<QNetworkCookie>() << QNetworkCookie( cookie.toLocal8Bit() ), url );
        }
        mLoginButton->hide();
        mLogoutButton->show();
        mRefreshButton->click();
      }
    }
  }
}

void KadasIamAuth::handleNewWindow( IDispatch **ppDisp, bool & /*cancel*/, uint /*dwFlags*/, QString /*bstrUrlContext*/, QString /*bstrUrl*/ )
{
#ifdef Q_OS_WIN
  if ( mLoginDialog )
  {
    WebWidget *webWidget = new WebWidget;
    connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
    connect( webWidget, SIGNAL( NewWindow3( IDispatch **, bool &, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch **, bool &, uint, QString, QString ) ) );
    connect( webWidget, SIGNAL( WindowClosing( bool, bool & ) ), this, SLOT( handleWindowClose( bool, bool & ) ) );
    IDispatch *appDisp;
    QAxObject *appDispObj = webWidget->querySubObject( "Application()" );
    appDispObj->queryInterface( IID_IDispatch, ( void ** )&appDisp );
    *ppDisp = appDisp;
    mLoginDialog->pushWidget( webWidget );
  }
#endif
}

void KadasIamAuth::handleWindowClose( bool /*isChild*/, bool & /*cancel*/ )
{
  if ( mLoginDialog )
  {
    mLoginDialog->popWidget( qobject_cast<QWidget *>( QObject::sender() ) );
  }
}

void KadasIamAuth::performLogout()
{
  QString url = QgsSettings().value( "/iamauth/logouturl", "" ).toString();
  if ( url.isEmpty() )
  {
    return;
  }
  QNetworkRequest req;
  req.setUrl( QUrl( url ) );
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
  connect( reply, &QNetworkReply::finished, this, &KadasIamAuth::checkLogoutComplete );
}

void KadasIamAuth::checkLogoutComplete()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *> ( QObject::sender() );
  if ( reply->error() == QNetworkReply::NoError )
  {
    QNetworkCookieJar *jar = QgsNetworkAccessManager::instance()->cookieJar();
    QStringList cookieUrls = QgsSettings().value( "/iamauth/cookieurls", "" ).toString().split( ";" );
    for ( const QString &url : cookieUrls )
    {
      QList<QNetworkCookie> cookies = jar->cookiesForUrl( url );
      for ( QNetworkCookie cookie : cookies )
      {
        jar->deleteCookie( cookie );
      }
    }
    kApp->mainWindow()->messageBar()->pushMessage( tr( "Logout successful" ), Qgis::Info, 5 );
    mLogoutButton->hide();
    mLoginButton->show();
    mRefreshButton->click();
  }
  reply->deleteLater();
}
