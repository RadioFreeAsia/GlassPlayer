// audiodevicefactory.cpp
//
// Instantiate AudioDevice classes.
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

#include "dev_alsa.h"
#include "dev_file.h"
#include "dev_jack.h"
#include "dev_mme.h"
#include "dev_stdout.h"

#include "audiodevicefactory.h"

AudioDevice *AudioDeviceFactory(AudioDevice::Type type,Codec *codec,QObject *parent)
{
  AudioDevice *audiodevice=NULL;

  switch(type) {
  case AudioDevice::Stdout:
    audiodevice=new DevStdout(codec,parent);
    break;

  case AudioDevice::Alsa:
#ifdef ALSA
    audiodevice=new DevAlsa(codec,parent);
#endif  // ALSA
    break;
 
  case AudioDevice::AsiHpi:
    break;

  case AudioDevice::File:
    audiodevice=new DevFile(codec,parent);
    break;

  case AudioDevice::Jack:
#ifdef JACK
    audiodevice=new DevJack(codec,parent);
#endif  // JACK
    break;

  case AudioDevice::Mme:
#ifdef MME
    audiodevice=new DevMme(codec,parent);
#endif  // MME
    break;

  case AudioDevice::LastType:
    break;
  }

  return audiodevice;
}
