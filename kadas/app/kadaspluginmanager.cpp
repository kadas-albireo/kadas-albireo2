/***************************************************************************
    kadaspluginmanager.h
    --------------------
    copyright            : (C) 2019 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAction>
#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QVersionNumber>
#include <quazip/quazipfile.h>

#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsnetworkcontentfetcher.h>
#include <qgis/qgssettings.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadaspluginmanager.h"
#include "kadaspythonintegration.h"


KadasPluginManagerInstallButton::KadasPluginManagerInstallButton( Status status, QWidget *parent )
  : QWidget( parent )
{
  setLayout( new QHBoxLayout );
  mButton = new QPushButton( this );
  mProgressbar = new QProgressBar( this );
  mProgressbar->setRange( 0, 100 );
  layout()->addWidget( mButton );
  layout()->addWidget( mProgressbar );
  layout()->setContentsMargins( 0, 0, 0, 0 );
  layout()->setSpacing( 0 );
  setStatus( status );

  connect( mButton, &QPushButton::clicked, this, &KadasPluginManagerInstallButton::clicked );
}

void KadasPluginManagerInstallButton::setStatus( Status status, int progress )
{
  mStatus = status;
  if ( progress == 0 )
  {
    mButton->setVisible( true );
    mProgressbar->setVisible( false );
  }
  else
  {
    mButton->setVisible( false );
    mProgressbar->setVisible( true );
    mProgressbar->setValue( progress );
  }
  mButton->setEnabled( status != Installing && status != Uninstalling );
  switch ( status )
  {
    case Install:
      mButton->setText( tr( "Install" ) );
      break;
    case Installing:
      mButton->setText( tr( "Installing..." ) );
      mProgressbar->setFormat( tr( "Installing..." ) );
      break;
    case Uninstall:
      mButton->setText( tr( "Uninstall" ) );
      break;
    case Uninstalling:
      mButton->setText( tr( "Uninstalling..." ) );
      mProgressbar->setFormat( tr( "Uninstalling..." ) );
      break;
    case Update:
      mButton->setText( tr( "Update" ) );
  }
}


KadasPluginManager::KadasPluginManager( QgsMapCanvas *canvas, QAction *action )
  : KadasBottomBar( canvas ), mAction( action )
{
  setupUi( this );
  mCloseButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  mInstalledTreeWidget->header()->setSectionResizeMode( 0, QHeaderView::Stretch );
  mInstalledTreeWidget->header()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
  mInstalledTreeWidget->header()->setStretchLastSection( true );
}

void KadasPluginManager::loadPlugins()
{
  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    qWarning() << "KadasPluginManager needs the python integration";
    return;
  }

  mInstalledTreeWidget->clear();
  mAvailableTreeWidget->clear();

  mAvailablePlugins = availablePlugins();

  //detect user plugins
  QDir userPluginDir( p->homePluginsPath() );
  QStringList installedUserPlugins = userPluginDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
  QMap<QString, PluginInfo> installedPluginInfo; // name/pluginInfo

  QStringList::const_iterator installedIt = installedUserPlugins.constBegin();
  for ( ; installedIt != installedUserPlugins.constEnd(); ++installedIt )
  {
    PluginInfo pi;
    pi.name = p->getPluginMetadata( *installedIt, "name" );
    pi.description = p->getPluginMetadata( *installedIt, "description" );
    pi.version = p->getPluginMetadata( *installedIt, "version" );
    installedPluginInfo.insert( pi.name, pi );
    QTreeWidgetItem *installedItem = new QTreeWidgetItem();
    installedItem->setText( INSTALLED_TREEWIDGET_COLUMN_NAME, pi.name );
    installedItem->setToolTip( INSTALLED_TREEWIDGET_COLUMN_NAME, pi.description );
    installedItem->setText( INSTALLED_TREEWIDGET_COLUMN_VERSION, pi.version );
    installedItem->setData( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole, *installedIt );
    if ( p->isPluginEnabled( *installedIt ) )
    {
      setItemDeactivatable( installedItem );
    }
    else
    {
      setItemActivatable( installedItem );
    }
    mInstalledTreeWidget->addTopLevelItem( installedItem );
  }

  QMap<QString, PluginInfo>::const_iterator pluginIt = mAvailablePlugins.constBegin();
  for ( ; pluginIt != mAvailablePlugins.constEnd(); ++pluginIt )
  {
    QTreeWidgetItem *availableItem = new QTreeWidgetItem();
    availableItem->setText( AVAILABLE_TREEWIDGET_COLUMN_NAME, pluginIt.key() );
    availableItem->setToolTip( AVAILABLE_TREEWIDGET_COLUMN_NAME, pluginIt.value().description );
    mAvailableTreeWidget->addTopLevelItem( availableItem );
    QString repoVersion = pluginIt.value().version;
    if ( installedPluginInfo.contains( pluginIt.key() ) )
    {
      setItemRemoveable( availableItem );

      QString installedVersion = installedPluginInfo.value( pluginIt.key() ).version;

      if ( updateAvailable( installedVersion, repoVersion ) )
      {
        setItemUpdateable( availableItem );
      }
      else
      {
        setItemNotUpdateable( availableItem );
      }
    }
    else
    {
      setItemInstallable( availableItem, repoVersion );
    }
  }

  mAvailableTreeWidget->resizeColumnToContents( 0 );
}

void KadasPluginManager::installMandatoryPlugins()
{
  QMap<QString, PluginInfo>::const_iterator pluginIt = mAvailablePlugins.constBegin();
  for ( ; pluginIt != mAvailablePlugins.constEnd(); ++pluginIt )
  {
    qDebug() << "Available plugin" << pluginIt.value().name << "is mandatory:" << pluginIt.value().mandatory;

    if ( pluginIt.value().mandatory )
    {
      QString pluginName = pluginIt.key();

      QList<QTreeWidgetItem *> availableTreeWidgetItems = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, INSTALLED_TREEWIDGET_COLUMN_NAME );
      if ( availableTreeWidgetItems.size() > 0 )
      {
        // Plugin already installed
        continue;
      }

      KadasPluginManagerInstallButton *button = getInstallButton( pluginName );
      if ( button == nullptr )
        continue;

      bool success = installPlugin( pluginName, pluginIt.value().downloadLink, pluginIt.value().description, pluginIt.value().version, button );

      if ( !success )
      {
        kApp->mainWindow()->messageBar()->pushMessage( tr( "Mandatory plugin installation error" ), tr( "Plugin '%1' couldn't be installed" ).arg( pluginName ), Qgis::Warning, kApp->mainWindow()->messageTimeout() );
      }
    }
  }
}

void KadasPluginManager::updateAllPlugins()
{
  QTreeWidgetItemIterator it( mInstalledTreeWidget );
  while ( *it )
  {
    const QString installedPluginName = ( *it )->text( INSTALLED_TREEWIDGET_COLUMN_NAME );

    if ( !mAvailablePlugins.contains( installedPluginName ) )
    {
      ++it;
      continue;
    }

    const PluginInfo availablePluginInfo = mAvailablePlugins.value( installedPluginName );
    const QString installedPluginVersion = ( *it )->text( INSTALLED_TREEWIDGET_COLUMN_VERSION );

    if ( availablePluginInfo.version > installedPluginVersion )
    {
      QString moduleName = ( *it )->data( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole ).toString();
      KadasPluginManagerInstallButton *button = getInstallButton( installedPluginName );
      if ( button == nullptr )
        continue;

      bool success = updatePlugin( installedPluginName, moduleName, availablePluginInfo.downloadLink, availablePluginInfo.description, availablePluginInfo.version, button );

      if ( success )
        kApp->mainWindow()->messageBar()->pushMessage( tr( "Plugin update" ), tr( "Plugin '%1' has been updated to version %2" ).arg( installedPluginName, availablePluginInfo.version ), Qgis::Info, kApp->mainWindow()->messageTimeout() );
      else
        kApp->mainWindow()->messageBar()->pushMessage( tr( "Plugin update error" ), tr( "Plugin '%1' update failed" ).arg( installedPluginName ), Qgis::Warning, kApp->mainWindow()->messageTimeout() );
    }

    ++it;
  }
}

KadasPluginManager::KadasPluginManager()
  : KadasBottomBar( nullptr )
{
}

QMap<QString, KadasPluginManager::PluginInfo> KadasPluginManager::availablePlugins()
{
  QgsSettings s;
  QString repoUrl = s.value( "/PythonPluginRepository/repositoryUrl", "http://pkg.sourcepole.ch/kadas/plugins/qgis-repo.xml" ).toString();
  QNetworkReply *reply = QgsNetworkAccessManager::instance()->get( QNetworkRequest( QUrl( repoUrl ) ) );
  QEventLoop evLoop;
  connect( reply, &QNetworkReply::finished, &evLoop, &QEventLoop::quit );
  evLoop.exec( QEventLoop::ExcludeUserInputEvents );
  reply->deleteLater();

  QMap<QString, PluginInfo> pluginMap;

  if ( reply->error() != QNetworkReply::NoError )
  {
    return pluginMap;
  }
  QByteArray response = reply->readAll();

  QDomDocument xml;
  QJsonParseError err;

  // Attemp to read as JSON first, then XML
  QJsonDocument json = QJsonDocument::fromJson( response, &err );
  if ( !json.isNull() )
  {
    QString baseUrl = QString( repoUrl ).replace( QRegularExpression( "/rest/search/?\?.*$" ), "/rest/content/items/" );

    for ( const QJsonValueRef &resultRef : json.object()["results"].toArray() )
    {
      QJsonObject result = resultRef.toObject();
      KadasPluginManager::PluginInfo pluginInfo;
      pluginInfo.name = result["title"].toString();
      pluginInfo.mandatory = false;
      for ( const QJsonValueRef &tagRef : result["tags"].toArray() )
      {
        QString tag = tagRef.toString();
        if ( tag.startsWith( "version:" ) )
        {
          pluginInfo.version = tag.mid( 8 );
        }

        if ( tag == "mandatoryplugin" )
        {
          pluginInfo.mandatory = true;
        }
      }
      pluginInfo.description = result["description"].toString();
      pluginInfo.downloadLink = baseUrl + result["id"].toString() + "/data";

      qDebug() << "Got plugin info name" << pluginInfo.name << "is mandatory:" << pluginInfo.mandatory;
      qDebug() << "Json payload:" << result.toVariantMap();

      pluginMap.insert( pluginInfo.name, pluginInfo );
    }
  }
  else if ( xml.setContent( response ) )
  {
    QDomNodeList pluginNodeList = xml.elementsByTagName( "pyqgis_plugin" );
    for ( int i = 0; i < pluginNodeList.size(); ++i )
    {
      QDomElement pluginElem = pluginNodeList.at( i ).toElement();
      KadasPluginManager::PluginInfo pluginInfo;
      pluginInfo.name = pluginElem.attribute( "name" );
      pluginInfo.version = pluginElem.attribute( "version" );
      pluginInfo.description = pluginElem.firstChildElement( "description" ).text();
      pluginInfo.downloadLink = pluginElem.firstChildElement( "download_url" ).text();
      QDomElement mandatoryPluginElement = pluginElem.firstChildElement( "mandatoryplugin" );
      pluginInfo.mandatory = !mandatoryPluginElement.isNull();
      pluginMap.insert( pluginInfo.name, pluginInfo );
    }
  }
  return pluginMap;
}

void KadasPluginManager::on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column )
{
  if ( column != 0 )
  {
    return;
  }

  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return;
  }

  QString pluginModule = item->data( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole ).toString();
  if ( p->isPluginLoaded( pluginModule ) )
  {
    if ( p->disablePlugin( pluginModule ) )
    {
      setItemActivatable( item );
    }
  }
  else
  {
    if ( p->loadPlugin( pluginModule ) )
    {
      setItemDeactivatable( item );
    }
  }
}

void KadasPluginManager::on_mCloseButton_clicked()
{
  if ( mAction )
  {
    mAction->toggle();
  }
}

void KadasPluginManager::updateButtonClicked()
{
  QString pluginName = sender()->property( PROPERTY_PLUGIN_NAME ).toString();
  if ( pluginName.isEmpty() )
  {
    return;
  }

  KadasPluginManagerInstallButton *b = qobject_cast<KadasPluginManagerInstallButton *>( QObject::sender() );
  QString downloadPath = mAvailablePlugins.value( pluginName ).downloadLink;
  QString pluginTooltip = mAvailablePlugins.value( pluginName ).description;
  QString pluginVersion = mAvailablePlugins.value( pluginName ).version;

  //get module name
  QList<QTreeWidgetItem *> installed = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, INSTALLED_TREEWIDGET_COLUMN_NAME );
  if ( installed.size() < 1 )
  {
    return;
  }
  QString moduleName = installed.at( 0 )->data( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole ).toString();
  updatePlugin( pluginName, moduleName, downloadPath, pluginTooltip, pluginVersion, b );

  QList<QTreeWidgetItem *> available = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, AVAILABLE_TREEWIDGET_COLUMN_NAME );
  if ( available.size() > 0 )
  {
    setItemNotUpdateable( available.at( 0 ) );
  }
}

void KadasPluginManager::installButtonClicked()
{
  KadasPluginManagerInstallButton *b = qobject_cast<KadasPluginManagerInstallButton *>( QObject::sender() );
  QString pluginName = sender()->property( PROPERTY_PLUGIN_NAME ).toString();
  if ( pluginName.isEmpty() )
  {
    return;
  }

  QList<QTreeWidgetItem *> installed = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, INSTALLED_TREEWIDGET_COLUMN_NAME );
  KadasPluginManagerInstallButton::Status prevStatus = b->status();
  bool success = false;

  if ( installed.size() < 1 ) //not yet installed
  {
    QString downloadPath = mAvailablePlugins.value( pluginName ).downloadLink;
    QString pluginTooltip = mAvailablePlugins.value( pluginName ).description;
    QString version = mAvailablePlugins.value( pluginName ).version;
    if ( !downloadPath.isEmpty() )
    {
      success = installPlugin( pluginName, downloadPath, pluginTooltip, version, b );
    }
  }
  else //plugin installed, remove it
  {
    if ( QMessageBox::question( this, tr( "Remove plugin" ), tr( "Are you sure you want to remove the plugin '%1'?" ).arg( pluginName ) ) == QMessageBox::Yes )
    {
      QString moduleName = installed.at( 0 )->data( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole ).toString();
      success = uninstallPlugin( pluginName, moduleName, b );
    }
  }
  if ( !success )
  {
    b->setStatus( prevStatus );
  }
}

bool KadasPluginManager::installPlugin( const QString &pluginName, const QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion, KadasPluginManagerInstallButton *b )
{
  b->setStatus( KadasPluginManagerInstallButton::Installing );

  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return false;
  }

  QString pp = p->homePluginsPath();
  if ( !QDir().mkpath( pp ) )
  {
    kApp->mainWindow()->messageBar()->pushCritical( tr( "Plugin install failed" ), tr( "Error creating plugin directory" ) );
    return false;
  }

  //download and unzip in kadasPluginsPath
  QgsNetworkContentFetcher nf;
  QTextStream( stdout ) << downloadUrl << Qt::endl;
  QUrl repositoryUrl( downloadUrl );
  QEventLoop e;
  nf.fetchContent( repositoryUrl );
  connect( &nf, &QgsNetworkContentFetcher::downloadProgress, [b]( qint64 rec, qint64 tot ) {
    b->setStatus( KadasPluginManagerInstallButton::Installing, double( rec ) / tot * 100 );
  } );
  QObject::connect( &nf, &QgsNetworkContentFetcher::finished, &e, &QEventLoop::quit );
  e.exec();

  if ( nf.reply()->error() != QNetworkReply::NoError )
  {
    kApp->mainWindow()->messageBar()->pushCritical( tr( "Plugin install failed" ), tr( "Error downloading plugin: %1" ).arg( nf.reply()->errorString() ) );
    return false;
  }

  QByteArray pluginData = nf.reply()->readAll();
  QBuffer buf( &pluginData );

  QuaZip zip( &buf );
  zip.open( QuaZip::mdUnzip );
  QuaZipFile file( &zip );
  QString moduleName;
  for ( bool f = zip.goToFirstFile(); f; f = zip.goToNextFile() )
  {
    QString fileName = zip.getCurrentFileName();
    QString absoluteFilePath = QDir( pp ).absoluteFilePath( fileName );

    if ( moduleName.isEmpty() )
    {
      QStringList parts = QDir::cleanPath( QFileInfo( fileName ).path() ).split( "/", Qt::SkipEmptyParts );
      if ( !parts.isEmpty() )
      {
        moduleName = parts[0];
      }
    }

    if ( fileName.endsWith( "/" ) )
    {
      continue;
    }

    if ( file.open( QIODevice::ReadOnly ) )
    {
      QByteArray ba = file.readAll();
      file.close();

      QDir().mkpath( QFileInfo( absoluteFilePath ).absolutePath() );
      QFile dstFile( absoluteFilePath );
      if ( dstFile.open( QIODevice::WriteOnly ) )
      {
        dstFile.write( ba );
        dstFile.close();
      }
    }
  }

  //insert into mInstalledTreeWidget
  QTreeWidgetItem *installedItem = new QTreeWidgetItem();
  installedItem->setText( INSTALLED_TREEWIDGET_COLUMN_NAME, pluginName );
  installedItem->setText( INSTALLED_TREEWIDGET_COLUMN_VERSION, pluginVersion );
  installedItem->setToolTip( INSTALLED_TREEWIDGET_COLUMN_NAME, pluginTooltip );
  installedItem->setData( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::UserRole, moduleName );

  p->pluginList();
  if ( p->loadPlugin( moduleName ) )
  {
    setItemDeactivatable( installedItem );
  }
  else
  {
    setItemActivatable( installedItem );
  }
  mInstalledTreeWidget->addTopLevelItem( installedItem );

  //change icon in mAvailableTreeWidget
  QList<QTreeWidgetItem *> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, INSTALLED_TREEWIDGET_COLUMN_NAME );
  if ( availableItem.size() > 0 )
  {
    setItemRemoveable( availableItem.at( 0 ) );
    setItemNotUpdateable( availableItem.at( 0 ) );
  }
  return true;
}

bool KadasPluginManager::uninstallPlugin( const QString &pluginName, const QString &moduleName, KadasPluginManagerInstallButton *b )
{
  b->setStatus( KadasPluginManagerInstallButton::Uninstalling );

  //deactivate first
  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return false;
  }
  p->unloadPlugin( moduleName );

  //remove folder
  QDir pluginDir( p->homePluginsPath() + "/" + moduleName );
  if ( !pluginDir.removeRecursively() )
  {
    QMessageBox::critical( this, tr( "Plugin deinstallation failed" ), tr( "The deinstallation of the plugin '%1' failed" ).arg( pluginName ) );
    return false;
  }

  //remove entry from mInstalledTreeWidget
  QList<QTreeWidgetItem *> installedItem = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, INSTALLED_TREEWIDGET_COLUMN_NAME );
  if ( installedItem.size() > 0 )
  {
    delete mInstalledTreeWidget->takeTopLevelItem( mInstalledTreeWidget->indexOfTopLevelItem( installedItem.at( 0 ) ) );
  }

  //set icon to download in mAvailableTreeWidget
  QList<QTreeWidgetItem *> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, AVAILABLE_TREEWIDGET_COLUMN_NAME );
  if ( mAvailablePlugins.contains( pluginName ) )
  {
    setItemInstallable( availableItem.at( 0 ), mAvailablePlugins[pluginName].version );
  }
  return true;
}

bool KadasPluginManager::updatePlugin( const QString &pluginName, const QString &moduleName, const QString &downloadUrl, const QString &pluginTooltip, const QString &pluginVersion, KadasPluginManagerInstallButton *b )
{
  uninstallPlugin( pluginName, moduleName, b );
  return installPlugin( pluginName, downloadUrl, pluginTooltip, pluginVersion, b );
}

void KadasPluginManager::setItemInstallable( QTreeWidgetItem *item, const QString &version )
{
  changeItemInstallationState( item, KadasPluginManagerInstallButton::Install );
  item->setText( 2, version );
}

void KadasPluginManager::setItemRemoveable( QTreeWidgetItem *item )
{
  changeItemInstallationState( item, KadasPluginManagerInstallButton::Uninstall );
}

void KadasPluginManager::changeItemInstallationState( QTreeWidgetItem *item, KadasPluginManagerInstallButton::Status buttonStatus )
{
  if ( item )
  {
    KadasPluginManagerInstallButton *b = new KadasPluginManagerInstallButton( buttonStatus );
    b->setProperty( PROPERTY_PLUGIN_NAME, item->text( INSTALLED_TREEWIDGET_COLUMN_NAME ) );
    QObject::connect( b, &KadasPluginManagerInstallButton::clicked, this, &KadasPluginManager::installButtonClicked );
    mAvailableTreeWidget->setItemWidget( item, INSTALLED_TREEWIDGET_COLUMN_VERSION, b );
  }
}

void KadasPluginManager::setItemActivatable( QTreeWidgetItem *item )
{
  if ( item )
  {
    item->setCheckState( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::Unchecked );
  }
}

void KadasPluginManager::setItemDeactivatable( QTreeWidgetItem *item )
{
  if ( item )
  {
    item->setCheckState( INSTALLED_TREEWIDGET_COLUMN_NAME, Qt::Checked );
  }
}

void KadasPluginManager::setItemUpdateable( QTreeWidgetItem *item )
{
  if ( item )
  {
    KadasPluginManagerInstallButton *b = new KadasPluginManagerInstallButton( KadasPluginManagerInstallButton::Update );
    b->setProperty( PROPERTY_PLUGIN_NAME, item->text( AVAILABLE_TREEWIDGET_COLUMN_NAME ) );
    QObject::connect( b, &KadasPluginManagerInstallButton::clicked, this, &KadasPluginManager::updateButtonClicked );
    mAvailableTreeWidget->setItemWidget( item, 2, b );
  }
}

void KadasPluginManager::setItemNotUpdateable( QTreeWidgetItem *item )
{
  if ( item )
  {
    mAvailableTreeWidget->removeItemWidget( item, 2 );
    item->setText( 2, tr( "up-to-date" ) );
  }
}

bool KadasPluginManager::updateAvailable( const QString &installedVersion, const QString &repoVersion )
{
  QVersionNumber installed = QVersionNumber::fromString( installedVersion );
  QVersionNumber repo = QVersionNumber::fromString( repoVersion );
  if ( installed.isNull() || repo.isNull() )
  {
    return false;
  }

  return ( repo > installed );
}

KadasPluginManagerInstallButton *KadasPluginManager::getInstallButton( const QString &pluginName )
{
  KadasPluginManagerInstallButton *button = nullptr;
  QList<QTreeWidgetItem *> availableTreeWidgetItems = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, AVAILABLE_TREEWIDGET_COLUMN_NAME );
  if ( availableTreeWidgetItems.size() > 0 )
  {
    QTreeWidgetItem *availableTreeWidgetItem = availableTreeWidgetItems.at( 0 );
    QWidget *itemWidget = mAvailableTreeWidget->itemWidget( availableTreeWidgetItem, AVAILABLE_TREEWIDGET_COLUMN_BUTTON );
    button = qobject_cast<KadasPluginManagerInstallButton *>( itemWidget );
  }

  return button;
}
