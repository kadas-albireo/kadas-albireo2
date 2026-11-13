/***************************************************************************
    kadasfileserver.h
    -----------------
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

#include <QMutex>
#include <QSemaphore>
#include <QThread>

#include "kadas/core/kadas_core.h"

class QTcpServer;
class QTcpSocket;

/**
 * Minimal HTTP file server, running in its own thread so that serving requests
 * is independent of the load of the GUI thread.
 */
class KADAS_CORE_EXPORT KadasFileServer : public QThread
{
    Q_OBJECT
  public:
    KadasFileServer( const QString &topdir, const QString &host = "", int port = 0 );
    ~KadasFileServer() override;
    const QString &getHost() const { return mHost; }
    int getPort() const { return mPort; }

  protected:
    void run() override;

  private:
    static constexpr int IDLE_TIMEOUT_MS = 15000;
    static constexpr int MAX_REQUEST_SIZE = 65536; // 64 KB

    QByteArray genHeaders( int code, qint64 contentLength, bool keepAlive, const QString &filePath = QString() );
    void handleConnections( QTcpServer *server );
    bool sendReply( QTcpSocket *socket, const QByteArray &request );

    // Written once by the worker thread before mStarted is released
    QString mHost;
    int mPort = 0;
    QSemaphore mStarted;

    QString mBindHost;
    int mBindPort = 0;

    const QString mTopdir;
};
