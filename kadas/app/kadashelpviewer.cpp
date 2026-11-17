/***************************************************************************
    kadashelpviewer.cpp
    ------------------
    copyright            : (C) 2024 by Damiano Lombardi
    email                : damiano at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDesktopServices>
#include <QDir>
#include <QSettings>
#include <QUrl>

#include <qgis/qgis.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsuserprofilemanager.h>

#include "kadas/core/kadas.h"
#include "kadashelpviewer.h"

KadasHelpViewer::KadasHelpViewer( QObject *parent )
  : QObject( parent )
  , mDocDir( QDir( Kadas::pkgDataPath() ).absoluteFilePath( "docs/html" ) )
  , mHelpFileServer( mDocDir, "127.0.0.1" )
{}

void KadasHelpViewer::showHelp() const
{
  const QString locale = QSettings().value( "locale/userLocale", "en" ).toString();
  QString lang = locale.left( 2 );

  QDir qDir( mDocDir );
  if ( !qDir.exists( lang ) )
  {
    QgsDebugMsgLevel( QString( "Documentation is not available for '%1'. Will be served in english." ).arg( lang ), 1 );
    lang = "en";
  }

  QUrl url( QString( "http://%1:%2/%3/" ).arg( mHelpFileServer.getHost() ).arg( mHelpFileServer.getPort() ).arg( lang ) );
  QDesktopServices::openUrl( url );
}
