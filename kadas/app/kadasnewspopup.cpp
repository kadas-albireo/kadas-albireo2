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

#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWebView>

#include <kadas/app/kadasnewspopup.h>

KadasNewsPopup *KadasNewsPopup::sInstance = nullptr;

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
  connect( reply, &QNetworkReply::finished, [reply, force, lastPortalNewsVer]
  {
    QVariantMap rootMap = QJsonDocument::fromJson( reply->readAll() ).object().toVariantMap();
    reply->deleteLater();
    QVariantList results = rootMap["results"].toList();
    if ( !results.isEmpty() )
    {
      QVariantMap result = results[0].toMap();
      QVariantList tags = result["tags"].toList();
      QString url = result["url"].toString();
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
          sInstance = new KadasNewsPopup( url, version );
          sInstance->show();
        }
        sInstance->raise();
      }
    }
  } );
}

KadasNewsPopup::KadasNewsPopup( const QString &url, const QString &version )
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

  QWebView *webView = new QWebView();
  webView->load( QUrl( url ) );
  webView->settings()->setAttribute( QWebSettings::JavascriptCanOpenWindows, true );
  webView->settings()->setAttribute( QWebSettings::JavascriptCanAccessClipboard, true );
  frame->layout()->addWidget( webView );

  QCheckBox *checkBox = new QCheckBox( tr( "Don't show this newsletter again" ) );
  checkBox->setChecked( version == QgsSettings().value( "kadas/lastPortalNewsVer" ).toString() );
  layout()->addWidget( checkBox );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close );

  QPushButton *closeButton = bbox->button( QDialogButtonBox::Close );
  connect( closeButton, &QPushButton::pressed, this, [checkBox, version]
  {
    if ( checkBox->isChecked() )
    {
      QgsSettings().setValue( "kadas/lastPortalNewsVer", version );
    }
    else
    {
      QgsSettings().setValue( "kadas/lastPortalNewsVer", "" );
    }
  } );

  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  layout()->addWidget( bbox );
}

KadasNewsPopup::~KadasNewsPopup()
{
  sInstance = nullptr;
}
