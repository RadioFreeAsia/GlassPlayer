// conn_hls.h
//
// Server connector for HTTP live streams (HLS).
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

#ifndef CONN_HLS_H
#define CONN_HLS_H

#include <QProcess>
#include <QTcpSocket>
#include <QTimer>

#include "connector.h"
#include "m3uplaylist.h"

class Hls : public Connector
{
  Q_OBJECT;
 public:
  Hls(const QString &mimetype,QObject *parent=0);
  ~Hls();
  Connector::ServerType serverType() const;
  void reset();

 protected:
  void connectToHostConnector();
  void disconnectFromHostConnector();
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private slots:
  void indexProcessStartData();
  void indexProcessFinishedData(int exit_code,QProcess::ExitStatus status);
  void indexProcessErrorData(QProcess::ProcessError err);
  void mediaProcessStartData();
  void mediaReadyReadData();
  void mediaProcessFinishedData(int exit_code,QProcess::ExitStatus status);
  void mediaProcessErrorData(QProcess::ProcessError err);

 private:
  QByteArray ReadHeaders(QByteArray &data);
  void ProcessHeader(const QString &str);
  QProcess *hls_index_process;
  M3uPlaylist *hls_index_playlist;
  QTimer *hls_index_timer;
  QProcess *hls_media_process;
  QUrl hls_index_url;
  QUrl hls_current_media_segment;
  QUrl hls_last_media_segment;
  QTimer *hls_media_timer;
  bool hls_new_segment;
  QString hls_server;
  QString hls_content_type;
};


#endif  // CONN_HLS_H
