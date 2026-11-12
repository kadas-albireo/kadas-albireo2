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

QByteArray KadasFileServer::genHeaders( int code, qint64 contentLength, const QString &filePath )
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
  h += "Connection: close\r\n";
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

    // Drop connections which never deliver a complete request
    QTimer *idleTimer = new QTimer( socket );
    idleTimer->setSingleShot( true );
    connect( idleTimer, &QTimer::timeout, socket, &QTcpSocket::close );
    idleTimer->start( IDLE_TIMEOUT_MS );

    // TCP is a byte stream: a request may arrive in several chunks, so accumulate
    // data until the end of the header section ("\r\n\r\n") has been received
    auto buffer = std::make_shared<QByteArray>();
    auto onReadyRead = [this, socket, idleTimer, buffer] {
      buffer->append( socket->readAll() );
      if ( buffer->contains( "\r\n\r\n" ) )
      {
        idleTimer->stop();
        sendReply( socket, *buffer );
      }
      else if ( buffer->size() > MAX_REQUEST_SIZE )
      {
        idleTimer->stop();
        socket->write( genHeaders( 431, 0 ) );
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

void KadasFileServer::sendReply( QTcpSocket *socket, const QByteArray &data )
{
  QByteArrayList parts = data.split( ' ' );

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
      headers = genHeaders( 200, body.size(), path );
    }
    else
    {
      body = QString( "<html><body><p>Error 404: File not found: '%1'</p></body></html>" ).arg( QString::fromUtf8( fileRequested ) ).toUtf8();
      headers = genHeaders( 404, body.size() );
    }
    // HEAD responses carry the headers of the GET response, but no body
    if ( requestMethod == "HEAD" )
    {
      body.clear();
    }
  }
  else
  {
    headers = genHeaders( 405, 0 );
  }

  socket->write( headers + body );
  socket->flush();
  socket->close();
  socket->deleteLater();
}
