/***************************************************************************
    kadasguidegridutils.cpp
    ------------------------
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

#include "guidegrid/kadasguidegridutils.h"

QString KadasGuideGridUtils::gridLabel( const QString &firstStart, int offset, bool avoidRepeatingLetters )
{
  bool isInt = false;
  int startNumber = firstStart.toInt( &isInt );
  if ( isInt )
  {
    return QString::number( startNumber + offset );
  }

  // Convert to decimal base
  int startOffset = 0;
  for ( int i = 0; i < firstStart.length(); ++i )
  {
    QChar c = firstStart.at( i );
    startOffset = startOffset * 26 + ( c.toLatin1() - 'A' + 1 );
  }

  int skipped = 0;
  if ( avoidRepeatingLetters )
  {
    for ( int i = 0; i <= offset; i++ )
    {
      QString lbl = KadasGuideGridUtils::gridLabel( firstStart, i );
      QChar firstChar = lbl.at( 0 );
      bool isRepeating = std::all_of( lbl.begin(), lbl.end(), [firstChar]( QChar c ) { return c == firstChar; } );

      if ( lbl.length() != 1 && isRepeating )
      {
        skipped++;
      }
    }
  }

  offset += startOffset + skipped;

  QString label;
  do
  {
    offset -= 1;
    int res = offset % 26;
    label.prepend( QChar( 'A' + res ) );
    offset /= 26;
  } while ( offset > 0 );
  return label;
}
