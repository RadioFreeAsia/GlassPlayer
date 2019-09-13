// dev_file.h
//
// Send audio to a WAV file.
//
//   (C) Copyright 2014-2019 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef DEV_FILE_H
#define DEV_FILE_H

#include <sndfile.h>

#include "audiodevice.h"

class DevFile : public AudioDevice
{
  Q_OBJECT;
 public:
  DevFile(unsigned pregap,Codec *codec,QObject *parent=0);
  ~DevFile();
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);
  void stop();

 public slots:
   void synchronousWrite(unsigned frames,bool is_last);

 protected:
   void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
  AudioDevice::Format file_format;
  QString file_file_name;
  SNDFILE *file_sndfile;
  uint64_t file_frames_processed;
};


#endif  // DEV_FILE_H
