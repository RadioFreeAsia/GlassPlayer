
// glassplayer.h
//
// glassplayer(1) Audio Player
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

#ifndef GLASSPLAYER_H
#define GLASSPLAYER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#include "audiodevice.h"
#include "codec.h"
#include "connector.h"
#include "ringbuffer.h"
#include "serverid.h"

#define GLASSPLAYER_USAGE "[options] stream-url\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void serverTypeFoundData(Connector::ServerType type,const QString &mimetype,
			   const QUrl &url);
  void serverConnectedData(bool state);
  void codecFramedData(unsigned chans,unsigned samprate,unsigned bitrate,
		       Ringbuffer *ring);
  void metadataReceivedData(MetaEvent *e);
  void starvationData();
  void statsData();
  void meterData();
  void exitData();

 private:
  void ListCodecs();
  void ListDevices();
  void RunScript(const QString &cmd);
  Connector::ServerType server_type;
  AudioDevice::Type audio_device_type;
  QUrl server_url;
  bool dump_bitstream;
  unsigned pregap;
  QString post_data;
  bool sir_stats_out;
  bool sir_metadata_out;
  QString sir_server_script_up;
  QString sir_server_script_down;
  QString sir_user;
  QString sir_password;
  QStringList device_keys;
  QStringList device_values;
  bool list_codecs;
  bool list_devices;
  Ringbuffer *sir_ring;
  Codec *sir_codec;
  Connector *sir_connector;
  AudioDevice *sir_audio_device;
  QTimer *sir_starvation_timer;
  ServerId *sir_server_id;
  QTimer *sir_stats_timer;
  MetaEvent sir_meta_event;
  bool sir_first_stats;
  bool sir_meter_data;
  QTimer *sir_meter_timer;
};


#endif  // GLASSPLAYER_H
