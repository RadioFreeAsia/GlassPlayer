// dev_mme.h
//
// Windows MME audio device for glassplayer(1)
//
//   (C) Copyright 2016 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DEV_MME_H
#define DEV_MME_H

#ifdef MME
#include <windows.h>
#include <mmsystem.h>
#endif  // MME

#include <samplerate.h>

#include <QTimer>

#include "audiodevice.h"
#include "meteraverage.h"

#define MME_BUFFER_SIZE 2048
#define MME_PERIOD_QUAN 4

class DevMme : public AudioDevice
{
  Q_OBJECT;
 public:
  DevMme(Codec *codec,QObject *parent=0);
  ~DevMme();
  bool isAvailable() const;
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);
  void stop();
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private slots:
  void audioData();

 private:
#ifdef MME
  QString MmeError(MMRESULT err) const;
  QStringList mme_device_names;
  unsigned mme_device_id;
  HWAVEOUT mme_handle;
  WAVEHDR mme_headers[MME_PERIOD_QUAN];
  unsigned mme_current_header;
  QTimer *mme_audio_timer;
  uint64_t mme_frames_played;
  float *mme_pcm_in;
  float *mme_pcm_out;
#endif  // MME
};


#endif  // DEV_MME_H
