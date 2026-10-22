/***************************************************************************
    kadasguidegridutils.cpp
    -----------------------
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

#include <algorithm>

#include "kadas/core/kadasguidegridutils.h"

QString KadasGuideGridUtils::gridLabel( const QString &start, int offset, bool avoidRepeatingLetters )
{
  bool isNumber = false;
  const int startNumber = start.toInt( &isNumber );
  if ( isNumber )
  {
    return QString::number( startNumber + offset );
  }

  const int first = lettersToNumber( start );
  int number = first + offset;
  if ( avoidRepeatingLetters )
  {
    // Shift by the number of repeating labels within [first, number] until the range no longer grows
    const int repeatingBeforeFirst = repeatingLettersCount( first - 1 );
    int previous = 0;
    do
    {
      previous = number;
      number = first + offset + repeatingLettersCount( number ) - repeatingBeforeFirst;
    } while ( number != previous );
  }
  return numberToLetters( number );
}

int KadasGuideGridUtils::lettersToNumber( const QString &letters )
{
  // Bijective base 26: A = 1, Z = 26, AA = 27, ...
  int number = 0;
  for ( const QChar c : letters )
  {
    number = number * 26 + c.toUpper().unicode() - 'A' + 1;
  }
  return number;
}

QString KadasGuideGridUtils::numberToLetters( int number )
{
  QString letters;
  while ( number > 0 )
  {
    --number;
    letters.prepend( QChar( 'A' + number % 26 ) );
    number /= 26;
  }
  return letters;
}

int KadasGuideGridUtils::repeatingLettersCount( int number )
{
  // Repeating labels of n letters are the multiples 1..26 of the n-letter label AA..A (27, 703, 18279, ...)
  int count = 0;
  for ( int repunit = 27; repunit <= number; repunit = repunit * 26 + 1 )
  {
    count += std::min( 26, number / repunit );
  }
  return count;
}
