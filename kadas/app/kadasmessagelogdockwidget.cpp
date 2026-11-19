/***************************************************************************
    kadasmessagelogdockwidget.cpp
    -----------------------------
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

#include <qgis/qgsapplication.h>
#include <qgis/qgsauthmanager.h>
#include <qgis/qgsmessagelogviewer.h>

#include "kadasmessagelogdockwidget.h"

KadasMessageLogDockWidget::KadasMessageLogDockWidget( QWidget *parent )
  : QDockWidget( tr( "Log Messages" ), parent )
{
  setObjectName( QStringLiteral( "MessageLog" ) );

  mMessageLogViewer = new QgsMessageLogViewer( this );
  mMessageLogViewer->setWindowFlags( Qt::Widget );
  setWidget( mMessageLogViewer );

  connect( QgsApplication::authManager(), &QgsAuthManager::messageLog, mMessageLogViewer, [this]( const QString &message, const QString &tag, Qgis::MessageLevel level ) {
    mMessageLogViewer->logMessage( message, tag, level );
  } );

  toggleViewAction()->setShortcut( QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_L ) );
}
