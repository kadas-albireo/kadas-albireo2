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

#include <QTcpServer>

#include "kadas/core/kadas_core.h"

class KADAS_CORE_EXPORT KadasFileServer : public QTcpServer {
  Q_OBJECT
public:
  KadasFileServer(const QString &topdir, const QString &host = "",
                  int port = 0);
  const QString &getHost() const { return mHost; }
  int getPort() const { return mPort; }

  QString getFilesTopDir() const { return mTopdir; }
  void setFilesTopDir(const QString &topDir);

private:
  QByteArray genHeaders(int code);

  QString mHost;
  int mPort;
  QString mTopdir;

private slots:
  void handleConnection();
  void sendReply(QTcpSocket *socket);
};
