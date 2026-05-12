/***************************************************************************
  kadasportaltokenfetcher.cpp
  ---------------------------
  Date                 : May 2026
  Copyright            : (C) 2026 by Sourcepole AG
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kadasportaltokenfetcher.h"

#include <QUrl>
#include <QUrlQuery>

#ifdef Q_OS_WIN

#include <QString>
#include <QStringList>

// Windows headers must be included before winhttp.h
#include <windows.h>
#include <winhttp.h>

namespace
{
  QString winHttpErrorString( DWORD code )
  {
    LPWSTR buffer = nullptr;
    const DWORD len
      = FormatMessageW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandleW( L"winhttp.dll" ), code, 0, reinterpret_cast<LPWSTR>( &buffer ), 0, nullptr );
    QString result;
    if ( len && buffer )
    {
      result = QString::fromWCharArray( buffer, static_cast<int>( len ) ).trimmed();
    }
    if ( buffer )
      LocalFree( buffer );
    if ( result.isEmpty() )
      result = QStringLiteral( "WinHTTP error 0x%1" ).arg( code, 0, 16 );
    return result;
  }

  KadasPortalTokenFetchResult fetchOnWindows( const QUrl &url, int timeoutMs )
  {
    KadasPortalTokenFetchResult result;

    if ( !url.isValid() || url.host().isEmpty() )
    {
      result.errorMessage = QStringLiteral( "Invalid token URL" );
      return result;
    }

    const bool isHttps = url.scheme().compare( QStringLiteral( "https" ), Qt::CaseInsensitive ) == 0;
    const std::wstring host = url.host().toStdWString();
    const INTERNET_PORT port = url.port() > 0 ? static_cast<INTERNET_PORT>( url.port() ) : ( isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT );

    QString pathAndQuery = url.path( QUrl::FullyEncoded );
    if ( pathAndQuery.isEmpty() )
      pathAndQuery = QStringLiteral( "/" );
    if ( url.hasQuery() )
      pathAndQuery += QLatin1Char( '?' ) + url.query( QUrl::FullyEncoded );
    const std::wstring objectName = pathAndQuery.toStdWString();

    HINTERNET hSession = WinHttpOpen( L"Kadas/SSO", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
    if ( !hSession )
    {
      result.errorMessage = QStringLiteral( "WinHttpOpen failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
      return result;
    }

    // Per-call timeouts: resolve, connect, send, receive (ms).
    WinHttpSetTimeouts( hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs );

    HINTERNET hConnect = WinHttpConnect( hSession, host.c_str(), port, 0 );
    if ( !hConnect )
    {
      result.errorMessage = QStringLiteral( "WinHttpConnect failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
      WinHttpCloseHandle( hSession );
      return result;
    }

    const DWORD requestFlags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest( hConnect, L"GET", objectName.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, requestFlags );
    if ( !hRequest )
    {
      result.errorMessage = QStringLiteral( "WinHttpOpenRequest failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
      WinHttpCloseHandle( hConnect );
      WinHttpCloseHandle( hSession );
      return result;
    }

    // Allow silent SSO with the logged-in Windows user's credentials, even on
    // non-intranet zones (the typical Portal hostname is a fully-qualified domain
    // outside Windows' "Intranet" zone heuristic).
    DWORD autologon = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
    WinHttpSetOption( hRequest, WINHTTP_OPTION_AUTOLOGON_POLICY, &autologon, sizeof( autologon ) );

    // Build header list: Referer (from query) is the only one we set explicitly.
    QString headers;
    {
      const QUrlQuery query( url );
      const QString referer = query.queryItemValue( QStringLiteral( "referer" ), QUrl::FullyDecoded );
      if ( !referer.isEmpty() )
        headers = QStringLiteral( "Referer: %1\r\n" ).arg( referer );
    }
    const std::wstring headersW = headers.toStdWString();
    LPCWSTR headersPtr = headersW.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headersW.c_str();
    const DWORD headersLen = headersW.empty() ? 0 : static_cast<DWORD>( -1L );

    auto sendAndReceive = [&]() -> bool {
      if ( !WinHttpSendRequest( hRequest, headersPtr, headersLen, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 ) )
        return false;
      if ( !WinHttpReceiveResponse( hRequest, nullptr ) )
        return false;
      return true;
    };

    if ( !sendAndReceive() )
    {
      result.errorMessage = QStringLiteral( "WinHttpSendRequest failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
      WinHttpCloseHandle( hRequest );
      WinHttpCloseHandle( hConnect );
      WinHttpCloseHandle( hSession );
      return result;
    }

    // Read status code.
    DWORD statusCode = 0;
    DWORD statusLen = sizeof( statusCode );
    WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX );

    // If the server challenged us, supply default credentials and try again.
    // WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW already permits this for the first
    // round-trip on most schemes, but Negotiate against an FQDN sometimes still
    // requires an explicit WinHttpSetCredentials call.
    if ( statusCode == HTTP_STATUS_DENIED )
    {
      DWORD supportedSchemes = 0;
      DWORD firstScheme = 0;
      DWORD authTarget = 0;
      if ( WinHttpQueryAuthSchemes( hRequest, &supportedSchemes, &firstScheme, &authTarget ) )
      {
        DWORD scheme = 0;
        if ( supportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE )
          scheme = WINHTTP_AUTH_SCHEME_NEGOTIATE;
        else if ( supportedSchemes & WINHTTP_AUTH_SCHEME_NTLM )
          scheme = WINHTTP_AUTH_SCHEME_NTLM;

        if ( scheme )
        {
          // nullptr username + password = use the current user's logged-in credentials
          if ( WinHttpSetCredentials( hRequest, authTarget, scheme, nullptr, nullptr, nullptr ) )
          {
            if ( sendAndReceive() )
            {
              statusLen = sizeof( statusCode );
              WinHttpQueryHeaders( hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX );
            }
          }
        }
      }
    }

    result.httpStatus = static_cast<int>( statusCode );

    // Read body regardless of status (server may include error details in body).
    QByteArray body;
    for ( ;; )
    {
      DWORD available = 0;
      if ( !WinHttpQueryDataAvailable( hRequest, &available ) )
      {
        result.errorMessage = QStringLiteral( "WinHttpQueryDataAvailable failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );
        return result;
      }
      if ( available == 0 )
        break;

      QByteArray chunk;
      chunk.resize( static_cast<int>( available ) );
      DWORD read = 0;
      if ( !WinHttpReadData( hRequest, chunk.data(), available, &read ) )
      {
        result.errorMessage = QStringLiteral( "WinHttpReadData failed: %1" ).arg( winHttpErrorString( GetLastError() ) );
        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );
        return result;
      }
      chunk.resize( static_cast<int>( read ) );
      body.append( chunk );
    }

    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hConnect );
    WinHttpCloseHandle( hSession );

    result.body = body;
    if ( statusCode >= 200 && statusCode < 300 )
    {
      result.ok = true;
    }
    else
    {
      result.errorMessage = QStringLiteral( "HTTP %1" ).arg( statusCode );
    }
    return result;
  }
} // namespace

#else // !Q_OS_WIN

#include <QNetworkRequest>
#include <QTimer>

#include <qgis/qgsfeedback.h>
#include <qgis/qgsnetworkaccessmanager.h>

namespace
{
  KadasPortalTokenFetchResult fetchWithQt( const QUrl &url, int timeoutMs )
  {
    KadasPortalTokenFetchResult result;

    QNetworkRequest req( url );
    const QUrlQuery query( url );
    const QString referer = query.queryItemValue( QStringLiteral( "referer" ), QUrl::FullyDecoded );
    if ( !referer.isEmpty() )
      req.setRawHeader( QByteArrayLiteral( "Referer" ), referer.toUtf8() );

    QgsFeedback feedback;
    QTimer::singleShot( timeoutMs, &feedback, [&feedback]() { feedback.cancel(); } );

    QgsNetworkReplyContent content = QgsNetworkAccessManager::instance()->blockingGet( req, QString(), false, &feedback );
    result.httpStatus = content.attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    if ( content.error() == QNetworkReply::NoError )
    {
      result.ok = true;
      result.body = content.content();
    }
    else
    {
      result.errorMessage = content.errorString();
      result.body = content.content();
    }
    return result;
  }
} // namespace

#endif // Q_OS_WIN

KadasPortalTokenFetchResult kadasFetchPortalTokenWithSso( const QUrl &url, int timeoutMs )
{
#ifdef Q_OS_WIN
  return fetchOnWindows( url, timeoutMs );
#else
  return fetchWithQt( url, timeoutMs );
#endif
}
