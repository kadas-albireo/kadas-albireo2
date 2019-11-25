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
#include <QDialog>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QStackedLayout>
#include <QToolButton>

#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/iamauth/kadasiamauth.h>


class WebAxWidget : public QAxWidget
{
  public:
    WebAxWidget( QWidget *parent = 0, Qt::WindowFlags f = 0 )
      : QAxWidget( parent, f ) { }
  protected:
    virtual bool translateKeyEvent( int message, int keycode ) const
    {
      if ( message >= WM_KEYFIRST && message <= WM_KEYLAST )
        return true;
      else
        return QAxWidget::translateKeyEvent( message, keycode );
    }
};


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



KadasIamAuth::KadasIamAuth( QToolButton *loginButton, QToolButton *refreshButton, QObject *parent )
  : QObject( parent ), mLoginButton( loginButton ), mRefreshButton( refreshButton )
{
  if ( QgsSettings().value( "/iamauth/loginurl", "" ).toString().isEmpty() )
  {
    mLoginButton->hide();
  }
  else
  {
    connect( mLoginButton, &QToolButton::clicked, this, &KadasIamAuth::performLogin );
  }
}


void KadasIamAuth::performLogin()
{
  mLoginDialog = new StackedDialog( kApp->mainWindow() );
  mLoginDialog->setWindowTitle( tr( "eIAM Authentication" ) );
  mLoginDialog->resize( 1000, 480 );

  WebAxWidget *webWidget = new WebAxWidget();
  webWidget->setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
  webWidget->dynamicCall( "Navigate(const QString&)", QgsSettings().value( "/iamauth/loginurl", "" ).toString() );
  connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
  connect( webWidget, SIGNAL( NewWindow3( IDispatch **, bool &, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch **, bool &, uint, QString, QString ) ) );
  connect( webWidget, SIGNAL( WindowClosing( bool, bool & ) ), this, SLOT( handleWindowClose( bool, bool & ) ) );
  mLoginDialog->pushWidget( webWidget );
  mLoginDialog->exec();
}

void KadasIamAuth::checkLoginComplete( QString /*addr*/ )
{
  if ( mLoginDialog )
  {
    WebAxWidget *webWidget = static_cast<WebAxWidget *>( QObject::sender() );
    QUrl url( webWidget->dynamicCall( "LocationURL()" ).toString() );
    QUrl baseUrl;
    baseUrl.setScheme( url.scheme() );
    baseUrl.setHost( url.host() );
    QAxObject *document = webWidget->querySubObject( "Document()" );
    QStringList cookies = document->property( "cookie" ).toString().split( QRegExp( "\\s*;\\s*" ) );
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
        mRefreshButton->click();
        /*
        QWebView* view = new QWebView();
        QWebPage* page = new QWebPage();
        page->setNetworkAccessManager( QgsNetworkAccessManager::instance() );
        view->setPage( page );
        QNetworkRequest req;
        req.setUrl( QUrl( "https://www.arcgis.com/sharing/rest/search" ) );
        QSslConfiguration conf = req.sslConfiguration();
        conf.setPeerVerifyMode( QSslSocket::VerifyNone );
        req.setSslConfiguration( conf );
        view->load( req );
        view->show();
        view->setAttribute( Qt::WA_DeleteOnClose );*/
      }
    }
  }
}

void KadasIamAuth::handleNewWindow( IDispatch **ppDisp, bool & /*cancel*/, uint /*dwFlags*/, QString /*bstrUrlContext*/, QString /*bstrUrl*/ )
{
  if ( mLoginDialog )
  {
    WebAxWidget *webWidget = new WebAxWidget;
    webWidget->setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
    connect( webWidget, SIGNAL( NavigateComplete( QString ) ), this, SLOT( checkLoginComplete( QString ) ) );
    connect( webWidget, SIGNAL( NewWindow3( IDispatch **, bool &, uint, QString, QString ) ), this, SLOT( handleNewWindow( IDispatch **, bool &, uint, QString, QString ) ) );
    connect( webWidget, SIGNAL( WindowClosing( bool, bool & ) ), this, SLOT( handleWindowClose( bool, bool & ) ) );
    IDispatch *appDisp;
    QAxObject *appDispObj = webWidget->querySubObject( "Application()" );
    appDispObj->queryInterface( IID_IDispatch, ( void ** )&appDisp );
    *ppDisp = appDisp;
    mLoginDialog->pushWidget( webWidget );
  }
}

void KadasIamAuth::handleWindowClose( bool /*isChild*/, bool & /*cancel*/ )
{
  if ( mLoginDialog )
  {
    mLoginDialog->popWidget( qobject_cast<QWidget *>( QObject::sender() ) );
  }
}

#else

#include <QToolButton>

#include <kadas/app/iamauth/kadasiamauth.h>

KadasIamAuth::KadasIamAuth( QToolButton *loginButton, QToolButton *refreshButton, QObject *parent )
  : QObject( parent ), mLoginButton( loginButton ), mRefreshButton( refreshButton )
{
  mLoginButton->hide();
}

void KadasIamAuth::performLogin()
{
}

void KadasIamAuth::checkLoginComplete( QString /*addr*/ )
{
}

void KadasIamAuth::handleNewWindow( IDispatch **ppDisp, bool & /*cancel*/, uint /*dwFlags*/, QString /*bstrUrlContext*/, QString /*bstrUrl*/ )
{
}

void KadasIamAuth::handleWindowClose( bool /*isChild*/, bool & /*cancel*/ )
{
}

#endif
