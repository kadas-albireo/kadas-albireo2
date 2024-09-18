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
#include <qgis/qgsuserprofilemanager.h>

#include <kadas/core/kadas.h>
#include <kadashelpviewer.h>

KadasHelpViewer::KadasHelpViewer( QObject *parent )
    : QObject( parent )
    , mHelpFileServer(QString(), "127.0.0.1")
{
  const QString docdir = QDir( Kadas::pkgDataPath() ).absoluteFilePath( "docs/html" );
  mHelpFileServer.setFilesTopDir( docdir );
}

void KadasHelpViewer::showHelp() const
{
  const QString locale = QSettings().value("locale/userLocale", "en").toString();
  QString lang = locale.left(2);

  QDir qDir(mHelpFileServer.getFilesTopDir());
  if(!qDir.exists(lang))
      lang = "en";

  QUrl url(QString("http:///%1:%2/%3/").arg(mHelpFileServer.getHost())
                                       .arg(mHelpFileServer.getPort())
                                       .arg(lang));
  QDesktopServices::openUrl(url);
}
