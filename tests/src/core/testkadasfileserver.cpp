/***************************************************************************
    testkadasfileserver.cpp
    -----------------------
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

#include <memory>

#include <QtTest/QTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QMap>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QThread>

#include <kadas/core/kadasfileserver.h>

namespace
{
  constexpr int TIMEOUT_MS = 5000;

  struct Response
  {
      bool valid = false;
      int status = 0;
      QMap<QByteArray, QByteArray> headers; // lower-case keys
      QByteArray body;
  };

  /**
   * Blocking HTTP client which keeps unconsumed bytes between responses,
   * so that pipelined responses can be read one after the other.
   */
  class Client
  {
    public:
      Client( const QString &host, int port )
      {
        mSocket.connectToHost( QHostAddress( host ), port );
        mSocket.waitForConnected( TIMEOUT_MS );
      }

      bool isConnected() const { return mSocket.state() == QAbstractSocket::ConnectedState; }

      void send( const QByteArray &data )
      {
        mSocket.write( data );
        mSocket.waitForBytesWritten( TIMEOUT_MS );
      }

      //! Reads exactly one response, using Content-Length to delimit the body
      Response read( bool head = false )
      {
        Response r;
        int end;
        while ( ( end = mBuffer.indexOf( "\r\n\r\n" ) ) < 0 )
        {
          if ( !fill() )
            return r;
        }
        const QByteArray headRaw = mBuffer.left( end );
        mBuffer.remove( 0, end + 4 );

        const QList<QByteArray> lines = headRaw.split( '\n' );
        const QList<QByteArray> statusLine = lines.value( 0 ).trimmed().split( ' ' );
        if ( statusLine.size() < 2 || !statusLine[0].startsWith( "HTTP/1.1" ) )
          return r;
        r.status = statusLine[1].toInt();
        for ( int i = 1; i < lines.size(); ++i )
        {
          const int colon = lines[i].indexOf( ':' );
          if ( colon > 0 )
            r.headers.insert( lines[i].left( colon ).trimmed().toLower(), lines[i].mid( colon + 1 ).trimmed() );
        }
        // Every header line must be terminated by CRLF
        for ( int i = 0; i < lines.size() - 1; ++i )
        {
          if ( !lines[i].endsWith( '\r' ) )
            return r;
        }

        const int length = head ? 0 : r.headers.value( "content-length", "0" ).toInt();
        while ( mBuffer.size() < length )
        {
          if ( !fill() )
            return r;
        }
        r.body = mBuffer.left( length );
        mBuffer.remove( 0, length );
        r.valid = true;
        return r;
      }

      //! Returns whether the server closed the connection within \a waitMs
      bool isClosed( int waitMs = 1000 )
      {
        QElapsedTimer timer;
        timer.start();
        while ( mSocket.state() != QAbstractSocket::UnconnectedState )
        {
          const int remaining = waitMs - static_cast<int>( timer.elapsed() );
          if ( remaining <= 0 || !mSocket.waitForReadyRead( remaining ) )
            break;
          mBuffer.append( mSocket.readAll() );
        }
        return mSocket.state() == QAbstractSocket::UnconnectedState;
      }

    private:
      bool fill()
      {
        if ( mSocket.bytesAvailable() == 0 && !mSocket.waitForReadyRead( TIMEOUT_MS ) && mSocket.bytesAvailable() == 0 )
          return false;
        mBuffer.append( mSocket.readAll() );
        return true;
      }

      QTcpSocket mSocket;
      QByteArray mBuffer;
  };

  QByteArray get( const QByteArray &path = "/", const QByteArray &extra = QByteArray(), const QByteArray &version = "1.1" )
  {
    return "GET " + path + " HTTP/" + version + "\r\nHost: localhost\r\n" + extra + "\r\n";
  }
} // namespace

class TestKadasFileServer : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void responseHasCrlfHeadersAndContentLength();
    void keepAliveServesMultipleRequests();
    void connectionCloseIsHonoured();
    void http10ClosesByDefault();
    void http10KeepAliveStaysOpen();
    void requestSplitInManyChunks();
    void lateRequestIsServed();
    void pipelinedRequestsAreAnswered();
    void headHasNoBody();
    void notFoundKeepsConnection();
    void directoryTraversalIsBlocked();
    void siblingDirectoryIsBlocked();
    void queryStringIsIgnored();
    void unsupportedMethodIsRejected();
    void oversizedHeadersAreRejected();
    void idleConnectionIsClosed();

  private:
    std::unique_ptr<Client> connect() const { return std::make_unique<Client>( mServer->getHost(), mServer->getPort() ); }

    QTemporaryDir mTempDir;
    std::unique_ptr<KadasFileServer> mServer;
    const QByteArray mIndexContent = "<html><body>KadasFileServer test</body></html>";
};

void TestKadasFileServer::initTestCase()
{
  QVERIFY( mTempDir.isValid() );
  // The server compares canonical paths (e.g. /var -> /private/var on macOS)
  const QString base = QFileInfo( mTempDir.path() ).canonicalFilePath();
  const QString root = base + "/www";
  QVERIFY( QDir().mkpath( root ) );
  QVERIFY( QDir().mkpath( base + "/www2" ) );

  QFile index( root + "/index.html" );
  QVERIFY( index.open( QIODevice::WriteOnly ) );
  index.write( mIndexContent );
  index.close();

  // Files outside the served directory, which must never be served
  for ( const QString &path : { base + "/secret.txt", base + "/www2/secret.txt" } )
  {
    QFile secret( path );
    QVERIFY( secret.open( QIODevice::WriteOnly ) );
    secret.write( "secret" );
  }

  mServer = std::make_unique<KadasFileServer>( root, QStringLiteral( "127.0.0.1" ) );
  QVERIFY( mServer->getPort() > 0 );
}

void TestKadasFileServer::cleanupTestCase()
{
  mServer.reset();
}

void TestKadasFileServer::responseHasCrlfHeadersAndContentLength()
{
  auto c = connect();
  c->send( get() );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 200 );
  QVERIFY( r.headers.contains( "content-length" ) );
  QCOMPARE( r.body, mIndexContent );
  QCOMPARE( r.headers.value( "content-type" ), QByteArray( "text/html" ) );
}

void TestKadasFileServer::keepAliveServesMultipleRequests()
{
  auto c = connect();
  c->send( get() );
  const Response r1 = c->read();
  c->send( get() );
  const Response r2 = c->read();
  QVERIFY( r1.valid );
  QVERIFY( r2.valid );
  QCOMPARE( r2.status, 200 );
  QCOMPARE( r1.body, r2.body );
  QCOMPARE( r1.headers.value( "connection" ), QByteArray( "keep-alive" ) );
}

void TestKadasFileServer::connectionCloseIsHonoured()
{
  auto c = connect();
  c->send( get( "/", "Connection: close\r\n" ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.headers.value( "connection" ), QByteArray( "close" ) );
  QVERIFY( c->isClosed() );
}

void TestKadasFileServer::http10ClosesByDefault()
{
  auto c = connect();
  c->send( get( "/", QByteArray(), "1.0" ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.headers.value( "connection" ), QByteArray( "close" ) );
  QVERIFY( c->isClosed() );
}

void TestKadasFileServer::http10KeepAliveStaysOpen()
{
  auto c = connect();
  c->send( get( "/", "Connection: keep-alive\r\n", "1.0" ) );
  const Response r1 = c->read();
  c->send( get( "/", "Connection: keep-alive\r\n", "1.0" ) );
  const Response r2 = c->read();
  QVERIFY( r1.valid );
  QVERIFY( r2.valid );
  QCOMPARE( r2.status, 200 );
}

void TestKadasFileServer::requestSplitInManyChunks()
{
  auto c = connect();
  const QByteArray request = get();
  for ( char b : request )
  {
    c->send( QByteArray( 1, b ) );
    QThread::msleep( 5 );
  }
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 200 );
}

void TestKadasFileServer::lateRequestIsServed()
{
  // Older code dropped connections which were idle for 500 ms
  auto c = connect();
  QThread::msleep( 1500 );
  c->send( get() );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 200 );
}

void TestKadasFileServer::pipelinedRequestsAreAnswered()
{
  auto c = connect();
  c->send( get() + get( "/does-not-exist.html" ) );
  const Response r1 = c->read();
  const Response r2 = c->read();
  QVERIFY( r1.valid );
  QVERIFY( r2.valid );
  QCOMPARE( r1.status, 200 );
  QCOMPARE( r2.status, 404 );
}

void TestKadasFileServer::headHasNoBody()
{
  auto c = connect();
  QByteArray request = get();
  request.replace( 0, 3, "HEAD" );
  c->send( request );
  const Response head = c->read( true );
  // A body sent after the HEAD response would be parsed as the next response here
  c->send( get() );
  const Response r = c->read();
  QVERIFY( head.valid );
  QVERIFY( r.valid );
  QCOMPARE( head.status, 200 );
  QCOMPARE( r.status, 200 );
  QCOMPARE( head.headers.value( "content-length" ), r.headers.value( "content-length" ) );
}

void TestKadasFileServer::notFoundKeepsConnection()
{
  auto c = connect();
  c->send( get( "/does-not-exist.html" ) );
  const Response r1 = c->read();
  QVERIFY( r1.valid );
  QCOMPARE( r1.status, 404 );
  c->send( get() );
  const Response r2 = c->read();
  QVERIFY( r2.valid );
  QCOMPARE( r2.status, 200 );
}

void TestKadasFileServer::directoryTraversalIsBlocked()
{
  auto c = connect();
  c->send( get( "/../secret.txt" ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 404 );
}

void TestKadasFileServer::siblingDirectoryIsBlocked()
{
  // ".../www2" starts with ".../www", but is outside the served directory
  auto c = connect();
  c->send( get( "/../www2/secret.txt" ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 404 );
}

void TestKadasFileServer::queryStringIsIgnored()
{
  auto c = connect();
  c->send( get( "/index.html?foo=bar" ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 200 );
  QCOMPARE( r.body, mIndexContent );
}

void TestKadasFileServer::unsupportedMethodIsRejected()
{
  auto c = connect();
  c->send( "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 3\r\n\r\nabc" );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 405 );
  QVERIFY( c->isClosed() );
}

void TestKadasFileServer::oversizedHeadersAreRejected()
{
  auto c = connect();
  c->send( "GET / HTTP/1.1\r\nX-Junk: " + QByteArray( 70000, 'a' ) );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 431 );
}

void TestKadasFileServer::idleConnectionIsClosed()
{
  if ( qEnvironmentVariableIsEmpty( "KADAS_RUN_SLOW_TESTS" ) )
    QSKIP( "Takes ~25 s, set KADAS_RUN_SLOW_TESTS=1 to run" );

  auto c = connect();
  QThread::msleep( 10000 );
  c->send( get() );
  const Response r = c->read();
  QVERIFY( r.valid );
  QCOMPARE( r.status, 200 );

  QElapsedTimer timer;
  timer.start();
  QVERIFY( c->isClosed( 20000 ) );
  QVERIFY2( timer.elapsed() > 13000 && timer.elapsed() < 18000, QByteArray::number( timer.elapsed() ).constData() );
}

QTEST_GUILESS_MAIN( TestKadasFileServer )
#include "testkadasfileserver.moc"
