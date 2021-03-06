// connector.h
//
// Abstract base class for streaming server connections.
//
//   (C) Copyright 2014-2020 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <stdint.h>

#include <vector>

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include "codec.h"
#include "metaevent.h"

class Connector : public QObject
{
  Q_OBJECT;
 public:
  enum ServerType {XCastServer=1,HlsServer=2,FileServer=3,SignalGenerator=4,
		   LastServer=5};
  Connector(const QString &mimetype,QObject *parent=0);
  ~Connector();
  virtual Connector::ServerType serverType() const=0;
  QString postData() const;
  void setPostData(const QString &str);
  QString serverUsername() const;
  void setServerUsername(const QString &str);
  QString serverPassword() const;
  void setServerPassword(const QString &str);
  QUrl serverUrl() const;
  void setServerUrl(const QUrl &url);
  QUrl publicUrl() const;
  void setPublicUrl(const QUrl &url);
  QString serverMountpoint() const;
  unsigned audioChannels() const;
  unsigned audioSamplerate() const;
  unsigned audioBitrate() const;
  void setAudioBitrate(unsigned rate);
  std::vector<unsigned> *audioBitrates();
  void setAudioBitrates(std::vector<unsigned> *rates);
  bool streamMetadataEnabled() const;
  void setStreamMetadataEnabled(bool state);
  bool dumpHeaders() const;
  void setDumpHeaders(bool state);
  void startMetadata();
  MetaEvent *metadataEvent();
  Codec::Type codecType() const;
  QString contentType() const;
  bool isConnected() const;
  virtual void connectToServer();
  void stop();
  virtual void reset()=0;
  QString scriptUp() const;
  void setScriptUp(const QString &cmd);
  QString scriptDown() const;
  void setScriptDown(const QString &cmd);
  virtual void getStats(QStringList *hdrs,QStringList *values,bool is_first);
  static QString serverTypeText(Connector::ServerType);
  static QString optionKeyword(Connector::ServerType type);
  static Connector::ServerType serverType(const QString &key);
  static bool acceptsContentType(ServerType type,const QString &mimetype);
  static QString subMountpointName(const QString &mntpt,unsigned bitrate);
  static QString pathPart(const QString &fullpath);
  static QString basePart(const QString &fullpath);
  static QString urlEncode(const QString &str);
  static QString urlDecode(const QString &str);
  static QString base64Encode(const QString &str);
  static QString base64Decode(const QString &str,bool *ok=NULL);
  static QString curlStrError(int exit_code);
  static QString httpStrError(int status_code);
  static QString socketErrorText(QAbstractSocket::SocketError err);
  static QString processErrorText(QProcess::ProcessError err);
  static QDateTime xmlTimestamp(const QString &str);
  static int timezoneOffset();
  static QString timezoneOffsetString();

 signals:
  void connected(bool state);
  void dataReceived(const QByteArray &data,bool is_last);
  void error(QAbstractSocket::SocketError err);
  void metadataReceived(uint64_t bytes,MetaEvent *e);

 protected:
  void setAudioChannels(unsigned chans);
  void setAudioSamplerate(unsigned samprate);
  void setCodecType(Codec::Type type);
  void setConnected(bool state);
  void setMetadataField(uint64_t bytes,const QString &key,const QString &str);
  virtual void connectToHostConnector()=0;
  virtual void disconnectFromHostConnector()=0;
  virtual void loadStats(QStringList *hdrs,QStringList *values,bool is_first)=0;

 private:
  QString conn_server_username;
  QString conn_server_password;
  QUrl conn_server_url;
  QUrl conn_public_url;
  QString conn_post_data;
  std::vector<unsigned> conn_audio_bitrates;
  QString conn_stream_name;
  QString conn_stream_description;
  QString conn_stream_url;
  QString conn_stream_irc;
  QString conn_stream_icq;
  QString conn_stream_aim;
  QString conn_stream_genre;
  bool conn_stream_metadata_enabled;
  bool conn_dump_headers;
  QString conn_stream_metadata;
  bool conn_stream_public;
  Codec::Type conn_codec_type;
  QString conn_host_hostname;
  uint16_t conn_host_port;
  QString conn_script_up;
  QString conn_script_down;
  bool conn_connected;
  bool conn_connected_changed;
  unsigned conn_dropouts;
  bool conn_dropouts_changed;
  QString conn_content_type;
  unsigned conn_audio_channels;
  unsigned conn_audio_samplerate;
  MetaEvent conn_metadata;
  bool conn_start_metadata;
};


#endif  // CONNECTOR_H
