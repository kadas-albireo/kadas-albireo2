/***************************************************************************
    kadasmilxclient.cpp
    -------------------
    copyright            : (C) 2019 by Sandro Mani
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

#include <librsvg/rsvg.h>
#include <cairo.h>
#include <memory>

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QImage>
#include <QProcess>
#include <QSettings>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

#include <qgis/qgssettings.h>

#include <kadas/app/milx/kadasmilxclient.h>
#include <kadas/app/milx/kadasmilxinterface.h>


KadasMilxClientWorker::KadasMilxClientWorker( bool sync )
  : mSync( sync ), mProcess( 0 ), mNetworkSession( 0 ), mTcpSocket( 0 )
{
}

void KadasMilxClientWorker::cleanup()
{
  if ( mProcess )
  {
    mProcess->deleteLater();
    mProcess = 0;
  }
  if ( mTcpSocket )
  {
    mTcpSocket->deleteLater();
    mTcpSocket = 0;
  }
  delete mNetworkSession;
  mNetworkSession = 0;
}

bool KadasMilxClientWorker::initialize()
{
  if ( mTcpSocket )
  {
    return true;
  }

  cleanup();
  mLastError = QString();

  // Start process
  QByteArray portEnv = mSync ? "MILIX_SERVER_PORT_SYNC" : "MILIX_SERVER_PORT_ASYNC";
#ifdef Q_OS_WIN
  int port;
  QHostAddress addr( QHostAddress::LocalHost );
  if ( !qgetenv( portEnv ).isEmpty() && !qgetenv( "MILIX_SERVER_ADDR" ).isEmpty() )
  {
    port = atoi( qgetenv( portEnv ) );
    addr = QHostAddress( QString( qgetenv( "MILIX_SERVER_ADDR" ) ) );
  }
  else
  {
    mProcess = new QProcess( this );
    connect( mProcess, &QProcess::finished, this, &KadasMilxClientWorker::cleanup );
    {
#ifdef __MINGW32__
      QString serverDir = QDir( QString( "%1/../opt/mss/" ).arg( QApplication::applicationDirPath() ) ).absolutePath();
      QString serverPath = QDir( serverDir ).absoluteFilePath( "milxserver.exe" );
      mProcess->setWorkingDirectory( serverDir );
#else
      QString serverPath = "milxserver";
#endif
      mProcess->start( QString( "\"%1\"" ).arg( serverPath ) );
      mProcess->waitForReadyRead( 10000 );
      QByteArray out = mProcess->readAllStandardOutput();
      if ( !mProcess->isOpen() )
      {
        cleanup();
        mLastError = tr( "Process failed to start: %1" ).arg( mProcess->errorString() );
        return false;
      }
      else if ( out.isEmpty() )
      {
        cleanup();
        mLastError = tr( "Could not determine process port" );
        return false;
      }
      port = QString( out ).toInt();
    }
  }
#else
  int port = atoi( qgetenv( portEnv ) );
  QHostAddress addr = QHostAddress( QString( qgetenv( "MILIX_SERVER_ADDR" ) ) );
#endif

  // Initialize network
  QNetworkConfigurationManager manager;
  if ( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
  {
    // Get saved network configuration
    QgsSettings settings( QSettings::UserScope, QLatin1String( "QtProject" ) );
    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    QString id = settings.value( QLatin1String( "DefaultNetworkConfiguration" ) ).toString();
    settings.endGroup();

    // If the saved network configuration is not currently discovered use the system default
    QNetworkConfiguration config = manager.configurationFromIdentifier( id );
    if ( ( config.state() & QNetworkConfiguration::Discovered ) !=
         QNetworkConfiguration::Discovered )
    {
      config = manager.defaultConfiguration();
    }

    mNetworkSession = new QNetworkSession( config, this );
    QEventLoop evLoop;
    connect( mNetworkSession, &QNetworkSession::opened, &evLoop, &QEventLoop::quit );
    mNetworkSession->open();
    evLoop.exec();

    // Save the used configuration
    config = mNetworkSession->configuration();
    if ( config.type() == QNetworkConfiguration::UserChoice )
      id = mNetworkSession->sessionProperty( QLatin1String( "UserChoiceConfiguration" ) ).toString();
    else
      id = config.identifier();

    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    settings.setValue( QLatin1String( "DefaultNetworkConfiguration" ), id );
    settings.endGroup();
  }

  // Connect to server
  mTcpSocket = new QTcpSocket( this );
  QTimer timeoutTimer;
  timeoutTimer.setSingleShot( true );
  connect( mTcpSocket, &QTcpSocket::disconnected, this, &KadasMilxClientWorker::cleanup );
  connect( mTcpSocket, qOverload<QAbstractSocket::SocketError>( &QTcpSocket::error ), this, &KadasMilxClientWorker::handleSocketError );
  {
    QEventLoop evLoop;
    connect( mTcpSocket, qOverload<QAbstractSocket::SocketError>( &QTcpSocket::error ), &evLoop, &QEventLoop::quit );
    connect( mTcpSocket, &QTcpSocket::disconnected, &evLoop, &QEventLoop::quit );
    connect( mTcpSocket, &QTcpSocket::connected, &evLoop, &QEventLoop::quit );
    connect( &timeoutTimer, &QTimer::timeout, &evLoop, &QEventLoop::quit );
    mTcpSocket->connectToHost( addr, port );
    timeoutTimer.start( 1000 );
    evLoop.exec();
  }
  if ( !mTcpSocket || mTcpSocket->state() != QTcpSocket::ConnectedState )
  {
    if ( mTcpSocket )
      mTcpSocket->abort();
    cleanup();
    return false;
  }

  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  QString lang = QgsSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();
  istream << MILX_REQUEST_INIT;
  istream << lang;
  istream << MILX_INTERFACE_VERSION;
  QByteArray response;
  if ( !processRequest( request, response, MILX_REPLY_INIT_OK ) )
  {
    cleanup();
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> mLibraryVersionTag;

  return true;
}

bool KadasMilxClientWorker::getCurrentLibraryVersionTag( QString &versionTag )
{
  if ( initialize() )
  {
    versionTag = mLibraryVersionTag;
    return true;
  }
  return false;
}

bool KadasMilxClientWorker::processRequest( const QByteArray &request, QByteArray &response, quint8 expectedReply, bool forceSync )
{
  mLastError = QString();

  if ( !mTcpSocket && !initialize() )
  {
    mLastError = tr( "Connection failed" );
    return false;
  }

  int requiredSize = 0;
  response.clear();

  qint32 len = request.size();
  mTcpSocket->write( reinterpret_cast<char *>( &len ), sizeof( quint32 ) );
  mTcpSocket->write( request );
  mTcpSocket->flush();

  do
  {
    if ( mSync || forceSync )
    {
      mTcpSocket->waitForReadyRead( 5000 );
    }
    else
    {
      if ( !mTcpSocket->bytesAvailable() )
      {
        QEventLoop evLoop;
        connect( mTcpSocket, &QTcpSocket::readyRead, &evLoop, &QEventLoop::quit );
        evLoop.exec( QEventLoop::ExcludeUserInputEvents );
      }
    }

    if ( !mLastError.isEmpty() || !mTcpSocket->isValid() )
    {
      return false;
    }
    response += mTcpSocket->readAll();
    if ( requiredSize == 0 )
    {
      requiredSize = *reinterpret_cast<qint32 *>( response.left( sizeof( qint32 ) ).data() );
      response = response.mid( sizeof( qint32 ) );
    }
  }
  while ( response.size() < requiredSize );
  Q_ASSERT( mTcpSocket->bytesAvailable() == 0 );

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  if ( replycmd == MILX_REPLY_ERROR )
  {
    ostream >> mLastError;
    return false;
  }
  else if ( replycmd != expectedReply )
  {
    mLastError = tr( "Unexpected reply" );
    return false;
  }
  return true;
}

void KadasMilxClientWorker::handleSocketError()
{
  if ( !mTcpSocket )
  {
    return;
  }
  QTcpSocket::SocketError socketError = mTcpSocket->error();

  switch ( socketError )
  {
    case QAbstractSocket::RemoteHostClosedError:
      mLastError = tr( "Connection closed" );
      break;
    case QAbstractSocket::HostNotFoundError:
      mLastError = tr( "Could not find specified host" );
      break;
    case QAbstractSocket::ConnectionRefusedError:
      mLastError = tr( "Connection refused" );
      break;
    default:
      mLastError = tr( "An error occured: %1" ).arg( mTcpSocket->errorString() );
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasMilxClient *KadasMilxClient::sInstance = 0;

KadasMilxClient *KadasMilxClient::instance()
{
  if ( !sInstance )
  {
    sInstance = new KadasMilxClient();
  }
  return sInstance;
}

KadasMilxClient::KadasMilxClient()
  : mAsyncWorker( false ), mSyncWorker( true )
{
  ;
  mSyncWorker.moveToThread( this );
  start();
}

KadasMilxClient::~KadasMilxClient()
{
  mAsyncWorker.cleanup();
  mSyncWorker.cleanup();
  QThread::quit();
  wait();
}

bool KadasMilxClient::processRequest( const QByteArray &request, QByteArray &response, quint8 expectedReply, bool async )
{
  // If not running in GUI thread, post synchronous request
  if ( QThread::currentThread() != qApp->thread() || !async )
  {
    bool result;
    QMetaObject::invokeMethod( &mSyncWorker, "processRequest", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ), Q_ARG( const QByteArray &, request ), Q_ARG( QByteArray &, response ), Q_ARG( quint8, expectedReply ) );
    return result;
  }
  else
  {
    return mAsyncWorker.processRequest( request, response, expectedReply );
  }
}

QString KadasMilxClient::attributeName( AttributeType idx )
{
  if ( idx == AttributeWidth )
    return "Width";
  else if ( idx == AttributeLength )
    return "Length";
  else if ( idx == AttributeRadius )
    return "Radius";
  else if ( idx == AttributeAttitude )
    return "Attitude";
  return "";
}

KadasMilxClient::AttributeType KadasMilxClient::attributeIdx( const QString &name )
{
  if ( name == "Width" )
    return AttributeWidth;
  else if ( name == "Length" )
    return AttributeLength;
  else if ( name == "Radius" )
    return AttributeRadius;
  else if ( name == "Attitude" )
    return AttributeAttitude;
  return AttributeUnknown;
}

bool KadasMilxClient::init()
{
  bool result;
  QMetaObject::invokeMethod( &instance()->mSyncWorker, "initialize", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ) );
  return result && instance()->mAsyncWorker.initialize();
}

bool KadasMilxClient::getSymbolMetadata( const QString &symbolId, SymbolDesc &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_SYMBOL_METADATA;
  istream << symbolId;
  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_SYMBOL_METADATA ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml;
  ostream >> result.name >> result.militaryName >> svgxml >> result.hasVariablePoints >> result.minNumPoints;
  result.icon = renderSvg( svgxml );
  return true;
}

bool KadasMilxClient::getSymbolsMetadata( const QStringList &symbolIds, QList<SymbolDesc> &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_SYMBOLS_METADATA;
  istream << symbolIds;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_SYMBOLS_METADATA ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  int nResults;
  ostream >> nResults;
  if ( nResults != symbolIds.size() )
  {
    return false;
  }
  for ( int i = 0; i < nResults; ++i )
  {
    QByteArray svgxml;
    SymbolDesc desc;
    desc.symbolXml = symbolIds[i];
    ostream >> desc.name >> desc.militaryName >> svgxml >> desc.hasVariablePoints >> desc.minNumPoints;
    desc.icon = renderSvg( svgxml );
    result.append( desc );
  }
  return true;
}

bool KadasMilxClient::appendPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, NPointSymbolGraphic &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_APPEND_POINT << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << newPoint;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_APPEND_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::insertPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, NPointSymbolGraphic &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_INSERT_POINT << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << newPoint;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_INSERT_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::movePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, const QPoint &newPos, NPointSymbolGraphic &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_MOVE_POINT << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << index << newPos;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_MOVE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::moveAttributePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int attr, const QPoint &newPos, NPointSymbolGraphic &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_MOVE_ATTRIBUTE_POINT << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << attr << newPos;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_MOVE_ATTRIBUTE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::canDeletePoint( const NPointSymbol &symbol, int index, bool &canDelete )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_CAN_DELETE_POINT;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << index;
  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_CAN_DELETE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> canDelete;
  return true;
}

bool KadasMilxClient::deletePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, NPointSymbolGraphic &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_DELETE_POINT << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << index;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_DELETE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::editSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, QString &newSymbolXml, QString &newSymbolMilitaryName, NPointSymbolGraphic &result, WId parentWid )
{
#ifdef Q_OS_WIN
  WId wid = parentWid;
#else
  Q_UNUSED( parentWid );
  WId wid = 0;
#endif

  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_EDIT_SYMBOL << visibleExtent << dpi << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << wid;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_EDIT_SYMBOL, true ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> newSymbolXml;
  ostream >> newSymbolMilitaryName;
  KadasMilxClient::deserializeSymbol( ostream, result );
  return true;
}

bool KadasMilxClient::createSymbol( QString &symbolId, SymbolDesc &result, WId parentWid )
{
#ifdef Q_OS_WIN
  WId wid = parentWid;
#else
  Q_UNUSED( parentWid );
  WId wid = 0;
#endif

  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_CREATE_SYMBOL << wid;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_CREATE_SYMBOL, true ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml;
  ostream >> symbolId >> result.name >> result.militaryName >> svgxml >> result.hasVariablePoints >> result.minNumPoints;
  result.icon = renderSvg( svgxml );
  return true;
}

bool KadasMilxClient::updateSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, NPointSymbolGraphic &result, bool returnPoints )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_UPDATE_SYMBOL;
  istream << visibleExtent << dpi;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << returnPoints;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_UPDATE_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  KadasMilxClient::deserializeSymbol( ostream, result, returnPoints );
  return true;
}

bool KadasMilxClient::updateSymbols( const QRect &visibleExtent, int dpi, double scaleFactor, const QList<NPointSymbol> &symbols, QList<NPointSymbolGraphic> &result )
{
  int nSymbols = symbols.length();
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_UPDATE_SYMBOLS;
  istream << visibleExtent;
  istream << dpi;
  istream << scaleFactor;
  istream << nSymbols;
  for ( const NPointSymbol &symbol : symbols )
  {
    istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored;
  }
  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_UPDATE_SYMBOLS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  int nOutSymbols;
  ostream >> nOutSymbols;
  if ( nOutSymbols != nSymbols )
  {
    return false;
  }
  for ( int i = 0; i < nOutSymbols; ++i )
  {
    NPointSymbolGraphic symbolGraphic;
    QByteArray svgxml; ostream >> svgxml;
    symbolGraphic.graphic = renderSvg( svgxml );
    ostream >> symbolGraphic.offset;
    result.append( symbolGraphic );
  }
  return true;
}

bool KadasMilxClient::upgradeMilXFile( const QString &inputXml, QString &outputXml, bool &valid, QString &messages )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_UPGRADE_MILXLY;
  istream << inputXml;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_UPGRADE_MILXLY ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> outputXml >> valid >> messages;
  return true;
}

bool KadasMilxClient::downgradeMilXFile( const QString &inputXml, QString &outputXml, const QString &mssVersion, bool &valid, QString &messages )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_DOWNGRADE_MILXLY;
  istream << inputXml << mssVersion;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_DOWNGRADE_MILXLY ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> outputXml >> valid >> messages;
  return true;
}

bool KadasMilxClient::validateSymbolXml( const QString &symbolXml, const QString &mssVersion, QString &adjustedSymbolXml, bool &valid, QString &messages )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_VALIDATE_SYMBOLXML;
  istream << symbolXml << mssVersion;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_VALIDATE_SYMBOLXML ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> adjustedSymbolXml >> valid >> messages;
  return true;
}


bool KadasMilxClient::hitTest( const NPointSymbol &symbol, const QPoint &clickPos, bool &hitTestResult )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_HIT_TEST;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored << clickPos;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_HIT_TEST ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> hitTestResult;
  return true;
}

bool KadasMilxClient::pickSymbol( const QList<NPointSymbol> &symbols, const QPoint &clickPos, int &selectedSymbol, QRect &boundingBox )
{
  int nSymbols = symbols.length();
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_PICK_SYMBOL << clickPos;
  istream << nSymbols;
  foreach ( const NPointSymbol &symbol, symbols )
  {
    istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.attributes << symbol.finalized << symbol.colored;
  }
  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_PICK_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> selectedSymbol >> boundingBox;
  return true;
}

bool KadasMilxClient::getSupportedLibraryVersionTags( QStringList &versionTags, QStringList &versionNames )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_LIBRARY_VERSION_TAGS;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_LIBRARY_VERSION_TAGS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> versionTags >> versionNames;
  return true;
}

bool KadasMilxClient::getCurrentLibraryVersionTag( QString &versionTag )
{
  bool result;
  QMetaObject::invokeMethod( &instance()->mSyncWorker, "getCurrentLibraryVersionTag", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ), Q_ARG( QString &, versionTag ) );
  return result;
}

bool KadasMilxClient::setSymbolOptions( int symbolSize, int lineWidth, int workMode )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_SET_SYMBOL_OPTIONS << symbolSize << lineWidth << workMode;

  QByteArray response;
  if ( !processRequest( request, response, MILX_REPLY_SET_SYMBOL_OPTIONS ) )
  {
    return false;
  }
  // Also set the options for the async worker
  if ( !mAsyncWorker.processRequest( request, response, MILX_REPLY_SET_SYMBOL_OPTIONS, true ) )
  {
    return false;
  }
  return true;
}

bool KadasMilxClient::getControlPointIndices( const QString &symbolXml, int nPoints, QList<int> &controlPoints )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_CONTROL_POINT_INDICES << symbolXml << nPoints;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_CONTROL_POINT_INDICES ) )
  {
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> controlPoints;
  return true;
}

bool KadasMilxClient::getControlPoints( const QString &symbolXml, QList<QPoint> &points, const QList< QPair<int, double> > &attributes, QList<int> &controlPoints, bool isCorridor )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_CONTROL_POINTS << symbolXml << points << attributes << isCorridor;

  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_CONTROL_POINTS ) )
  {
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> points >> controlPoints;
  return true;
}

bool KadasMilxClient::getMilitaryName( const QString &symbolXml, QString &militaryName )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << MILX_REQUEST_GET_MILITARY_NAME << symbolXml;
  QByteArray response;
  if ( !instance()->processRequest( request, response, MILX_REPLY_GET_MILITARY_NAME ) )
  {
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  MilXServerReply replycmd = 0; ostream >> replycmd;
  ostream >> militaryName;
  return true;
}

QImage KadasMilxClient::renderSvg( const QByteArray &xml )
{
  if ( xml.isEmpty() )
  {
    return QImage();
  }
  GInputStream *stream = g_memory_input_stream_new_from_data( reinterpret_cast<const unsigned char *>( xml.constData() ), xml.length(), nullptr );
  RsvgHandle *handle = rsvg_handle_new_from_stream_sync( stream, nullptr, RSVG_HANDLE_FLAGS_NONE, nullptr, nullptr );
  if ( handle == 0 )
  {
    return QImage();
  }
  RsvgDimensionData dimension_data;
  rsvg_handle_get_dimensions( handle, &dimension_data );

  QImage image( dimension_data.width, dimension_data.height, QImage::Format_ARGB32 );
  image.fill( Qt::transparent );
  cairo_surface_t *surface = cairo_image_surface_create_for_data( image.bits(), CAIRO_FORMAT_ARGB32, image.width(), image.height(), image.bytesPerLine() );
  cairo_t *cr = cairo_create( surface );
  rsvg_handle_render_cairo( handle, cr );
  cairo_destroy( cr );
  cairo_surface_destroy( surface );
  g_object_unref( handle );
  g_object_unref( stream );
  return image;
}

void KadasMilxClient::deserializeSymbol( QDataStream &ostream, NPointSymbolGraphic &result, bool deserializePoints )
{
  QList<QPair<int, double>> attributes;
  QList<QPair<int, QPoint>> attributePoints;
  QByteArray svgxml;

  ostream >> svgxml;
  ostream >> result.offset;
  if ( deserializePoints )
  {
    ostream >> result.adjustedPoints;
    ostream >> result.controlPoints;
    ostream >> attributes;
    ostream >> attributePoints;
  }

  result.graphic = renderSvg( svgxml );
  for ( const auto &pair : attributes )
  {
    KadasMilxClient::AttributeType attr = static_cast<KadasMilxClient::AttributeType>( pair.first );
    result.attributes.insert( attr, pair.second );
  }
  for ( const auto &pair : attributePoints )
  {
    KadasMilxClient::AttributeType attr = static_cast<KadasMilxClient::AttributeType>( pair.first );
    result.attributePoints.insert( attr, pair.second );
  }
}
