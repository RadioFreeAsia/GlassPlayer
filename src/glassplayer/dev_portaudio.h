// dev_portaudio.h
//
// PortAudio audio device for glassplayer(1)
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

#ifndef DEV_PORTAUDIO_H
#define DEV_PORTAUDIO_H

#ifdef PORTAUDIO
#include <portaudio.h>
#endif  // PORTAUDIO

#include <samplerate.h>

#include <QTimer>

#include "audiodevice.h"
#include "meteraverage.h"

class DevPortAudio : public AudioDevice
{
  Q_OBJECT;
 public:
  DevPortAudio(Codec *codec,QObject *parent=0);
  ~DevPortAudio();
  bool isAvailable() const;
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);
  void stop();
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private slots:
  void playPositionData();
  void meterData();

 private:
#ifdef PORTAUDIO
  PaStream *portaudio_stream;
  PaDeviceIndex portaudio_device_index;
  friend int __PortAudioCallback(const void *input,void *output,
				 unsigned long frames,
				 const PaStreamCallbackTimeInfo *ti,
				 PaStreamCallbackFlags flags,void *priv);
#endif  // PORTAUDIO
};


#endif  // DEV_PORTAUDIO_H
