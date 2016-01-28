// connector.h
//
// Abstract base class for streaming server connections.
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

class Connector : public QObject
{
  Q_OBJECT;
 public:
  enum ServerType {XCastServer=1,LastServer=2};
  Connector(QObject *parent=0);
  ~Connector();
  virtual Connector::ServerType serverType() const=0;
  QString serverUsername() const;
  void setServerUsername(const QString &str);
  QString serverPassword() const;
  void setServerPassword(const QString &str);
  QUrl serverUrl() const;
  void setServerUrl(const QUrl &url);
  QString serverMountpoint() const;
  QString contentType() const;
  void setContentType(const QString &str);
  unsigned audioBitrate() const;
  void setAudioBitrate(unsigned rate);
  std::vector<unsigned> *audioBitrates();
  void setAudioBitrates(std::vector<unsigned> *rates);
  QString streamName() const;
  void setStreamName(const QString &str);
  QString streamDescription() const;
  void setStreamDescription(const QString &str);
  QString streamUrl() const;
  void setStreamUrl(const QString &str);
  QString streamIrc() const;
  void setStreamIrc(const QString &str);
  QString streamIcq() const;
  void setStreamIcq(const QString &str);
  QString streamAim() const;
  void setStreamAim(const QString &str);
  QString streamGenre() const;
  void setStreamGenre(const QString &str);
  bool streamMetadataEnabled() const;
  void setStreamMetadataEnabled(bool state);
  QString streamMetadata() const;
  void setStreamMetadata(const QString &str);
  bool streamPublic() const;
  void setStreamPublic(bool state);
  QString extension() const;
  void setExtension(const QString &str);
  QString formatIdentifier() const;
  void setFormatIdentifier(const QString &str);
  virtual void connectToServer();
  void stop();
  virtual void reset()=0;
  QString scriptUp() const;
  void setScriptUp(const QString &cmd);
  QString scriptDown() const;
  void setScriptDown(const QString &cmd);
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
  static QString timezoneOffset();
  static QString socketErrorText(QAbstractSocket::SocketError err);

 signals:
  void connected(bool state);
  void dataReceived(const QByteArray &data);
  void error(QAbstractSocket::SocketError err);
  void streamMetadataChanged(const QString &str);

 protected:
  void setConnected(bool state);
  void setError(QAbstractSocket::SocketError err);
  virtual void connectToHostConnector(const QString &hostname,uint16_t port)=0;
  virtual void disconnectFromHostConnector()=0;
  QString hostHostname() const;
  uint16_t hostPort() const;


 private:
  QString conn_server_username;
  QString conn_server_password;
  QUrl conn_server_url;
  QString conn_content_type;
  std::vector<unsigned> conn_audio_bitrates;
  QString conn_stream_name;
  QString conn_stream_description;
  QString conn_stream_url;
  QString conn_stream_irc;
  QString conn_stream_icq;
  QString conn_stream_aim;
  QString conn_stream_genre;
  bool conn_stream_metadata_enabled;
  QString conn_stream_metadata;
  bool conn_stream_public;
  QString conn_extension;
  QString conn_format_identifier;
  QString conn_host_hostname;
  uint16_t conn_host_port;
  QString conn_script_up;
  QString conn_script_down;
  bool conn_connected;
};


#endif  // CONNECTOR_H
