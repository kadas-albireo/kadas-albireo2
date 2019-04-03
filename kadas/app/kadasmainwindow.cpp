/***************************************************************************
    kadasmainwindow.cpp
    -----------------
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

#include "kadasmainwindow.h"

KadasMainWindow::KadasMainWindow(QSplashScreen *splash)
{
  KadasWindowBase::setupUi( this );

  QWidget* topWidget = new QWidget();
  KadasTopWidget::setupUi( topWidget );
  setMenuWidget( topWidget );

  QWidget* statusWidget = new QWidget();
  KadasStatusWidget::setupUi( statusWidget );
  statusBar()->addPermanentWidget( statusWidget, 1 );

// TODO
//  QStringList catalogUris = QSettings().value( "/Qgis/geodatacatalogs" ).toString().split( ";;" );
//  foreach ( const QString& catalogUri, catalogUris )
//  {
//    QUrl u = QUrl::fromEncoded( "?" + catalogUri.toLocal8Bit() );
//    QUrlQuery q(u);
//    QString type = q.queryItemValue( "type" );
//    QString url = q.queryItemValue( "url" );
//    if ( type == "geoadmin" )
//    {
//      addProvider( new QgsGeoAdminRestCatalogProvider( url, this ) );
//    }
//    else if ( type == "arcgisrest" )
//    {
//      addProvider( new QgsArcGisRestCatalogProvider( url, this ) );
//    }
//    else if ( type == "vbs" )
//    {
//      addProvider( new QgsVBSCatalogProvider( url, this ) );
//    }
//  }

  // TODO
//  addSearchProvider( new QgsCoordinateSearchProvider( mMapCanvas ) );
//  addSearchProvider( new QgsLocationSearchProvider( mMapCanvas ) );
//  addSearchProvider( new QgsLocalDataSearchProvider( mMapCanvas ) );
//  addSearchProvider( new QgsPinSearchProvider( mMapCanvas ) );
//  addSearchProvider( new QgsRemoteDataSearchProvider( mMapCanvas ) );
//  addSearchProvider( new QgsWorldLocationSearchProvider( mMapCanvas ) );
}

void KadasMainWindow::openProject(const QString& fileName)
{
  // TODO
}
