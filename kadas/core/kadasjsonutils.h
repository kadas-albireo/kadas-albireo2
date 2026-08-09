/***************************************************************************
    kadasjsonutils.h
    ----------------
    copyright            : (C) 2026 by Damiano Lombardi
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

#ifndef KADASJSONUTILS_H
#define KADASJSONUTILS_H

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>

class KadasJsonUtils
{
  public:
    static QString jsonPathToString( const QJsonDocument &document, const QString &jsonPath );
    static QString jsonPathToString( const QJsonValue &value, const QString &jsonPath );
};

inline QString KadasJsonUtils::jsonPathToString( const QJsonDocument &document, const QString &jsonPath )
{
  return jsonPathToString( document.isArray() ? QJsonValue( document.array() ) : QJsonValue( document.object() ), jsonPath );
}

inline QString KadasJsonUtils::jsonPathToString( const QJsonValue &value, const QString &jsonPath )
{
  if ( jsonPath.isEmpty() || !jsonPath.startsWith( QLatin1Char( '$' ) ) )
    return QString();

  QJsonValue current = value;
  int pos = 1;

  auto fail = []() -> QString { return QString(); };

  while ( pos < jsonPath.size() )
  {
    const QChar ch = jsonPath.at( pos );
    if ( ch == QLatin1Char( '.' ) )
    {
      ++pos;
      const int keyStart = pos;
      while ( pos < jsonPath.size() )
      {
        const QChar keyChar = jsonPath.at( pos );
        if ( keyChar == QLatin1Char( '.' ) || keyChar == QLatin1Char( '[' ) )
          break;
        ++pos;
      }
      if ( pos == keyStart || !current.isObject() )
        return fail();

      current = current.toObject().value( jsonPath.mid( keyStart, pos - keyStart ) );
      if ( current.isUndefined() )
        return fail();
    }
    else if ( ch == QLatin1Char( '[' ) )
    {
      ++pos;
      const int indexStart = pos;
      while ( pos < jsonPath.size() && jsonPath.at( pos ).isDigit() )
        ++pos;
      if ( pos == indexStart || pos >= jsonPath.size() || jsonPath.at( pos ) != QLatin1Char( ']' ) || !current.isArray() )
        return fail();

      bool ok = false;
      const int index = jsonPath.mid( indexStart, pos - indexStart ).toInt( &ok );
      if ( !ok )
        return fail();

      const QJsonArray array = current.toArray();
      if ( index < 0 || index >= array.size() )
        return fail();

      current = array.at( index );
      ++pos;
    }
    else
    {
      return fail();
    }
  }

  return current.isString() ? current.toString() : QString();
}

#endif // KADASJSONUTILS_H