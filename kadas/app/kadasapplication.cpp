/***************************************************************************
    kadasapplication.h
    ------------------
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

#include <QFile>
#include <QIcon>
#include <QSplashScreen>

#include <qgis/qgsmessagebar.h>
#include <qgis/qgsnetworkaccessmanager.h>

#include <kadas/core/kadas.h>

#include "kadasapplication.h"
#include "kadasmainwindow.h"

KadasApplication *KadasApplication::instance()
{
  return qobject_cast<KadasApplication *>( QCoreApplication::instance() );
}

KadasApplication::KadasApplication(int& argc, char** argv)
  : QApplication(argc, argv)
{
  // Setup application style
  setWindowIcon(QIcon(":/images/icon"));
  QFile styleSheet(":/stylesheet");
  if (styleSheet.open(QIODevice::ReadOnly))
  {
    setStyleSheet(QString::fromLocal8Bit(styleSheet.readAll()));
  }

  // Create / migrate settings
  QSettings settings;

  QFile srcSettings;
  bool settingsEmpty = false;
  if ( QFile( settings.fileName() ).exists() )
  {
    QgsDebugMsg( "Patching settings" );
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_patch.ini" ) );
  }
  else
  {
    QgsDebugMsg( "Copying full settings" );
    settingsEmpty = true;
    srcSettings.setFileName( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "settings_full.ini" ) );
  }
  if ( srcSettings.exists() )
  {
    QSettings newSettings( srcSettings.fileName(), QSettings::IniFormat );
    QString timestamp = settings.value( "timestamp", "0" ).toString();
    QString newtimestamp = newSettings.value( "timestamp" ).toString();
    if ( settingsEmpty || newtimestamp > timestamp )
    {
      settings.setValue( "timestamp", newtimestamp );
      // Merge new settings to old settings
      for ( const QString &group : newSettings.childGroups() )
      {
        newSettings.beginGroup( group );
        settings.beginGroup( group );
        for ( const QString &key : newSettings.childKeys() )
        {
          settings.setValue( key, newSettings.value( key ) );
        }
        newSettings.endGroup();
        settings.endGroup();
      }
    }
  }
  settings.sync();

  // Setup localization
  QString userLocale = settings.value( "locale/userLocale", QLocale::system().name() ).toString();

  QString qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  QTranslator qtTranslator;
  qtTranslator.load("qt_" + userLocale, qtTranslationsPath);
  qtTranslator.load("qtbase_" + userLocale, qtTranslationsPath);
  installTranslator(&qtTranslator);

  QString i18nPath = QDir(Kadas::pkgDataPath()).absoluteFilePath("i18n");
  QTranslator appTranslator;
  appTranslator.load(QString("%1_%2").arg(Kadas::KADAS_RELEASE_NAME), userLocale, i18nPath);
  installTranslator(&appTranslator);

  // Create main window
  QSplashScreen splash(QPixmap(":/images/splash"));
  splash.show();
  mMainWindow = new KadasMainWindow(&splash);

  // Perform online/offline check to select default template
  QString onlineTestUrl = settings.value( "/kadas/onlineTestUrl" ).toString();
  QString projectTemplate;
  if ( !onlineTestUrl.isEmpty() )
  {
    QEventLoop eventLoop;
    QNetworkReply* reply = QgsNetworkAccessManager::instance()->head( QNetworkRequest( onlineTestUrl ) );
    QObject::connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
    eventLoop.exec();

    if ( reply->error() == QNetworkReply::NoError )
    {
      projectTemplate = settings.value( "/kadas/onlineDefaultProject" ).toString();
      settings.setValue( "/kadas/isOffline", false );
    }
    else
    {
      projectTemplate = settings.value( "/kadas/offlineDefaultProject" ).toString();
      settings.setValue( "/kadas/isOffline", true );
    }
    delete reply;
  }

  if(!projectTemplate.isEmpty())
  {
    projectTemplate = QDir(Kadas::projectTemplatesPath()).absoluteFilePath(projectTemplate);
    projectOpen(projectTemplate);
  }

  // TODO: QgsApplication::setMaxThreads( QSettings().value( "/Qgis/max_threads", -1 ).toInt() );

  mMainWindow->show();
  splash.finish(mMainWindow);

  QObject::connect(this, &QApplication::lastWindowClosed, this, &QApplication::quit);
}

void KadasApplication::addDelimitedTextLayer()
{
  // TODO
}

void KadasApplication::addRasterLayer()
{
  // TODO
}

void KadasApplication::addVectorLayer()
{
  // TODO
}

void KadasApplication::addWcsLayer()
{
  // TODO
}

void KadasApplication::addWfsLayer()
{
  // TODO
}

void KadasApplication::addWmsLayer()
{
  // TODO
}

void KadasApplication::exportToGpx()
{
  // TODO
}

void KadasApplication::exportToKml()
{
  // TODO
}

void KadasApplication::importFromGpx()
{
  // TODO
}

void KadasApplication::importFromKml()
{
  // TODO
}

void KadasApplication::paste()
{
  // TODO
}

void KadasApplication::projectOpen( const QString& fileName )
{
  // TODO
}

void KadasApplication::projectSave()
{
  // TODO
}

void KadasApplication::projectSaveAs( const QString& fileName )
{
  // TODO
}

void KadasApplication::saveMapAsImage()
{
  // TODO
}

void KadasApplication::saveMapToClipboard()
{
  // TODO
}

void KadasApplication::showLayerAttributeTable(const QgsMapLayer* layer)
{
  // TODO
}

void KadasApplication::showLayerProperties(const QgsMapLayer* layer)
{
  // TODO
}

void KadasApplication::showLayerInfo(const QgsMapLayer* layer)
{
  // TODO
}

void KadasApplication::zoomFull()
{
  // TODO
}

void KadasApplication::zoomIn()
{
  mMainWindow->mapCanvas()->zoomIn();
}

void KadasApplication::zoomNext()
{
  // TODO
}

void KadasApplication::zoomOut()
{
  mMainWindow->mapCanvas()->zoomOut();
}

void KadasApplication::zoomPrev()
{
  // TODO
}

#if 0
TODO
void KadasMainWindow::addImage()
{
  mMapCanvas->setMapTool( kApp->mapToolPan() ); // Ensure pan tool is active

  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QSet<QString> formats;
  foreach ( const QByteArray& format, QImageReader::supportedImageFormats() )
  {
    formats.insert( QString( "*.%1" ).arg( QString( format ).toLower() ) );
  }
  formats.insert( "*.svg" ); // Ensure svg is present

  QString filter = QString( "Images (%1)" ).arg( QStringList( formats.toList() ).join( " " ) );
  QString filename = QFileDialog::getOpenFileName( this, tr( "Select Image" ), lastDir, filter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  QString errMsg;
  if ( filename.endsWith( ".svg", Qt::CaseInsensitive ) )
  {
    QgsSvgAnnotationItem* item = new QgsSvgAnnotationItem( mapCanvas() );
    item->setFilePath( filename );
    item->setMapPosition( mapCanvas()->extent().center(), mapCanvas()->mapSettings().destinationCrs() );
    QgsAnnotationLayer::getLayer( mapCanvas(), "svgSymbols", tr( "SVG graphics" ) )->addItem( item );
    mapCanvas()->setMapTool( new QgsMapToolEditAnnotation( mapCanvas(), item ) );
  }
  else if ( !QgsGeoImageAnnotationItem::create( mapCanvas(), filename, false, &errMsg ) )
  {
    mInfoBar->pushCritical( tr( "Could not add image" ), errMsg );
  }
}
#endif
