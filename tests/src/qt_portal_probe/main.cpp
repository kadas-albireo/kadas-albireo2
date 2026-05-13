// qt_portal_probe — minimal diagnostic for "browser works, Kadas times out"
//
// Build (Windows, with the same vcpkg toolchain Kadas uses):
//   mkdir build && cd build
//   cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
//   cmake --build .
//
// Run:
//   qt_portal_probe <url>                 # no proxy (Kadas default)
//   qt_portal_probe <url> --system-proxy  # like the browser
//   qt_portal_probe <url> --user me --pass secret
//
// The probe prints:
//   - Qt build features that matter (sspi, ssl backend, network info)
//   - Effective proxy
//   - Every signal QNAM emits (auth, proxy-auth, SSL errors, redirects, finished)
//   - Response status, headers, first bytes
//   - Whether it timed out (10 s)

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>

static QTextStream out( stdout );

static void dumpQtInfo()
{
  out << "=== Qt build ===" << Qt::endl;
  out << "Qt version:        " << QT_VERSION_STR << Qt::endl;
  out << "Runtime version:   " << qVersion() << Qt::endl;
  out << "SSL supported:     " << ( QSslSocket::supportsSsl() ? "yes" : "NO" ) << Qt::endl;
  out << "SSL build version: " << QSslSocket::sslLibraryBuildVersionString() << Qt::endl;
  out << "SSL runtime ver:   " << QSslSocket::sslLibraryVersionString() << Qt::endl;
  out << "Active TLS backend:" << QSslSocket::activeBackend() << Qt::endl;
  out << "Available backends:" << QSslSocket::availableBackends().join( ", " ) << Qt::endl;

  // QT_CONFIG(sspi) is a build-time check; expose via a probe define.
  // We can't read it from the public API, so we infer indirectly:
  //  - On Windows, if sspi is compiled in, the authenticator will try SSO
  //    on a Negotiate/NTLM challenge before emitting authenticationRequired().
  //  - You'll see this in the run: a request that gets a 401 with
  //    WWW-Authenticate: Negotiate but DOES NOT emit authenticationRequired
  //    implies SSO was attempted via SSPI.
  out << Qt::endl;
}

static void dumpProxy( const QUrl &url )
{
  out << "=== Proxy ===" << Qt::endl;
  const auto proxies = QNetworkProxyFactory::systemProxyForQuery( QNetworkProxyQuery( url ) );
  out << "System proxies for " << url.toString() << ":" << Qt::endl;
  for ( const auto &p : proxies )
    out << "  type=" << p.type() << " host=" << p.hostName() << ":" << p.port() << Qt::endl;
  const QNetworkProxy app = QNetworkProxy::applicationProxy();
  out << "QNetworkProxy::applicationProxy(): type=" << app.type() << " host=" << app.hostName() << ":" << app.port() << Qt::endl;
  out << Qt::endl;
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  QCommandLineParser p;
  p.addPositionalArgument( "url", "URL to probe" );
  QCommandLineOption sysProxyOpt( "system-proxy", "Use system proxy (like a browser)." );
  QCommandLineOption userOpt( "user", "Provide a username (no SSO).", "user" );
  QCommandLineOption passOpt( "pass", "Provide a password.", "pass" );
  QCommandLineOption timeoutOpt( "timeout", "Timeout in seconds (default 10).", "secs", "10" );
  QCommandLineOption verboseOpt( "verbose", "Enable qt.network.* logging." );
  p.addOption( sysProxyOpt );
  p.addOption( userOpt );
  p.addOption( passOpt );
  p.addOption( timeoutOpt );
  p.addOption( verboseOpt );
  p.addHelpOption();
  p.process( app );

  if ( p.positionalArguments().isEmpty() )
  {
    out << "Usage: qt_portal_probe <url> [--system-proxy] [--user X --pass Y] [--verbose]" << Qt::endl;
    return 2;
  }
  const QUrl url( p.positionalArguments().first() );
  const bool useSystemProxy = p.isSet( sysProxyOpt );
  const QString user = p.value( userOpt );
  const QString pass = p.value( passOpt );
  const int timeoutMs = p.value( timeoutOpt ).toInt() * 1000;

  if ( p.isSet( verboseOpt ) )
  {
    QLoggingCategory::setFilterRules(
      "qt.network.*=true\n"
      "qt.network.ssl.debug=true"
    );
  }

  dumpQtInfo();
  dumpProxy( url );

  QNetworkAccessManager nam;
  if ( useSystemProxy )
  {
    QNetworkProxyFactory::setUseSystemConfiguration( true );
    out << ">>> Using system proxy configuration (like the browser)" << Qt::endl;
  }
  else
  {
    nam.setProxy( QNetworkProxy::NoProxy );
    out << ">>> NoProxy (this is QNAM's default — what Kadas likely does)" << Qt::endl;
  }
  out << Qt::endl;

  QObject::connect( &nam, &QNetworkAccessManager::authenticationRequired, [&]( QNetworkReply *r, QAuthenticator *a ) {
    out
      << "[signal] authenticationRequired"
      << "  method="
      << a->option( "method" ).toString()
      << "  realm="
      << a->realm()
      << Qt::endl;
    out << "         (this means SSO was NOT used, or failed — Qt is asking us for credentials)" << Qt::endl;
    if ( !user.isEmpty() )
    {
      a->setUser( user );
      a->setPassword( pass );
      out << "         supplied --user/--pass" << Qt::endl;
    }
    else
    {
      out << "         no --user provided; leaving empty (forces SSPI/SSO if available)" << Qt::endl;
    }
  } );
  QObject::connect( &nam, &QNetworkAccessManager::proxyAuthenticationRequired, [&]( const QNetworkProxy &px, QAuthenticator *a ) {
    out << "[signal] proxyAuthenticationRequired host=" << px.hostName() << "  realm=" << a->realm() << Qt::endl;
  } );

  QNetworkRequest req( url );
  req.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy );
  req.setRawHeader( "User-Agent", "qt-portal-probe/1.0" );

  const qint64 t0 = QDateTime::currentMSecsSinceEpoch();
  QNetworkReply *reply = nam.get( req );

  QObject::connect( reply, &QNetworkReply::sslErrors, [&]( const QList<QSslError> &errs ) {
    for ( const auto &e : errs )
      out << "[ssl error] " << e.errorString() << Qt::endl;
  } );
  QObject::connect( reply, &QNetworkReply::redirected, [&]( const QUrl &u ) { out << "[redirect] -> " << u.toString() << Qt::endl; } );

  QTimer::singleShot( timeoutMs, &app, [&]() {
    out << Qt::endl << "!!! TIMEOUT after " << timeoutMs / 1000 << " s — aborting" << Qt::endl;
    out << "    isFinished=" << reply->isFinished() << "  bytesAvailable=" << reply->bytesAvailable() << Qt::endl;
    reply->abort();
  } );

  QObject::connect( reply, &QNetworkReply::finished, [&]() {
    const qint64 dt = QDateTime::currentMSecsSinceEpoch() - t0;
    out << Qt::endl << "=== Reply finished after " << dt << " ms ===" << Qt::endl;
    out << "error()        : " << reply->error() << "  (" << reply->errorString() << ")" << Qt::endl;
    out << "HTTP status    : " << reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() << " " << reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString() << Qt::endl;
    out << "final URL      : " << reply->url().toString() << Qt::endl;
    out << "headers:" << Qt::endl;
    const auto hdrs = reply->rawHeaderPairs();
    for ( const auto &h : hdrs )
      out << "  " << h.first << ": " << h.second << Qt::endl;
    const QByteArray body = reply->readAll();
    out << "body (" << body.size() << " bytes, first 400):" << Qt::endl;
    out << QString::fromUtf8( body.left( 400 ) ) << Qt::endl;
    app.quit();
  } );

  return app.exec();
}
