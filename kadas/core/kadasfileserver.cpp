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

#include <QDateTime>
#include <QFile>
#include <QTcpSocket>
#include <QTimer>

#include <qgis/qgslogger.h>
#include <kadas/core/kadasfileserver.h>

KadasFileServer::KadasFileServer( const QString &topdir, const QString &host, int port )
{
  mTopdir = topdir;
  listen( QHostAddress( host ), port );
  mHost = serverAddress().toString();
  mPort = serverPort();
  connect( this, &QTcpServer::newConnection, this, &KadasFileServer::handleConnection );
  QgsDebugMsgLevel( QString( "KadasFileServer running on %1:%2" ).arg( mHost ).arg( mPort ) , 2 );
}

QByteArray KadasFileServer::genHeaders( int code )
{
  QByteArray h;
  if ( code == 200 )
  {
    h = "HTTP/1.1 200 OK\n";
  }
  else if ( code == 404 )
  {
    h = "HTTP/1.1 404 Not Found\n";
  }
  else if ( code == 405 )
  {
    h = "HTTP/1.1 405 Method Not Allowed\n";
  }

  QByteArray current_date = QDateTime::currentDateTime().toString( Qt::RFC2822Date ).toLocal8Bit();

  h += "Date: " + current_date + "\n";
  h += "Connection: close\n\n";

  return h;
}

void KadasFileServer::handleConnection()
{
  while ( hasPendingConnections() )
  {
    QTcpSocket *socket = nextPendingConnection();
    if ( socket->isOpen() )
    {
      if ( socket->bytesAvailable() )
      {
        sendReply( socket );
      }
      else
      {
        QTimer *timeoutTimer = new QTimer( socket );
        connect( timeoutTimer, &QTimer::timeout, socket, [socket]
        {
          socket->close();
          socket->deleteLater();
        } );
        timeoutTimer->start( 500 );
        connect( socket, &QTcpSocket::readyRead, socket, [this, socket, timeoutTimer]
        {
          timeoutTimer->stop();
          timeoutTimer->deleteLater();
          sendReply( socket );
        } );
      }
    }
    else
    {
      socket->deleteLater();
    }
  }
}

void KadasFileServer::sendReply( QTcpSocket *socket )
{
  QByteArray data = socket->readAll();

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
    QString path = mTopdir + fileRequested;
    QFile file( path );
    if ( file.open( QIODevice::ReadOnly ) )
    {
      headers = genHeaders( 200 );
      if ( requestMethod == "GET" )
      {
        body = file.readAll();
      }
    }
    else
    {
      headers = genHeaders( 404 );
      if ( requestMethod == "GET" )
      {
        body = "<html><body><p>Error 404: File not found</p></body></html>";
      }
    }
  }
  else
  {
    headers = genHeaders( 405 );
  }

  socket->write( headers + body );
  socket->flush();
  socket->close();
  socket->deleteLater();
}
