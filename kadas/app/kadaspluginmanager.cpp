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

#include <QDomDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTreeWidgetItem>
#include <quazip5/quazipfile.h>

#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsnetworkcontentfetcher.h>
#include <qgis/qgssettings.h>

#include <kadas/app/kadasapplication.h>
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

  QStringList installedPlugins = p->pluginList();
  QSet<QString> installedPluginNames;

  QStringList::const_iterator installedIt = installedPlugins.constBegin();
  for ( ; installedIt != installedPlugins.constEnd(); ++installedIt )
  {
    QString pluginName = p->getPluginMetadata( *installedIt, "name" );
    QString pluginDescription = p->getPluginMetadata( *installedIt, "description" );
    installedPluginNames.insert( pluginName );
    QTreeWidgetItem *installedItem = new QTreeWidgetItem();
    installedItem->setText( 0, pluginName );
    installedItem->setToolTip( 0, pluginDescription );
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

  QMap<QString, QPair< QString, QString > >::const_iterator pluginIt = mAvailablePlugins.constBegin();
  for ( ; pluginIt != mAvailablePlugins.constEnd(); ++pluginIt )
  {
    QTreeWidgetItem *availableItem = new QTreeWidgetItem();
    availableItem->setText( 0, pluginIt.key() );
    availableItem->setToolTip( 0, pluginIt->first );
    mAvailableTreeWidget->addTopLevelItem( availableItem );
    if ( installedPluginNames.contains( pluginIt.key() ) )
    {
      setItemRemoveable( availableItem );
    }
    else
    {
      setItemInstallable( availableItem );
    }
  }

  mInstalledTreeWidget->resizeColumnToContents( 0 );
  mAvailableTreeWidget->resizeColumnToContents( 0 );
}

KadasPluginManager::KadasPluginManager(): KadasBottomBar( 0 )
{
}

QMap< QString, QPair< QString, QString > > KadasPluginManager::availablePlugins()
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
  QMap< QString, QPair< QString, QString > > pluginMap;

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
    QString pluginName = pluginElem.attribute( "name" );
    QString pluginDescription = pluginElem.firstChildElement( "description" ).text();
    QString downloadUrl = pluginElem.firstChildElement( "download_url" ).text();
    pluginMap.insert( pluginName, qMakePair( pluginDescription, downloadUrl ) );
  }
  return pluginMap;
}

void KadasPluginManager::on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column )
{
  if ( column != 1 )
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
    if ( p->unloadPlugin( pluginModule ) )
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
    QString downloadPath = mAvailablePlugins.value( pluginName ).second;
    QString pluginTooltip = mAvailablePlugins.value( pluginName ).first;
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

  //download and unzip in kadasPluginsPath
  QgsNetworkContentFetcher nf;
  QUrl repositoryUrl( downloadUrl );
  QEventLoop e;
  nf.fetchContent( repositoryUrl );
  QObject::connect( &nf, &QgsNetworkContentFetcher::finished, &e, &QEventLoop::quit );
  e.exec();

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

    if ( fileName.endsWith( "/" ) )
    {
      if ( moduleName.isEmpty() )
      {
        moduleName = fileName; moduleName.chop( 1 );
      }
      QDir pluginDir( absoluteFilePath );
      pluginDir.mkdir( absoluteFilePath );
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
  }
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
    item->setCheckState( 1, Qt::Unchecked );
  }
}

void KadasPluginManager::setItemDeactivatable( QTreeWidgetItem *item )
{
  if ( item )
  {
    item->setCheckState( 1, Qt::Checked );
  }
}
