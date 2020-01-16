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
#include <QDir>
#include <QDomDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <quazip5/quazipfile.h>

#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsnetworkcontentfetcher.h>
#include <qgis/qgssettings.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadaspluginmanager.h>
#include <kadas/app/kadaspythonintegration.h>

KadasPluginManager::KadasPluginManager( QgsMapCanvas *canvas, QAction *action ): KadasBottomBar( canvas ), mAction( action )
{
  setupUi( this );
  mCloseButton->setIcon( QIcon( ":/kadas/icons/close" ) );

  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return;
  }

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
    installedItem->setText( 0, pi.name );
    installedItem->setToolTip( 0, pi.description );
    installedItem->setData( 0, Qt::UserRole, *installedIt );
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

  QMap<QString, PluginInfo >::const_iterator pluginIt = mAvailablePlugins.constBegin();
  for ( ; pluginIt != mAvailablePlugins.constEnd(); ++pluginIt )
  {
    QTreeWidgetItem *availableItem = new QTreeWidgetItem();
    availableItem->setText( 0, pluginIt.key() );
    availableItem->setToolTip( 0, pluginIt.value().description );
    mAvailableTreeWidget->addTopLevelItem( availableItem );
    if ( installedPluginInfo.contains( pluginIt.key() ) )
    {
      setItemRemoveable( availableItem );

      QString repoVersion = pluginIt.value().version;
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
      setItemInstallable( availableItem );
    }
  }

  mInstalledTreeWidget->header()->setStretchLastSection( true );
  mAvailableTreeWidget->resizeColumnToContents( 0 );
}

KadasPluginManager::KadasPluginManager(): KadasBottomBar( 0 )
{
}

QMap< QString, KadasPluginManager::PluginInfo > KadasPluginManager::availablePlugins()
{
  QgsSettings s;
  QString repoUrl = s.value( "/PythonPluginRepository/repositoryUrl", "http://pkg.sourcepole.ch/kadas/plugins/qgis-repo.xml" ).toString();
  QgsNetworkContentFetcher nf;
  QUrl repositoryUrl( repoUrl );
  QEventLoop e;
  nf.fetchContent( repositoryUrl );
  QObject::connect( &nf, &QgsNetworkContentFetcher::finished, &e, &QEventLoop::quit );
  e.exec();

  QString pluginContent = nf.contentAsString();
  QMap< QString, PluginInfo > pluginMap;

  //Dom document
  QDomDocument xml;
  if ( !xml.setContent( pluginContent ) )
  {
    return pluginMap;
  }

  QDomNodeList pluginNodeList = xml.elementsByTagName( "pyqgis_plugin" );
  for ( int i = 0; i < pluginNodeList.size(); ++i )
  {
    QDomElement pluginElem  = pluginNodeList.at( i ).toElement();
    KadasPluginManager::PluginInfo p;
    p.name = pluginElem.attribute( "name" );
    p.version = pluginElem.attribute( "version" );
    p.description = pluginElem.firstChildElement( "description" ).text();
    p.downloadLink = pluginElem.firstChildElement( "download_url" ).text();
    pluginMap.insert( p.name, p );
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

  QString pluginModule = item->data( 0, Qt::UserRole ).toString();
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
  QString pluginName = sender()->property( "PluginName" ).toString();
  if ( pluginName.isEmpty() )
  {
    return;
  }

  QString downloadPath = mAvailablePlugins.value( pluginName ).downloadLink;
  QString pluginTooltip = mAvailablePlugins.value( pluginName ).description;

  //get module name
  QList<QTreeWidgetItem *> installed = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
  if ( installed.size() < 1 )
  {
    return;
  }
  QString moduleName = installed.at( 0 )->data( 0, Qt::UserRole ).toString();
  updatePlugin( pluginName, moduleName, downloadPath, pluginTooltip );

  QList<QTreeWidgetItem *> available = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
  if ( available.size() > 0 )
  {
    setItemNotUpdateable( available.at( 0 ) );
  }
}

void KadasPluginManager::installButtonClicked()
{
  QString pluginName = sender()->property( "PluginName" ).toString();
  if ( pluginName.isEmpty() )
  {
    return;
  }

  QList<QTreeWidgetItem *> installed = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );

  if ( installed.size() < 1 ) //not yet installed
  {
    QString downloadPath = mAvailablePlugins.value( pluginName ).downloadLink;
    QString pluginTooltip = mAvailablePlugins.value( pluginName ).description;
    if ( downloadPath.isEmpty() )
    {
      return;
    }
    installPlugin( pluginName, downloadPath, pluginTooltip );
  }
  else //plugin installed, remove it
  {
    if ( QMessageBox::question( this, tr( "Remove plugin" ), tr( "Are you sure you want to remove the plugin '%1'?" ).arg( pluginName ) ) == QMessageBox::Yes )
    {
      QString moduleName = installed.at( 0 )->data( 0, Qt::UserRole ).toString();
      uninstallPlugin( pluginName, moduleName );
    }
  }
}

void KadasPluginManager::installPlugin( const QString &pluginName, const  QString &downloadUrl, const QString &pluginTooltip )
{
  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return;
  }

  QString pp = p->homePluginsPath();
  if ( !QDir().mkpath( pp ) )
  {
    kApp->mainWindow()->messageBar()->pushCritical( tr( "Plugin install failed" ), tr( "Error creating plugin directory" ) );
    return;
  }

  //download and unzip in kadasPluginsPath
  QgsNetworkContentFetcher nf;
  QUrl repositoryUrl( downloadUrl );
  QEventLoop e;
  nf.fetchContent( repositoryUrl );
  QObject::connect( &nf, &QgsNetworkContentFetcher::finished, &e, &QEventLoop::quit );
  e.exec();

  if ( nf.reply()->error() != QNetworkReply::NoError )
  {
    kApp->mainWindow()->messageBar()->pushCritical( tr( "Plugin install failed" ), tr( "Error downloading plugin: %1" ).arg( nf.reply()->error() ) );
    return;
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
    QString absoluteFilePath = pp + "/" + fileName;

    if ( moduleName.isEmpty() )
    {
      moduleName = QFileInfo( fileName ).path();;
    }

    if ( fileName.endsWith( "/" ) )
    {
      continue;
    }

    QString zipName = zip.getZipName();
    QuaZipFile zFile( zipName, fileName );
    zFile.setZip( &zip );
    if ( !zFile.open( QIODevice::ReadOnly ) )
    {
//      int errorCode = zFile.getZipError();
      continue;
    }

    QByteArray ba = zFile.readAll();
    zFile.close();
    QDir().mkpath( QFileInfo( absoluteFilePath ).absolutePath() );
    QFile dstFile( absoluteFilePath );
    if ( !dstFile.open( QIODevice::WriteOnly ) )
    {
      continue;
    }

    dstFile.write( ba.data() );
    dstFile.close();
  }

  //insert into mInstalledTreeWidget
  QTreeWidgetItem *installedItem = new QTreeWidgetItem();
  installedItem->setText( 0, pluginName );
  installedItem->setToolTip( 0, pluginTooltip );
  installedItem->setData( 0, Qt::UserRole, moduleName );

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
  QList<QTreeWidgetItem *> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
  if ( availableItem.size() > 0 )
  {
    setItemRemoveable( availableItem.at( 0 ) );
    setItemNotUpdateable( availableItem.at( 0 ) );
  }
}

void KadasPluginManager::uninstallPlugin( const QString &pluginName, const QString &moduleName )
{
  //deactivate first
  KadasPythonIntegration *p = KadasApplication::instance()->pythonIntegration();
  if ( !p )
  {
    return;
  }
  p->unloadPlugin( moduleName );

  //remove folder
  QDir pluginDir( p->homePluginsPath() + "/" + moduleName );
  if ( !pluginDir.removeRecursively() )
  {
    QMessageBox::critical( this, tr( "Plugin deinstallation failed" ), tr( "The deinstallation of the plugin '%1' faild" ).arg( pluginName ) );
    return;
  }

  //remove entry from mInstalledTreeWidget
  QList<QTreeWidgetItem *> installedItem = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
  if ( installedItem.size() > 0 )
  {
    delete mInstalledTreeWidget->takeTopLevelItem( mInstalledTreeWidget->indexOfTopLevelItem( installedItem.at( 0 ) ) );
  }

  //set icon to download in mAvailableTreeWidget
  QList<QTreeWidgetItem *> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
  if ( availableItem.size() > 0 )
  {
    setItemInstallable( availableItem.at( 0 ) );
    availableItem.at( 0 )->setText( 2, "" );
  }
}

void KadasPluginManager::updatePlugin( const QString &pluginName, const QString &moduleName, const  QString &downloadUrl, const QString &pluginTooltip )
{
  uninstallPlugin( pluginName, moduleName );
  installPlugin( pluginName, downloadUrl, pluginTooltip );
}

void KadasPluginManager::setItemInstallable( QTreeWidgetItem *item )
{
  changeItemInstallationState( item, tr( "Install" ) );
}

void KadasPluginManager::setItemRemoveable( QTreeWidgetItem *item )
{
  changeItemInstallationState( item, tr( "Uninstall" ) );
}

void KadasPluginManager::changeItemInstallationState( QTreeWidgetItem *item, const QString &buttonText )
{
  if ( item )
  {
    QPushButton *b = new QPushButton( buttonText );
    b->setProperty( "PluginName", item->text( 0 ) );
    QObject::connect( b, &QPushButton::clicked, this, &KadasPluginManager::installButtonClicked );
    mAvailableTreeWidget->setItemWidget( item, 1, b );
  }
}

void KadasPluginManager::setItemActivatable( QTreeWidgetItem *item )
{
  if ( item )
  {
    item->setCheckState( 0, Qt::Unchecked );
  }
}

void KadasPluginManager::setItemDeactivatable( QTreeWidgetItem *item )
{
  if ( item )
  {
    item->setCheckState( 0, Qt::Checked );
  }
}

void KadasPluginManager::setItemUpdateable( QTreeWidgetItem *item )
{
  if ( item )
  {
    QPushButton *b = new QPushButton( tr( "Update" ) );
    b->setProperty( "PluginName", item->text( 0 ) );
    QObject::connect( b, &QPushButton::clicked, this, &KadasPluginManager::updateButtonClicked );
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
