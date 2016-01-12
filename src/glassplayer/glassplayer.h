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
#include <QUrl>

#include "audiodevice.h"
#include "codec.h"
#include "connector.h"
#include "ringbuffer.h"

#define GLASSPLAYER_USAGE "[options]\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void serverConnectedData(bool state);
  void codecFramedData(unsigned chans,unsigned samprate,unsigned bitrate,
		       Ringbuffer *ring);
  void streamMetadataChangedData(const QString &str);

 private:
  void StartServerConnection();
  Connector::ServerType server_type;
  AudioDevice::Type audio_device_type;
  QUrl server_url;
  bool dump_bitstream;
  QStringList device_keys;
  QStringList device_values;
  Ringbuffer *sir_ring;
  Codec *sir_codec;
  Connector *sir_connector;
  AudioDevice *sir_audio_device;
};


#endif  // GLASSPLAYER_H
