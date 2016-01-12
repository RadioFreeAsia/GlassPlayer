// dev_stdout.cpp
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

#include "dev_stdout.h"

#include <unistd.h>

DevStdout::DevStdout(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
}


DevStdout::~DevStdout()
{
}


bool DevStdout::processOptions(QString *err,const QStringList &keys,
			       const QStringList &values)
{
  return true;
}


bool DevStdout::start(QString *err)
{
  return true;
}


void DevStdout::synchronousWrite(unsigned frames)
{
  float pcm[frames*codec()->channels()];

  codec()->ring()->read(pcm,frames);
  write(1,pcm,frames*codec()->channels()*sizeof(float));
}
