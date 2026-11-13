/***************************************************************************
    kadasfileserver.cpp
    -------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <memory>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <qgis/qgslogger.h>

#include "kadas/core/kadasfileserver.h"

KadasFileServer::KadasFileServer( const QString &topdir, const QString &host, int port )
  : mBindHost( host )
  , mBindPort( port )
  , mTopdir( topdir )
{
  setObjectName( "KadasFileServer" );
  start();
  // Wait until the worker thread is listening, so that host and port are valid
  mStarted.acquire();
}

KadasFileServer::~KadasFileServer()
{
  quit();
  wait();
}

void KadasFileServer::run()
{
  // Created here, so that the server and all its sockets live in the worker thread
  QTcpServer server;
  if ( !server.listen( QHostAddress( mBindHost ), mBindPort ) )
  {
    QgsDebugMsgLevel( QString( "KadasFileServer failed to listen: %1" ).arg( server.errorString() ), 1 );
  }
  mHost = server.serverAddress().toString();
  mPort = server.serverPort();
  QgsDebugMsgLevel( QString( "KadasFileServer running on %1:%2" ).arg( mHost ).arg( mPort ), 2 );
  mStarted.release();

  // The server is the context object, so the slot runs in the worker thread
  connect( &server, &QTcpServer::newConnection, &server, [this, &server] { handleConnections( &server ); } );
  exec();
}

QByteArray KadasFileServer::genHeaders( int code, qint64 contentLength, bool keepAlive, const QString &filePath )
{
  QByteArray h;
  if ( code == 200 )
  {
    h = "HTTP/1.1 200 OK\r\n";
  }
  else if ( code == 404 )
  {
    h = "HTTP/1.1 404 Not Found\r\n";
  }
  else if ( code == 405 )
  {
    h = "HTTP/1.1 405 Method Not Allowed\r\n";
  }
  else if ( code == 431 )
  {
    h = "HTTP/1.1 431 Request Header Fields Too Large\r\n";
  }

  QByteArray current_date = QDateTime::currentDateTime().toString( Qt::RFC2822Date ).toLocal8Bit();

  h += "Date: " + current_date + "\r\n";
  if ( keepAlive )
  {
    h += "Connection: keep-alive\r\n";
    h += "Keep-Alive: timeout=" + QByteArray::number( IDLE_TIMEOUT_MS / 1000 ) + "\r\n";
  }
  else
  {
    h += "Connection: close\r\n";
  }
  h += "Content-Length: " + QByteArray::number( contentLength ) + "\r\n";

  if ( code == 200 && !filePath.isEmpty() )
  {
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile( filePath, QMimeDatabase::MatchExtension );
    h += "Content-Type: " + mimeType.name().toUtf8() + "\r\n";
  }

  h += "\r\n";
  return h;
}

void KadasFileServer::handleConnections( QTcpServer *server )
{
  while ( server->hasPendingConnections() )
  {
    QTcpSocket *socket = server->nextPendingConnection();
    connect( socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater );

    // Drop connections which stay idle, the timer is restarted whenever data arrives
    QTimer *idleTimer = new QTimer( socket );
    idleTimer->setSingleShot( true );
    connect( idleTimer, &QTimer::timeout, socket, &QTcpSocket::close );
    idleTimer->start( IDLE_TIMEOUT_MS );

    // TCP is a byte stream: a request may arrive in several chunks, and several
    // requests may arrive in one chunk (keep-alive). Accumulate data and process
    // every request whose header section ("\r\n\r\n") is complete.
    auto buffer = std::make_shared<QByteArray>();
    auto onReadyRead = [this, socket, idleTimer, buffer] {
      idleTimer->start( IDLE_TIMEOUT_MS );
      buffer->append( socket->readAll() );
      int end;
      while ( socket->state() == QAbstractSocket::ConnectedState && ( end = buffer->indexOf( "\r\n\r\n" ) ) >= 0 )
      {
        const QByteArray request = buffer->left( end + 4 );
        buffer->remove( 0, end + 4 );
        if ( !sendReply( socket, request ) )
        {
          return;
        }
      }
      if ( buffer->size() > MAX_REQUEST_SIZE )
      {
        socket->write( genHeaders( 431, 0, false ) );
        socket->close();
      }
    };
    connect( socket, &QTcpSocket::readyRead, socket, onReadyRead );

    // Data may already have arrived before the signal was connected
    if ( socket->bytesAvailable() )
    {
      onReadyRead();
    }
  }
}

bool KadasFileServer::sendReply( QTcpSocket *socket, const QByteArray &data )
{
  const QByteArray requestLine = data.left( data.indexOf( "\r\n" ) );
  const QByteArrayList parts = requestLine.split( ' ' );

  // HTTP/1.1 connections are persistent unless the client asks to close,
  // HTTP/1.0 connections are closed unless the client asks to keep them open
  const QByteArray lowerData = data.toLower();
  const bool http10 = parts.length() >= 3 && parts[2].trimmed() == "HTTP/1.0";
  bool keepAlive = http10 ? lowerData.contains( "\r\nconnection: keep-alive" ) : !lowerData.contains( "\r\nconnection: close" );

  QByteArray headers;
  QByteArray body;

  if ( parts.length() >= 2 && ( ( parts[0] == "GET" ) || ( parts[0] == "HEAD" ) ) )
  {
    QByteArray requestMethod = parts[0];
    QByteArray fileRequested = parts[1];


    // Omit any querystring
    fileRequested = fileRequested.split( '?' )[0];

    // load index.html if not file specified
    if ( fileRequested.endsWith( '/' ) )
    {
      fileRequested += "index.html";
    }
    // Normalize path to resolve any ".." segments (e.g. /en/../search/worker.js)
    QString path = QFileInfo( mTopdir + fileRequested ).canonicalFilePath();
    // Prevent directory traversal outside the top dir
    if ( !path.startsWith( mTopdir ) )
    {
      path = QString();
    }
    QFile file( path );
    if ( !path.isEmpty() && file.open( QIODevice::ReadOnly ) )
    {
      body = file.readAll();
      headers = genHeaders( 200, body.size(), keepAlive, path );
    }
    else
    {
      body = QString( "<html><body><p>Error 404: File not found: '%1'</p></body></html>" ).arg( QString::fromUtf8( fileRequested ) ).toUtf8();
      headers = genHeaders( 404, body.size(), keepAlive );
    }
    // HEAD responses carry the headers of the GET response, but no body
    if ( requestMethod == "HEAD" )
    {
      body.clear();
    }
  }
  else
  {
    // Unsupported request, a body may follow which we can not delimit: close
    keepAlive = false;
    headers = genHeaders( 405, 0, false );
  }

  socket->write( headers + body );
  socket->flush();
  if ( !keepAlive )
  {
    socket->close();
  }
  return keepAlive;
}
