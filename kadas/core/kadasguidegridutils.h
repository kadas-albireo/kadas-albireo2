/***************************************************************************
    kadasguidegridutils.h
    ---------------------
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

#include "kadas/core/kadas_core.h"

class KADAS_CORE_EXPORT KadasGuideGridUtils
{
  public:
    /**
     * Returns the label of the guide grid row or column at \a offset from the first one, labeled \a start.
     * \a start is either a number (1, 20, ...) or letters (A, AA, BA, ...), letters run as A..Z, AA..AZ, BA..ZZ, AAA, ...
     * If \a avoidRepeatingLetters is set, letter labels made of a single repeated letter (AA, BB, AAA, ...) are skipped.
     */
    static QString gridLabel( const QString &start, int offset, bool avoidRepeatingLetters = false );

  private:
    static int lettersToNumber( const QString &letters );
    static QString numberToLetters( int number );
    static int repeatingLettersCount( int number );
};

#endif // KADASGUIDEGRIDUTILS_H
