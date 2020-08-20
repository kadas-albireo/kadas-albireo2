/***************************************************************************
    kadaskmlintegration.cpp
    -----------------------
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

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QKeySequence>
#include <QShortcut>

#include <qgis/qgsmessagebar.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kml/kadaskmlexport.h>
#include <kadas/app/kml/kadaskmlexportdialog.h>
#include <kadas/app/kml/kadaskmlimport.h>
#include <kadas/app/kml/kadaskmlintegration.h>



KadasKmlIntegration::KadasKmlIntegration( QToolButton *kmlButton, QObject *parent )
  : mKMLButton( kmlButton )
{
  QMenu *kmlMenu = new QMenu();

  QAction *actionExportKml = new QAction( tr( "KML Export" ), kmlMenu );
  connect( actionExportKml, &QAction::triggered, this, &KadasKmlIntegration::exportToKml );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_E, Qt::CTRL + Qt::Key_K ), kApp->mainWindow() ), &QShortcut::activated, actionExportKml, &QAction::trigger );
  kmlMenu->addAction( actionExportKml );

  QAction *actionImportKml = new QAction( tr( "KML Import" ), kmlMenu );
  connect( actionImportKml, &QAction::triggered, this, &KadasKmlIntegration::importFromKml );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_I, Qt::CTRL + Qt::Key_K ), kApp->mainWindow() ), &QShortcut::activated, actionImportKml, &QAction::trigger );
  kmlMenu->addAction( actionImportKml );

  mKMLButton->setIcon( QIcon( ":/kadas/icons/kml" ) );
  mKMLButton->setMenu( kmlMenu );
  mKMLButton->setPopupMode( QToolButton::InstantPopup );

  kApp->mainWindow()->addCustomDropHandler( &mDropHandler );
}

KadasKmlIntegration::~KadasKmlIntegration()
{
  kApp->mainWindow()->removeCustomDropHandler( &mDropHandler );
}

void KadasKmlIntegration::exportToKml()
{
  kApp->unsetMapTool();

  KadasKMLExportDialog d( kApp->mainWindow()->mapCanvas()->layers(), kApp->mainWindow() );
  d.show();
  QEventLoop loop;
  connect( &d, &QDialog::accepted, &loop, &QEventLoop::quit );
  connect( &d, &QDialog::rejected, &loop, &QEventLoop::quit );
  loop.exec();
  if ( d.result() != QDialog::Accepted )
  {
    return;
  }
  kApp->setOverrideCursor( Qt::BusyCursor );
  KadasKMLExport kmlExport;
  if ( kmlExport.exportToFile( d.getFilename(), d.getSelectedLayers(), d.getExportScale(), kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs(), d.getFilterRect() ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML export completed" ), Qgis::Info, 5 );
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML export failed" ), Qgis::Critical, 5 );
  }
  kApp->restoreOverrideCursor();
}

void KadasKmlIntegration::importFromKml()
{
  kApp->unsetMapTool();

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getOpenFileName( kApp->mainWindow(), tr( "Select KML/KMZ File" ), lastDir, tr( "KML Files (*.kml *.kmz)" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );

  QString errMsg;
  kApp->setOverrideCursor( Qt::BusyCursor );
  if ( KadasKMLImport().importFile( filename, errMsg ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML import completed" ), Qgis::Info, 5 );
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML import failed" ), errMsg, Qgis::Critical, 5 );
  }
  kApp->restoreOverrideCursor();
}

bool KadasKmlDropHandler::canHandleMimeData( const QMimeData *data )
{
  for ( const QUrl &url : data->urls() )
  {
    if ( url.toLocalFile().endsWith( ".kmz", Qt::CaseInsensitive ) || url.toLocalFile().endsWith( ".kml", Qt::CaseInsensitive ) )
    {
      return true;
    }
  }
  return false;
}

bool KadasKmlDropHandler::handleMimeDataV2( const QMimeData *data )
{
  int handled = 0;
  for ( const QUrl &url : data->urls() )
  {
    QString path = url.toLocalFile();
    QStringList errors;
    if ( path.endsWith( ".kmz", Qt::CaseInsensitive ) || path.endsWith( ".kml", Qt::CaseInsensitive ) )
    {
      ++handled;
      QString errMsg;
      if ( !KadasKMLImport().importFile( path, errMsg ) )
      {
        errors.append( QString( "%1: %2" ).arg( QFileInfo( path ).fileName() ).arg( errMsg ) );
      }
    }
    if ( handled > 0 )
    {
      if ( errors.isEmpty() )
      {
        kApp->mainWindow()->messageBar()->pushMessage( tr( "KML import completed" ), Qgis::Info, 5 );
      }
      else
      {
        QMessageBox::critical( kApp->mainWindow(), tr( "KML import failed" ), tr( "The following files could not be imported:\n%1" ).arg( errors.join( "\n" ) ) );
      }
    }
  }
  return handled > 0;
}
