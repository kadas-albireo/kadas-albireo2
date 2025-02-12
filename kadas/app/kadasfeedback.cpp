/***************************************************************************
  kadasfeedback.cpp
  -----------------
  Date                 : February 2025
  Copyright            : (C) 2025 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kadasfeedback.h"

#include "kadas/core/kadassettingstree.h"

#include <qgssettingsentryimpl.h>

#include <QDesktopServices>
#include <QUrl>

const QgsSettingsEntryString *KadasFeedback::settingsPortalFeedbackUrl = new QgsSettingsEntryString( QStringLiteral( "feedback-url" ), KadasSettingsTree::sTreePortal, QString(), QStringLiteral( "URL to the feedback page." ) );

bool KadasFeedback::isConfigured()
{
  return !settingsPortalFeedbackUrl->value().isEmpty();
}

void KadasFeedback::show()
{
  QDesktopServices::openUrl( QUrl( settingsPortalFeedbackUrl->value() ) );
}
