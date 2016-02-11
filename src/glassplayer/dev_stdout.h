// dev_stdout.h
//
// Send audio to standard output.
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

#ifndef DEV_STDOUT_H
#define DEV_STDOUT_H

#include "audiodevice.h"

class DevStdout : public AudioDevice
{
  Q_OBJECT;
 public:
  DevStdout(Codec *codec,QObject *parent=0);
  ~DevStdout();
  bool processOptions(QString *err,const QStringList &keys,
		      const QStringList &values);
  bool start(QString *err);

 public slots:
   void synchronousWrite(unsigned frames,bool is_last);

 protected:
   void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
  AudioDevice::Format stdout_format;
};


#endif  // DEV_STDOUT_H
