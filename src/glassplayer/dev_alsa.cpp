// dev_alsa.cpp
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

#include "dev_alsa.h"
#include "logging.h"

void *AlsaCallback(void *ptr)
{
#ifdef ALSA
  static DevAlsa *dev=(DevAlsa *)ptr;
  static float pcm_in[32768];
  static float pcm_out[32768];
  static int16_t pcm16[32768];
  static int32_t pcm32[32768];
  static int n;

  while(1==1) {
    if((n=dev->codec()->ring()->
	read(pcm_in,dev->alsa_buffer_size/(dev->alsa_period_quantity*2)))>0) {
      dev->remixChannels(pcm_out,dev->alsa_channels,pcm_in,dev->codec()->channels(),n);
      switch(dev->alsa_format) {
      case AudioDevice::S16_LE:
	dev->convertFromFloat(pcm16,pcm_out,n,dev->alsa_channels);
	snd_pcm_writei(dev->alsa_pcm,pcm16,n);
	break;

      case AudioDevice::S32_LE:
	dev->convertFromFloat(pcm32,pcm_out,n,dev->alsa_channels);
	snd_pcm_writei(dev->alsa_pcm,pcm32,n);
	break;

      case AudioDevice::FLOAT:
	snd_pcm_writei(dev->alsa_pcm,pcm_out,n);
	break;

      case AudioDevice::LastFormat:
	break;
      }
    }
  }

#endif  // ALSA
  return NULL;
}


DevAlsa::DevAlsa(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
#ifdef ALSA
  alsa_device=ALSA_DEFAULT_DEVICE;
  alsa_pcm_buffer=NULL;
#endif  // ALSA
}


DevAlsa::~DevAlsa()
{
}


bool DevAlsa::processOptions(QString *err,const QStringList &keys,
			     const QStringList &values)
{
#ifdef ALSA
  for(int i=0;i<keys.size();i++) {
    bool processed=false;
    if(keys[i]=="--alsa-device") {
      alsa_device=values[i];
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
#endif  // ALSA
}


bool DevAlsa::start(QString *err)
{
#ifdef ALSA
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;
  int dir;
  int aerr;
  pthread_attr_t pthread_attr;

  if(snd_pcm_open(&alsa_pcm,alsa_device.toUtf8(),
		  SND_PCM_STREAM_PLAYBACK,0)!=0) {
    *err=tr("unable to open ALSA device")+" \""+alsa_device+"\"";
    return false;
  }
  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_hw_params_any(alsa_pcm,hwparams);

  //
  // Access Type
  //
  if(snd_pcm_hw_params_test_access(alsa_pcm,hwparams,
				   SND_PCM_ACCESS_RW_INTERLEAVED)<0) {
    *err=tr("interleaved access not supported");
    return false;
  }
  snd_pcm_hw_params_set_access(alsa_pcm,hwparams,SND_PCM_ACCESS_RW_INTERLEAVED);

  //
  // Sample Format
  //
  if(snd_pcm_hw_params_set_format(alsa_pcm,hwparams,
				  SND_PCM_FORMAT_S32_LE)==0) {
    alsa_format=AudioDevice::S32_LE;
    Log(LOG_INFO,"using ALSA S32_LE sample format");
  }
  else {
    if(snd_pcm_hw_params_set_format(alsa_pcm,hwparams,
				    SND_PCM_FORMAT_S16_LE)==0) {
      alsa_format=AudioDevice::S16_LE;
      Log(LOG_INFO,"using ALSA S16_LE sample format");
    }
    else {
      *err=tr("incompatible sample format");
      return false;
    }
  }

  //
  // Sample Rate
  //
  alsa_samplerate=codec()->samplerate();
  snd_pcm_hw_params_set_rate_near(alsa_pcm,hwparams,&alsa_samplerate,&dir);
  if(alsa_samplerate!=codec()->samplerate()) {
    Log(LOG_INFO,
	QString().sprintf("using ALSA sample rate of %u samples/sec",
			  alsa_samplerate));
  }

  //
  // Channels
  //
  alsa_channels=codec()->channels();
  snd_pcm_hw_params_set_channels_near(alsa_pcm,hwparams,&alsa_channels);
  if(alsa_channels!=codec()->channels()) {
    Log(LOG_INFO,
	QString().sprintf("using ALSA channel count of %u",alsa_channels));
  }

  //
  // Buffer Parameters
  //
  alsa_period_quantity=ALSA_PERIOD_QUANTITY;
  snd_pcm_hw_params_set_periods_near(alsa_pcm,hwparams,&alsa_period_quantity,
				     &dir);
  if(alsa_period_quantity!=ALSA_PERIOD_QUANTITY) {
    Log(LOG_INFO,
	QString().sprintf("using ALSA period quantity of %u",
			  alsa_period_quantity));
  }
  alsa_buffer_size=alsa_samplerate/2;
  snd_pcm_hw_params_set_buffer_size_near(alsa_pcm,hwparams,&alsa_buffer_size);
  if(alsa_buffer_size!=(alsa_samplerate/2)) {
    Log(LOG_INFO,
	QString().sprintf("using ALSA buffer size of %lu frames",
			  alsa_buffer_size));
  }

  //
  // Fire It Up
  //
  if((aerr=snd_pcm_hw_params(alsa_pcm,hwparams))<0) {
    *err=tr("ALSA device error 1")+": "+snd_strerror(aerr);
    return false;
  }
  alsa_pcm_buffer=new float[alsa_buffer_size*alsa_channels];

  //
  // Set Wake-up Timing
  //
  snd_pcm_sw_params_alloca(&swparams);
  snd_pcm_sw_params_current(alsa_pcm,swparams);
  snd_pcm_sw_params_set_avail_min(alsa_pcm,swparams,alsa_buffer_size/2);
  if((aerr=snd_pcm_sw_params(alsa_pcm,swparams))<0) {
    *err=tr("ALSA device error 2")+": "+snd_strerror(aerr);
    return false;
  }

  //
  // Start the Callback
  //
  pthread_attr_init(&pthread_attr);

//  if(use_realtime) {
//    pthread_attr_setschedpolicy(&pthread_attr,SCHED_FIFO);

  pthread_create(&alsa_pthread,&pthread_attr,AlsaCallback,this);

  //  alsa_meter_timer->start(AUDIO_METER_INTERVAL);

  return true;
#else
  return false;
#endif  // ALSA
}
