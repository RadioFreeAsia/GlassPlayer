// conn_file.h
//
// Server connector for static files.
//
//   (C) Copyright 2016 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef CONN_FILE_H
#define CONN_FILE_H

#include <QTcpSocket>
#include <QTimer>

#include "connector.h"

#define FILE_WATCHDOG_RETRY_INTERVAL 5000

class File : public Connector
{
  Q_OBJECT;
 public:
  File(QObject *parent=0);
  ~File();
  Connector::ServerType serverType() const;
  void reset();

 protected:
  void connectToHostConnector();
  void disconnectFromHostConnector();
  void loadStats(QStringList *hdrs,QStringList *values);

 private slots:
  void connectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);
  void watchdogRetryData();

 private:
  void ProcessFrames(QByteArray &data);
  void SendHeader(const QString &str);
  void ProcessHeader(const QString &str);
  void ProcessMetadata(const QByteArray &mdata);
  void InitSocket();
  QString file_header;
  bool file_header_active;
  QTcpSocket *file_socket;
  int file_result_code;
  int file_metadata_interval;
  int file_metadata_istate;
  QString file_metadata_backstore;
  QString file_metadata_string;
  int file_metadata_counter;
  QTimer *file_watchdog_retry_timer;
  uint64_t file_byte_counter;
  QString file_server;
  QString file_content_type;
  bool file_is_shoutcast;
};


#endif  // CONN_FILE_H
