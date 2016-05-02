// dev_jack.h
//
// JACK audio device for glassplayer(1)
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

#ifndef DEV_JACK_H
#define DEV_JACK_H

#ifdef JACK
#include <jack/jack.h>
#endif  // JACK

#include <samplerate.h>

#include <QTimer>

#include "audiodevice.h"
#include "meteraverage.h"

#define DEFAULT_JACK_CLIENT_NAME "glassplayer"

class DevJack : public AudioDevice
{
  Q_OBJECT;
 public:
  DevJack(Codec *codec,QObject *parent=0);
  ~DevJack();
  bool isAvailable() const;
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private slots:
  void playPositionData();
  void meterData();

 private:
#ifdef JACK
  QString jack_server_name;
  QString jack_client_name;
  jack_client_t *jack_jack_client;
  jack_nframes_t jack_jack_sample_rate;
  jack_nframes_t jack_buffer_size;
  jack_port_t *jack_jack_ports[MAX_AUDIO_CHANNELS];
  MeterAverage *jack_meter_avg[MAX_AUDIO_CHANNELS];
  QTimer *jack_meter_timer;
  friend int JackBufferSizeChanged(jack_nframes_t frames, void *arg);
  friend int JackProcess(jack_nframes_t nframes, void *arg);
  SRC_STATE *jack_src;
  SRC_DATA jack_data;
  double jack_pll_offset;
  double jack_pll_setpoint_ratio;
  unsigned jack_pll_setpoint_frames;
  uint64_t jack_play_position;
  QTimer *jack_play_position_timer;
  bool jack_started;
#endif  // JACK
};


#endif  // DEV_JACK_H
