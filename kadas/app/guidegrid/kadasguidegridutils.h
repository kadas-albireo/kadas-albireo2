/***************************************************************************
    kadasguidegridutils.h
    ----------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASGUIDEGRIDUTILS_H
#define KADASGUIDEGRIDUTILS_H

#include <QString>

/**
 * Helpers for guide grid labeling, used by both KadasGuideGridLayer and KadasGuideGridRenderer.
 */
class KadasGuideGridUtils
{
  public:
    static QString gridLabel( const QString &firstStart, int offset, bool avoidRepeatingLetters = false );
};

#endif // KADASGUIDEGRIDUTILS_H
