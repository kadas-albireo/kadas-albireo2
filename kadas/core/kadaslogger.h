/***************************************************************************
    kadaslogger.h
    -------------
  Date                 : November 2026
  Copyright            : (C) 2026 by Damiano Lombardi
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

#ifndef KADASLOGGER_H
#define KADASLOGGER_H

#include <QFileInfo>
#include <QString>

#include <qgis/qgslogger.h>
#include <qgis/qgsmessagelog.h>

/**
 * Debug logging which, unlike QgsDebugMsgLevel, also shows up in the "KADAS" tab of the message log panel.
 * Messages up to the level set with the QGIS_DEBUG environment variable (default 1, 0 = errors only)
 * are logged, and also forwarded to QgsLogger (debug output, or QGIS_LOG_FILE if set).
 */
class KadasLogger
{
  public:
    static inline const QString LOG_TAG = QStringLiteral( "KADAS" );

    static void log( const QString &msg, int level, const char *file, const char *function, int line )
    {
      const QString location = QStringLiteral( "%1:%2 (%3)" ).arg( QFileInfo( QString::fromUtf8( file ) ).fileName() ).arg( line ).arg( QString::fromUtf8( function ) );
      // QgsMessageLog also forwards the message to QgsLogger
      QgsMessageLog::logMessage( QStringLiteral( "%1 %2" ).arg( location, msg ), LOG_TAG, level == 0 ? Qgis::MessageLevel::Warning : Qgis::MessageLevel::Info, false, file, function, line );
    }
};

#define KadasDebugMsgLevel( str, level )                                           \
  do                                                                               \
  {                                                                                \
    if ( ( level ) <= QgsLogger::debugLevel() )                                    \
    {                                                                              \
      KadasLogger::log( QString( str ), ( level ), __FILE__, __func__, __LINE__ ); \
    }                                                                              \
  } while ( false )

#define KadasDebugError( str ) KadasLogger::log( QString( str ), 0, __FILE__, __func__, __LINE__ )

#endif // KADASLOGGER_H
