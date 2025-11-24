/***************************************************************************
    kadasprojecttemplateselectiondialog.cpp
    ---------------------------------------
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

#include <QDialogButtonBox>
#include <QDirIterator>
#include <QFileIconProvider>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QVBoxLayout>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include "kadas/core/kadas.h"
#include "kadas/gui/kadasprojecttemplateselectiondialog.h"


KadasProjectTemplateSelectionDialog::KadasProjectTemplateSelectionDialog( QWidget *parent )
  : QDialog( parent )
{
  mUi.setupUi( this );

  QString serviceUrl = QgsSettings().value( "kadas/project_template_service" ).toString();
  bool offline = QgsSettings().value( "/kadas/isOffline" ).toBool();
  if ( !offline && !serviceUrl.isEmpty() )
  {
    QNetworkRequest req( serviceUrl );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( req );
    connect( reply, &QNetworkReply::finished, this, &KadasProjectTemplateSelectionDialog::parseServiceReply );
  }
  else
  {
    QFileIconProvider provider;
    QDir projectTemplatesDir( Kadas::projectTemplatesPath() );
    populateFileTree( projectTemplatesDir, mUi.mTreeWidget->invisibleRootItem(), provider );
  }

  connect( mUi.mTreeWidget, &QTreeWidget::itemClicked, this, &KadasProjectTemplateSelectionDialog::itemClicked );
  connect( mUi.mTreeWidget, &QTreeWidget::itemDoubleClicked, this, &KadasProjectTemplateSelectionDialog::itemDoubleClicked );

  mCreateButton = mUi.mButtonBox->addButton( tr( "Create" ), QDialogButtonBox::AcceptRole );
  mCreateButton->setEnabled( false );
  connect( mCreateButton, &QAbstractButton::clicked, this, &KadasProjectTemplateSelectionDialog::createProject );
}

void KadasProjectTemplateSelectionDialog::parseServiceReply()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  QFileIconProvider iconProvider;

  if ( reply->error() == QNetworkReply::NoError )
  {
    QString serviceUrl = QgsSettings().value( "kadas/project_template_service" ).toString();
    QString baseUrl = serviceUrl.replace( QRegularExpression( "/rest/search/?\?.*$" ), "/rest/content/items/" );
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( response );
    QJsonObject root = doc.object();
    QJsonArray results = root.value( "results" ).toArray();

    QTreeWidgetItem *remoteItem = new QTreeWidgetItem( QStringList() << tr( "Remote templates" ) );
    remoteItem->setIcon( 0, iconProvider.icon( QFileIconProvider::Network ) );
    mUi.mTreeWidget->invisibleRootItem()->addChild( remoteItem );
    remoteItem->setExpanded( true );
    for ( const QJsonValue &value : results )
    {
      QJsonObject result = value.toObject();
      QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << result["title"].toString() );
      item->setIcon( 0, QIcon( ":/kadas/logo" ) );
      item->setData( 0, sItemUrlRole, baseUrl + result["id"].toString() + "/data" );
      remoteItem->addChild( item );
    }
  }

  QTreeWidgetItem *localItem = new QTreeWidgetItem( QStringList() << tr( "Local templates" ) );
  localItem->setIcon( 0, iconProvider.icon( QFileIconProvider::Folder ) );
  mUi.mTreeWidget->invisibleRootItem()->addChild( localItem );
  localItem->setExpanded( true );
  QDir projectTemplatesDir( Kadas::projectTemplatesPath() );
  populateFileTree( projectTemplatesDir, localItem, iconProvider );
  reply->deleteLater();
}

void KadasProjectTemplateSelectionDialog::populateFileTree( const QDir &dir, QTreeWidgetItem *parent, const QFileIconProvider &iconProvider )
{
  QStringList folderNames = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
  for ( const QString &folderName : folderNames )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << folderName );
    item->setIcon( 0, iconProvider.icon( QFileIconProvider::Folder ) );
    parent->addChild( item );
    item->setExpanded( true );
    populateFileTree( QDir( dir.absoluteFilePath( folderName ) ), item, iconProvider );
  }

  QStringList fileNames = dir.entryList( QStringList() << "*.qgz", QDir::Files | QDir::NoDotAndDotDot, QDir::Name );
  for ( const QString &fileName : fileNames )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << QFileInfo( fileName ).completeBaseName() );
    item->setIcon( 0, QIcon( ":/kadas/logo" ) );
    item->setData( 0, sFilePathRole, dir.absoluteFilePath( fileName ) );
    parent->addChild( item );
  }
}

void KadasProjectTemplateSelectionDialog::itemClicked( QTreeWidgetItem *item, int column )
{
  QString itemUrl = item->data( 0, sItemUrlRole ).toString();
  QString filePath = item->data( 0, sFilePathRole ).toString();
  mCreateButton->setEnabled( !itemUrl.isEmpty() || !filePath.isEmpty() );
}

void KadasProjectTemplateSelectionDialog::itemDoubleClicked( QTreeWidgetItem *item, int column )
{
  createProject();
}

void KadasProjectTemplateSelectionDialog::createProject()
{
  QTreeWidgetItem *item = mUi.mTreeWidget->currentItem();
  if ( !item )
  {
    return;
  }
  QString itemUrl = item->data( 0, sItemUrlRole ).toString();
  QString filePath = item->data( 0, sFilePathRole ).toString();
  if ( !filePath.isEmpty() )
  {
    emit templateSelected( filePath, QUrl() );
  }
  else if ( !itemUrl.isEmpty() )
  {
    QUrl url( itemUrl );
    url.setFragment( item->text( 0 ) + ".qgz" );
    emit templateSelected( QString(), url );
  }
  accept();
}
