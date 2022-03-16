// conn_hls.h
//
// Server connector for HTTP live streams (HLS).
//
//   (C) Copyright 2014-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <curl/curl.h>

#include <QProcess>
#include <QTcpSocket>
#include <QTimer>

#include "connector.h"
#include "id3parser.h"
#include "m3uplaylist.h"
#include "meteraverage.h"

// #define CONN_HLS_DUMP_SEGMENTS QString("/home/fredg/hls_segments")

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
  void tagReceivedData(uint64_t bytes,Id3Tag *tag);
  void indexProcessStartData();
  void mediaProcessStartData();

 private:
  QByteArray ReadHeaders(QByteArray &data);
  void ProcessHeader(const QString &str);
  void StopProcess(QProcess *proc);
  M3uPlaylist *hls_index_playlist;
  Id3Parser *hls_id3_parser;
  QTimer *hls_index_timer;
  QDateTime hls_download_start_datetime;
  MeterAverage *hls_download_average;
  QUrl hls_index_url;
  QUrl hls_current_media_segment;
  QUrl hls_last_media_segment;
  QTimer *hls_media_timer;
  QString hls_server;
  QString hls_content_type;
  MetaEvent hls_meta_event;
  CURL *hls_curl_handle;
#ifdef CONN_HLS_DUMP_SEGMENTS
  int hls_segment_fd;
#endif  // CONN_HLS_DUMP_SEGMENTS
};


#endif  // CONN_HLS_H
