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

#include <QBuffer>
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
#include <quazip5/quazipfile.h>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgssettings.h>

#include <kadas/core/kadas.h>
#include <kadas/gui/kadasprojecttemplateselectiondialog.h>


KadasProjectTemplateSelectionDialog::KadasProjectTemplateSelectionDialog( QWidget *parent )
  : QDialog( parent )
{
  setupUi( this );

  mProjectTempDir = new QTemporaryDir();
  mProjectTempDir->setAutoRemove( true );

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
    populateFileTree( projectTemplatesDir, mTreeWidget->invisibleRootItem(), provider );
  }

  connect( mTreeWidget, &QTreeWidget::itemClicked, this, &KadasProjectTemplateSelectionDialog::itemClicked );
  connect( mTreeWidget, &QTreeWidget::itemDoubleClicked, this, &KadasProjectTemplateSelectionDialog::itemDoubleClicked );

  mCreateButton = mButtonBox->addButton( tr( "Create" ), QDialogButtonBox::AcceptRole );
  mCreateButton->setEnabled( false );
  connect( mCreateButton, &QAbstractButton::clicked, this, &KadasProjectTemplateSelectionDialog::createProject );
}

KadasProjectTemplateSelectionDialog::~KadasProjectTemplateSelectionDialog()
{
  delete mProjectTempDir;
}

void KadasProjectTemplateSelectionDialog::parseServiceReply()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( QObject::sender() );
  if ( reply->error() == QNetworkReply::NoError )
  {
    QString serviceUrl = QgsSettings().value( "kadas/project_template_service" ).toString();
    QString baseUrl = serviceUrl.replace( QRegularExpression( "/rest/search/\?.*$" ), "/rest/content/items/" );
    QByteArray response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( response );
    QJsonObject root = doc.object();
    QJsonArray results = root.value( "results" ).toArray();
    QFileIconProvider iconProvider;

    QTreeWidgetItem *remoteItem = new QTreeWidgetItem( QStringList() << tr( "Remote templates" ) );
    remoteItem->setIcon( 0, iconProvider.icon( QFileIconProvider::Network ) );
    mTreeWidget->invisibleRootItem()->addChild( remoteItem );
    remoteItem->setExpanded( true );
    for ( const QJsonValue &value : results )
    {
      QJsonObject result = value.toObject();
      QTreeWidgetItem *item = new QTreeWidgetItem( QStringList() << result["title"].toString() );
      item->setIcon( 0, QIcon( ":/kadas/logo" ) );
      item->setData( 0, sItemUrlRole, baseUrl + result["id"].toString() + "/data" );
      remoteItem->addChild( item );
    }

    QTreeWidgetItem *localItem = new QTreeWidgetItem( QStringList() << tr( "Local templates" ) );
    localItem->setIcon( 0, iconProvider.icon( QFileIconProvider::Folder ) );
    mTreeWidget->invisibleRootItem()->addChild( localItem );
    localItem->setExpanded( true );
    QDir projectTemplatesDir( Kadas::projectTemplatesPath() );
    populateFileTree( projectTemplatesDir, localItem, iconProvider );
  }
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
  QTreeWidgetItem *item = mTreeWidget->currentItem();
  if ( !item )
  {
    return;
  }
  QString itemUrl = item->data( 0, sItemUrlRole ).toString();
  QString filePath = item->data( 0, sFilePathRole ).toString();
  if ( !filePath.isEmpty() )
  {
    mSelectedTemplate = filePath;
  }
  else if ( !itemUrl.isEmpty() )
  {

    QNetworkRequest request = QNetworkRequest( QUrl( itemUrl ) );
    QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( request );
    QEventLoop loop;
    connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    loop.exec( QEventLoop::ExcludeUserInputEvents );
    if ( reply->error() != QNetworkReply::NoError )
    {
      QgsDebugMsg( QString( "Could not read %1" ).arg( itemUrl ) );
      QMessageBox::critical( this, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return;
    }
    QString projectFileName = item->text( 0 ) + ".qgz";

    QByteArray data = reply->readAll();
    QBuffer buf( &data );
    QuaZip zip( &buf );
    zip.open( QuaZip::mdUnzip );
    if ( !zip.setCurrentFile( projectFileName, QuaZip::csInsensitive ) )
    {
      QgsDebugMsg( QString( "Could not find file %1 in archive %2" ).arg( projectFileName, itemUrl ) );
      QMessageBox::critical( this, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return;
    }
    QuaZipFile zipFile( &zip );

    QFile unzipFile( mProjectTempDir->filePath( projectFileName ) );
    if ( zipFile.open( QIODevice::ReadOnly ) && unzipFile.open( QIODevice::WriteOnly ) )
    {
      unzipFile.write( zipFile.readAll() );
    }
    else
    {
      QgsDebugMsg( QString( "Could not extract file %1 from archive %2 to dir %3" ).arg( projectFileName, itemUrl, mProjectTempDir->path() ) );
      QMessageBox::critical( this, tr( "Error" ),  tr( "Failed to read the project template." ) );
      return;
    }
    item->setData( 0, sFilePathRole, unzipFile.fileName() );
    mSelectedTemplate = unzipFile.fileName();
  }
  accept();
}
