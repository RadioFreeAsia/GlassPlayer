// audiodevice.h
//
// Abstract base class for audio output sources.
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

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <stdint.h>

#include <queue>
#include <vector>

#include <QObject>
#include <QStringList>

#include "codec.h"
#include "glasslimits.h"
#include "metaevent.h"

#define AUDIO_METER_INTERVAL 50

class AudioDevice : public QObject
{
  Q_OBJECT;
 public:
  enum Type {Stdout=0,Alsa=1,AsiHpi=2,File=3,Jack=4,LastType=5};
  enum Format {FLOAT=0,S16_LE=1,S32_LE=2,LastFormat=3};
  AudioDevice(Codec *codec,QObject *parent=0);
  ~AudioDevice();
  virtual bool isAvailable() const;
  virtual bool processOptions(QString *err,const QStringList &keys,
			      const QStringList &values)=0;
  virtual bool start(QString *err)=0;
  virtual void stop();
  virtual void getStats(QStringList *hdrs,QStringList *values,bool is_first);
  void meterLevels(int *lvls) const;
  static QString typeText(AudioDevice::Type type);
  static QString optionKeyword(AudioDevice::Type type);
  static AudioDevice::Type type(const QString &key);
  static QString formatString(AudioDevice::Format fmt);

 public slots:
  virtual void synchronousWrite(unsigned frames,bool is_last);
  void processMetadata(uint64_t frames,MetaEvent *e);

 signals:
  void hasStopped();
  void metadataReceived(MetaEvent *e);

 protected:
  void setMeterLevels(float *lvls);
  void setMeterLevels(int *lvls);
  void updateMeterLevels(int *lvls);
  void updatePlayPosition(uint64_t frames);
  Codec *codec();
  void remixChannels(float *pcm_out,unsigned chans_out,
		     float *pcm_in,unsigned chans_in,unsigned nframes); 
  void convertFromFloat(int16_t *pcm_out,const float *pcm_in,
			unsigned nframes,unsigned chans);
  void convertFromFloat(int32_t *pcm_out,const float *pcm_in,
			unsigned nframes,unsigned chans);
  void peakLevels(float *lvls,const float *pcm,unsigned nframes,unsigned chans);
  void peakLevels(int *lvls,const float *pcm,unsigned nframes,unsigned chans);
  virtual void loadStats(QStringList *hdrs,QStringList *values,bool is_first)=0;

 private:
  Codec *audio_codec;
  int audio_meter_levels[MAX_AUDIO_CHANNELS];
  uint64_t audio_play_position;
  bool audio_play_position_changed;
  unsigned audio_ring_read_space_prev;
  std::queue<uint64_t> audio_metadata_frames;
  std::queue<MetaEvent *> audio_metadata_events;
};


#endif  // AUDIODEVICE_H
