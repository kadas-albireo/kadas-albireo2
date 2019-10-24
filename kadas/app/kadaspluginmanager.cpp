#include "kadaspluginmanager.h"
#include "kadasapplication.h"
#include "kadaspythonintegration.h"
#include <qgis/qgsnetworkaccessmanager.h>
#include <qgis/qgsnetworkcontentfetcher.h>
#include <QDomDocument>
#include <QMessageBox>
#include <QSet>
#include <QTreeWidgetItem>
#include <quazip5/quazipfile.h>

KadasPluginManager::KadasPluginManager( QgsMapCanvas* canvas ): KadasBottomBar( canvas )
{
    setupUi( this );

    KadasPythonIntegration* p = KadasApplication::instance()->pythonIntegration();
    if( !p )
    {
        return;
    }

    mAvailablePlugins = availablePlugins();

    QStringList installedPlugins = p->pluginList();
    QSet<QString> installedPluginNames;

    QStringList::const_iterator installedIt = installedPlugins.constBegin();
    for(; installedIt != installedPlugins.constEnd(); ++installedIt )
    {
        QString pluginName = p->getPluginMetadata( *installedIt, "name");
        installedPluginNames.insert( pluginName );
        QTreeWidgetItem* installedItem = new QTreeWidgetItem();
        installedItem->setText( 0, pluginName );
        installedItem->setData( 0, Qt::UserRole, *installedIt );
        if( p->isPluginEnabled( *installedIt ) )
        {
            installedItem->setIcon( 1, QIcon( ":/kadas/icons/remove" ) );
        }
        else
        {
            installedItem->setIcon( 1, QIcon( ":/kadas/icons/add" ) );
        }
        mInstalledTreeWidget->addTopLevelItem( installedItem );
    }

    QMap<QString, QString>::const_iterator pluginIt = mAvailablePlugins.constBegin();
    for(; pluginIt != mAvailablePlugins.constEnd(); ++pluginIt )
    {
        QTreeWidgetItem* availableItem = new QTreeWidgetItem();
        availableItem->setText( 0, pluginIt.key() );
        if( installedPluginNames.contains( pluginIt.key() ) )
        {
            availableItem->setIcon( 1, QIcon( ":/kadas/icons/remove" ) );
        }
        else
        {
            availableItem->setIcon( 1, QIcon( ":/kadas/icons/download" ) );
        }
        mAvailableTreeWidget->addTopLevelItem( availableItem );
    }

    mInstalledTreeWidget->resizeColumnToContents( 0 );
    mAvailableTreeWidget->resizeColumnToContents( 0 );
}

KadasPluginManager::KadasPluginManager(): KadasBottomBar( 0 )
{
}

QMap< QString, QString > KadasPluginManager::availablePlugins()
{
    QgsNetworkContentFetcher nf;
    QUrl repositoryUrl( "http://localhost/kadas_plugin_repository/qgis-repo.xml" );
    QEventLoop e;
    nf.fetchContent( repositoryUrl );
    QObject::connect( &nf, &QgsNetworkContentFetcher::finished, &e, &QEventLoop::quit );
    e.exec();

    QString pluginContent = nf.contentAsString();
    QMap< QString, QString > pluginMap;

    //Dom document
    QDomDocument xml;
    if( !xml.setContent( pluginContent ) )
    {
        qWarning( "************************xml not valid******************" );
        return pluginMap;
    }

    QDomNodeList pluginNodeList = xml.elementsByTagName( "pyqgis_plugin" );
    for( int i = 0; i < pluginNodeList.size(); ++i )
    {
        QDomElement pluginElem  = pluginNodeList.at( i ).toElement();
        pluginMap.insert( pluginElem.attribute( "name" ), pluginElem.firstChildElement( "download_url" ).text() );
    }
    return pluginMap;
}

void KadasPluginManager::on_mInstalledTreeWidget_itemClicked( QTreeWidgetItem *item, int column )
{
    if( column != 1 )
    {
        return;
    }

    KadasPythonIntegration* p = KadasApplication::instance()->pythonIntegration();
    if( !p )
    {
        return;
    }

    QString pluginModule = item->data( 0, Qt::UserRole ).toString();
    if( p->isPluginLoaded( pluginModule ) )
    {
        if( p->unloadPlugin( pluginModule ) )
        {
            item->setIcon( 1, QIcon( ":/kadas/icons/add" ) );
        }
    }
    else
    {
        if( p->loadPlugin( pluginModule ) )
        {
            item->setIcon( 1, QIcon( ":/kadas/icons/remove" ) );
        }
    }
}

void KadasPluginManager::on_mAvailableTreeWidget_itemClicked( QTreeWidgetItem *item, int column )
{
    if( column == 1 )
    {
        QString pluginName = item->text( 0 );
        QList<QTreeWidgetItem *> installed = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );

        if( installed.size() < 1 ) //not yet installed
        {
            QString downloadPath = mAvailablePlugins.value( pluginName );
            if( downloadPath.isEmpty() )
            {
                return;
            }
            installPlugin( pluginName, downloadPath );
        }
        else //plugin installed, remove it
        {
            if( QMessageBox::question( this, tr( "Remove plugin" ), tr("Are you sure you want to remove the plugin '%1'?").arg( pluginName ) ) == QMessageBox::Yes )
            {
                QString moduleName = installed.at( 0 )->data( 0, Qt::UserRole ).toString();
                uninstallPlugin( pluginName, moduleName );
            }
        }
    }
}

void KadasPluginManager::installPlugin( const QString& pluginName, const  QString& downloadUrl ) const
{
    KadasPythonIntegration* p = KadasApplication::instance()->pythonIntegration();
    if( !p )
    {
        return;
    }

    QString debug = "Installing plugin " + pluginName + "  " + downloadUrl;
    qWarning( debug.toLocal8Bit().data() );

    QString pp = p->homePluginsPath();
    qWarning( pp.toLocal8Bit().data() );

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
    zip.open(QuaZip::mdUnzip);
    QuaZipFile file(&zip);
    QString moduleName;

    for(bool f=zip.goToFirstFile(); f; f=zip.goToNextFile() )
    {
        QString fileName = zip.getCurrentFileName();
        QString absoluteFilePath = pp + "/" + fileName;

        if( fileName.endsWith( "/" ) )
        {
            if( moduleName.isEmpty() )
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
        if( !zFile.open( QIODevice::ReadOnly ) )
        {
            int errorCode = zFile.getZipError();
            continue;
        }

        QByteArray ba = zFile.readAll();
        zFile.close();
        QFile dstFile( absoluteFilePath );
        if( !dstFile.open( QIODevice::WriteOnly )  )
        {
            continue;
        }

        dstFile.write( ba.data() );
        dstFile.close();
    }

    //insert into mInstalledTreeWidget
    QTreeWidgetItem* installedItem = new QTreeWidgetItem();
    installedItem->setText( 0, pluginName );
    installedItem->setData( 0, Qt::UserRole, moduleName );

    p->pluginList();
    if( p->loadPlugin( moduleName ) )
    {
       installedItem->setIcon( 1, QIcon( ":/kadas/icons/remove" ) );
    }
    else
    {
        installedItem->setIcon( 1, QIcon( ":/kadas/icons/add" ) );
    }
    mInstalledTreeWidget->addTopLevelItem( installedItem );

    //change icon in mAvailableTreeWidget
    QList<QTreeWidgetItem*> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
    if( availableItem.size() > 0 )
    {
        availableItem.at( 0 )->setIcon( 1, QIcon( ":/kadas/icons/remove" ) );
    }
}

void KadasPluginManager::uninstallPlugin( const QString& pluginName, const QString& moduleName )
{
    //deactivate first
    KadasPythonIntegration* p = KadasApplication::instance()->pythonIntegration();
    if( !p )
    {
        return;
    }
    p->unloadPlugin( moduleName );

    //remove folder
    QDir pluginDir( p->homePluginsPath() + "/" + moduleName );
    if( !pluginDir.removeRecursively() )
    {
        QMessageBox::critical( this, tr( "Plugin deinstallation failed" ), tr( "The deinstallation of the plugin '%1' faild" ).arg( pluginName ) );
        return;
    }

    //remove entry from mInstalledTreeWidget
    QList<QTreeWidgetItem*> installedItem = mInstalledTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
    if( installedItem.size() > 0 )
    {
        delete mInstalledTreeWidget->takeTopLevelItem( mInstalledTreeWidget->indexOfTopLevelItem( installedItem.at( 0 ) ) );
    }

    //set icon to download in mAvailableTreeWidget
    QList<QTreeWidgetItem*> availableItem = mAvailableTreeWidget->findItems( pluginName, Qt::MatchExactly, 0 );
    if( availableItem.size() > 0 )
    {
        availableItem.at( 0 )->setIcon( 1, QIcon( ":/kadas/icons/download" ) );
    }
}

void KadasPluginManager::activatePlugin( const QString& pluginName ) const
{
    //KadasPythonIntegration::loadPlugin()
    //todo...
}
void KadasPluginManager::deactivatePlugin( const QString& pluginName ) const
{
    //todo...
}
