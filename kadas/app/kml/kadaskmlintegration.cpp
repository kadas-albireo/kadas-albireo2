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
}

void KadasKmlIntegration::exportToKml()
{
  KadasKMLExportDialog d( kApp->mainWindow()->mapCanvas()->layers() );
  if ( d.exec() != QDialog::Accepted )
  {
    return;
  }
  kApp->setOverrideCursor( Qt::BusyCursor );
  KadasKMLExport kmlExport;
  if ( kmlExport.exportToFile( d.getFilename(), d.getSelectedLayers(), d.getExportScale() ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML export completed" ), Qgis::Info, 4 );
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML export failed" ), Qgis::Critical, 4 );
  }
  kApp->restoreOverrideCursor();
}

void KadasKmlIntegration::importFromKml()
{
  QStringList filters;
  filters.append( tr( "KMZ File (*.kmz)" ) );
  filters.append( tr( "KML File (*.kml)" ) );

  QString lastDir = QgsSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getOpenFileName( kApp->mainWindow(), tr( "Select KML/KMZ File" ), lastDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QgsSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );

  QString errMsg;
  kApp->setOverrideCursor( Qt::BusyCursor );
  if ( KadasKMLImport().importFile( filename, errMsg ) )
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML import completed" ), Qgis::Info, 4 );
  }
  else
  {
    kApp->mainWindow()->messageBar()->pushMessage( tr( "KML import failed" ), errMsg, Qgis::Critical, 4 );
  }
  kApp->restoreOverrideCursor();
}
