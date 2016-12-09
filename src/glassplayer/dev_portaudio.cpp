// dev_portaudio.cpp
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

#include <stdio.h>

#include "dev_portaudio.h"

#ifdef PORTAUDIO
int __PortAudioCallback(const void *input,void *output,unsigned long frames,
			const PaStreamCallbackTimeInfo *ti,
			PaStreamCallbackFlags flags,void *priv)
{
  DevPortAudio *dev=(DevPortAudio *)priv;

  dev->codec()->ring()->read((float *)output,frames);

  return paContinue;
}
#endif  // PORTAUDIO


DevPortAudio::DevPortAudio(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
#ifdef PORTAUDIO
  PaError pa_err;

  portaudio_device_index=0;

  if((pa_err=Pa_Initialize())<0) {
    fprintf(stderr,"glassplayer: unable to initialize PortAudio [%s]\n",
	    Pa_GetErrorText(pa_err));
    exit(10);
  }
#endif  // PORTAUDIO
}


DevPortAudio::~DevPortAudio()
{
#ifdef PORTAUDIO
  Pa_Terminate();
#endif  // PORTAUDIO
}


bool DevPortAudio::isAvailable() const
{
#ifdef PORTAUDIO
  return true;
#else
  return false;
#endif  // PORTAUDIO
}


bool DevPortAudio::processOptions(QString *err,const QStringList &keys,
				  const QStringList &values)
{
#ifdef PORTAUDIO
  bool ok=false;

  for(int i=0;i<keys.size();i++) {
    bool processed=false;
    if(keys.at(i)=="--portaudio-index") {
      portaudio_device_index=values.at(i).toUInt(&ok);
      if((!ok)||(portaudio_device_index>=Pa_GetDeviceCount())) {
	*err=tr("invalid --portaudio-index value");
	return false;
      }
      processed=true;
    }
    if(!processed) {
      *err=tr("unrecognized option")+" "+keys[i]+"\"";
      return false;
    }
  }

  return true;
#else
  return false;
#endif  // PORTAUDIO
}


bool DevPortAudio::start(QString *err)
{
#ifdef PORTAUDIO
  PaError pa_err;
  PaStreamParameters params;
  /*
  for(int i=0;i<Pa_GetDeviceCount();i++) {
    const PaDeviceInfo *info=Pa_GetDeviceInfo(i);
    printf("%d: %s\n",i,info->name);
  }
  */
  params.device=portaudio_device_index;
  params.channelCount=codec()->channels();
  params.sampleFormat=paFloat32;
  params.suggestedLatency=0.5;
  params.hostApiSpecificStreamInfo=NULL;
  if((pa_err=Pa_OpenStream(&portaudio_stream,NULL,&params,codec()->samplerate(),
			   paFramesPerBufferUnspecified,paNoFlag,
			   __PortAudioCallback,this))<0) {
    portaudio_stream=NULL;
    *err=Pa_GetErrorText(pa_err);
    return false;
  }
  if((pa_err=Pa_StartStream(portaudio_stream))<0) {
    *err=Pa_GetErrorText(pa_err);
    return false;
  }

  return true;
#else
  return false;
#endif  // PORTAUDIO
}


void DevPortAudio::stop()
{
#ifdef PORTAUDIO
  if(portaudio_stream!=NULL) {
    Pa_StopStream(portaudio_stream);
    Pa_CloseStream(portaudio_stream);
  }
#endif  // PORTAUDIO
}


void DevPortAudio::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
}


void DevPortAudio::playPositionData()
{
}


void DevPortAudio::meterData()
{
}
