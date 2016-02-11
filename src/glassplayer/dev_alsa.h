// dev_alsa.h
//
// ALSA audio device for glassplayer(1)
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

#ifndef DEV_ALSA_H
#define DEV_ALSA_H

#ifdef ALSA
#include <alsa/asoundlib.h>
#include <pthread.h>
#endif  // ALSA

#include <QTimer>

#include "audiodevice.h"

#define ALSA_DEFAULT_DEVICE "hw:0"
#define ALSA_PERIOD_QUANTITY 4

class DevAlsa : public AudioDevice
{
  Q_OBJECT;
 public:
  DevAlsa(Codec *codec,QObject *parent=0);
  ~DevAlsa();
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);
  void stop();

 private slots:
  void playPositionData();

 protected:
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
#ifdef ALSA
  QString alsa_device;
  snd_pcm_t *alsa_pcm;
  AudioDevice::Format alsa_format;
  unsigned alsa_samplerate;
  unsigned alsa_channels;
  unsigned alsa_period_quantity;
  snd_pcm_uframes_t alsa_buffer_size; 
  float *alsa_pcm_buffer;
  pthread_t alsa_pthread;
  bool alsa_stopping;
  //  MeterAverage *alsa_meter_avg[MAX_AUDIO_CHANNELS];
  //  QTimer *alsa_meter_timer;
  QTimer *alsa_play_position_timer;
  friend void *AlsaCallback(void *ptr);
  double alsa_pll_offset;
  unsigned alsa_pll_setpoint_frames;
  uint64_t alsa_play_position;
#endif  // ALSA
};


#endif  // DEV_ALSA_H
