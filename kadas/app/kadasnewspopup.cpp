/***************************************************************************
    kadasnewspopup.cpp
    ------------------
    copyright            : (C) 2022 by Sandro Mani
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

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#include <windows.h>

#include <QAxObject>
#include <QAxWidget>
#endif

#include "kadasnewspopup.h"

KadasNewsPopup *KadasNewsPopup::sInstance = nullptr;

#ifdef Q_OS_WIN

class KadasNewsWebView : public QAxWidget
{
  public:
    KadasNewsWebView( QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags() )
      : QAxWidget( parent, f )
    {
      setControl( QString::fromUtf8( "{8856F961-340A-11D0-A96B-00C04FD705A2}" ) );
    }

    void load( const QUrl &url )
    {
      dynamicCall( "Navigate(const QString&)", url.toString() );
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

class KadasNewsWebView : public QWidget
{
  public:
    KadasNewsWebView( QWidget *parent = nullptr )
      : QWidget( parent )
    {}

    void load( const QUrl &url )
    {
      QDesktopServices::openUrl( url );
    }
};

#endif


bool KadasNewsPopup::isConfigured()
{
  return !QgsSettings().value( "kadas/portalNewsUrl" ).toString().isEmpty();
}

void KadasNewsPopup::showIfNewsAvailable( bool force )
{
  QgsSettings settings;
  QString portalNewsUrl = settings.value( "kadas/portalNewsUrl" ).toString();
  QString lastPortalNewsVer = settings.value( "kadas/lastPortalNewsVer" ).toString();
  if ( portalNewsUrl.isEmpty() )
  {
    return;
  }

  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( QNetworkRequest( QUrl( portalNewsUrl ) ) );
  connect( reply, &QNetworkReply::finished, [reply, force, lastPortalNewsVer] {
    QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    reply->deleteLater();
    const QVariantList results = rootMap["results"].toList();
    if ( !results.isEmpty() )
    {
      QVariantMap chosenResult = results[0].toMap();
      QString lang = QgsSettings().value( "/locale/userLocale", "en" ).toString().left( 2 ).toUpper();
      QString langTag = "lang:" + lang;
      // Search result matching the current lang
      for ( const QVariant &result : results )
      {
        bool found = false;
        const QVariantList tags = result.toMap()["tags"].toList();
        for ( const QVariant &tag : tags )
        {
          if ( tag.toString() == langTag )
          {
            chosenResult = result.toMap();
            found = true;
            break;
          }
          if ( found )
          {
            break;
          }
        }
      }

      // Read version and URL for chosen result
      const QVariantList tags = chosenResult["tags"].toList();
      QString url = chosenResult["url"].toString();
      QString version;
      for ( const QVariant &tag : tags )
      {
        if ( tag.toString().startsWith( "version:" ) )
        {
          version = tag.toString().mid( 8 );
          break;
        }
      }
      if ( !url.isEmpty() && ( force || ( !version.isEmpty() && version > lastPortalNewsVer ) ) )
      {
        if ( sInstance == nullptr )
        {
          QDesktopServices::openUrl( url );
          QgsSettings().setValue( "kadas/lastPortalNewsVer", version );
          //          sInstance = new KadasNewsPopup( url );
          //          sInstance->show();
        }
        //        sInstance->raise();
      }
    }
  } );
}

KadasNewsPopup::KadasNewsPopup( const QString &url )
{
  setAttribute( Qt::WA_DeleteOnClose );
  setWindowTitle( tr( "KADAS Newsletter" ) );
  setObjectName( "NewsPopup" );
  setModal( true );
  resize( 800, 600 );

  setLayout( new QVBoxLayout );

  QFrame *frame = new QFrame();
  frame->setFrameShape( QFrame::StyledPanel );
  frame->setLayout( new QVBoxLayout );
  frame->layout()->setContentsMargins( 0, 0, 0, 0 );
  layout()->addWidget( frame );

  KadasNewsWebView *webView = new KadasNewsWebView();
  webView->load( QUrl( url ) );
  frame->layout()->addWidget( webView );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close );

  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  layout()->addWidget( bbox );
}

KadasNewsPopup::~KadasNewsPopup()
{
  sInstance = nullptr;
}
