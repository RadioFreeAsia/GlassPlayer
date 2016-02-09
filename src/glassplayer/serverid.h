// serverid.h
//
// Identify remote server
//
//   (C) Copyright 2014-2016 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef SERVERID_H
#define SERVERID_H

#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include "connector.h"

class ServerId : public QObject
{
  Q_OBJECT;
 public:
  ServerId(QObject *parent=0);
  ~ServerId();
  void connectToServer(const QUrl &url);

 signals:
  void typeFound(Connector::ServerType type,const QString &mimetype,
		 const QUrl &url);

 private slots:
  void connectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);
  void killData();
  void restartData();

 private:
  void ProcessResult();
  void SendHeader(const QString &str);
  void ProcessHeader(const QString &str);
  QTcpSocket *CreateSocket();
  QTcpSocket *id_socket;
  QTimer *id_kill_timer;
  QUrl id_url;
  QString id_header;
  QString id_content_type;
  QString id_location;
  bool id_header_active;
  unsigned id_result_code;
  QString id_result_text;
  QByteArray id_body;
  QTimer *id_restart_timer;
  bool id_restarting;
  bool id_icy;
};


#endif  // SERVERID_H
