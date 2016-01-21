// dev_file.cpp
//
// Send audio to a WAV file.
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

#include <stdio.h>

#include "dev_file.h"

DevFile::DevFile(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
  file_format=AudioDevice::S16_LE;
  file_file_name="";
}


DevFile::~DevFile()
{
  stop();
}


bool DevFile::processOptions(QString *err,const QStringList &keys,
			     const QStringList &values)
{
  for(int i=0;i<keys.size();i++) {
    bool processed=false;
    if(keys[i]=="--file-format") {
      if(values[i].toLower()=="float") {
	file_format=AudioDevice::FLOAT;
	processed=true;
      }
      if(values[i].toLower()=="s16_le") {
	file_format=AudioDevice::S16_LE;
	processed=true;
      }
      if(values[i].toLower()=="s32_le") {
	file_format=AudioDevice::S32_LE;
	processed=true;
      }
      if(!processed) {
	*err=tr("invalid --file-format value");
	return false;
      }
    }
    if(keys[i]=="--file-name") {
      file_file_name=values[i];
      processed=true;
    }
    if(!processed) {
      *err=tr("unrecognized option")+" "+keys[i]+"\"";
      return false;
    }
  }
  if(file_file_name.isEmpty()) {
    char filename[256];

    scanf("%255s",filename);
    file_file_name=filename;
  }
  return true;
}


bool DevFile::start(QString *err)
{
  SF_INFO sf;

  if(file_file_name.isEmpty()) {
    *err=tr("no --file-name specified");
    return false;
  }

  memset(&sf,0,sizeof(sf));
  sf.samplerate=codec()->samplerate();
  sf.channels=codec()->channels();
  switch(file_format) {
  case AudioDevice::FLOAT:
    sf.format=SF_FORMAT_WAV|SF_FORMAT_FLOAT;
    break;

  case AudioDevice::S16_LE:
    sf.format=SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    break;

  case AudioDevice::S32_LE:
    sf.format=SF_FORMAT_WAV|SF_FORMAT_PCM_32;
    break;

  case AudioDevice::LastFormat:
    *err=tr("Internal Error");
    return false;
  }
  if((file_sndfile=sf_open(file_file_name.toUtf8(),SFM_WRITE,&sf))==NULL) {
    *err=sf_strerror(file_sndfile);
    return false;
  }

  return true;
}


void DevFile::stop()
{
  sf_close(file_sndfile);
}


void DevFile::synchronousWrite(unsigned frames)
{
  float pcm[frames*codec()->channels()];
  int n;

  n=codec()->ring()->read(pcm,frames);
  sf_writef_float(file_sndfile,pcm,n);
}
