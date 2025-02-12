/***************************************************************************
  kadasfeedback.h
  ----------------
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

#ifndef KADASFEEDBACK_H
#define KADASFEEDBACK_H

class QgsSettingsEntryString;

class KadasFeedback
{
  public:
    static const QgsSettingsEntryString *settingsPortalFeedbackUrl;

    static bool isConfigured();
    static void show();

  private:
    KadasFeedback() {}
};

#endif // KADASFEEDBACK_H
