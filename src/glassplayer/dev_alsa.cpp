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

#include <samplerate.h>

#include "dev_alsa.h"
#include "logging.h"

void *AlsaCallback(void *ptr)
{
#ifdef ALSA
  static DevAlsa *dev=NULL;
  static float pcm_s1[ALSA_MAX_CARD_BUFFER];
  static float *pcm_s2;
  static float *pcm_s3;
  static int16_t pcm16[ALSA_MAX_CARD_BUFFER];
  static int32_t pcm32[ALSA_MAX_CARD_BUFFER];
  static int n;
  static SRC_STATE *src=NULL;
  static SRC_DATA data;
  static int err;
  static unsigned ring_frames=0;
  static unsigned count=0;
  static bool show_xrun=false;
  static double pll_setpoint_ratio;
  static float lvls[MAX_AUDIO_CHANNELS];
  static unsigned i;

  dev=(DevAlsa *)ptr;
  src=NULL;
  pll_setpoint_ratio=1.0;
  dev->alsa_pll_setpoint_frames=0;
  dev->alsa_pll_offset=0.0;
  ring_frames=0;
  count=0;
  show_xrun=false;
  dev->alsa_play_position=0;

  //
  // Initialize sample rate converter
  //
  pcm_s2=new float[262144];
  memset(&data,0,sizeof(data));
  data.data_in=pcm_s1;
  data.data_out=pcm_s2;
  data.output_frames=262144/dev->alsa_channels;
  pll_setpoint_ratio=
    (double)dev->alsa_samplerate/(double)dev->codec()->samplerate();
  data.src_ratio=pll_setpoint_ratio;
  if((src=src_new(SRC_LINEAR,dev->codec()->channels(),&err))==NULL) {
    fprintf(stderr,"SRC initialization error [%s]\n",src_strerror(err));
    exit(GLASS_EXIT_SRC_ERROR);
  }

  //
  // Initialize channel mixdown buffer
  //
  if(dev->codec()->channels()==dev->alsa_channels) {
    pcm_s3=pcm_s2;
  }
  else {
    pcm_s3=new float[32768];
  }

  //
  // Wait for PCM buffer to fill
  //
  while((!dev->alsa_stopping)&&(!dev->codec()->ring()->isFinished())&&
	(dev->codec()->ring()->readSpace()<2*dev->codec()->samplerate())) {
    usleep(36);
  }

  while(count<PLL_SETTLE_INTERVAL) {  // Allow ringbuffer to stabilize
    if((ring_frames=dev->codec()->ring()->readSpace())>dev->alsa_pll_setpoint_frames) {
      dev->alsa_pll_setpoint_frames=ring_frames;
    }
    dev->alsa_pll_setpoint_frames=ring_frames;
    count++;
  }
  while((!dev->alsa_stopping)&&(dev->codec()->ring()->readSpace()>0)) {
    ring_frames=dev->codec()->ring()->readSpace();
    if(ring_frames>dev->alsa_pll_setpoint_frames) {
      if(dev->alsa_pll_offset>(-PLL_CORRECTION_LIMIT)) {
	dev->alsa_pll_offset-=PLL_CORRECTION;
      }
    }
    else {
      if(dev->alsa_pll_offset<(PLL_CORRECTION_LIMIT)) {
	dev->alsa_pll_offset+=PLL_CORRECTION;
      }
    }
    data.src_ratio=pll_setpoint_ratio+dev->alsa_pll_offset;
    src_set_ratio(src,data.src_ratio);
    if(snd_pcm_state(dev->alsa_pcm)!=SND_PCM_STATE_RUNNING) {
      if(show_xrun) {
	fprintf(stderr,"*** XRUN ***\n");
	snd_pcm_drop(dev->alsa_pcm);
	snd_pcm_prepare(dev->alsa_pcm);
	show_xrun=false;
      }
    }
    else {
      show_xrun=!dev->codec()->ring()->isFinished();
    }
    if((n=dev->codec()->ring()->
	read(pcm_s1,dev->alsa_buffer_size/(dev->alsa_period_quantity*2)))>0) {
      if(src!=NULL) {
	data.input_frames=n;
	dev->alsa_play_position+=n;
	if((err=src_process(src,&data))<0) {
	  fprintf(stderr,"SRC processing error [%s]\n",src_strerror(err));
	  exit(GLASS_EXIT_SRC_ERROR);
	}
	n=data.output_frames_gen;
      }
      if(dev->codec()->channels()!=dev->alsa_channels) {
	dev->remixChannels(pcm_s3,dev->alsa_channels,
			   pcm_s2,dev->codec()->channels(),n);
      }
      switch(dev->alsa_format) {
      case AudioDevice::S16_LE:
	memset(pcm16,0,ALSA_MAX_CARD_BUFFER*sizeof(int16_t));
	dev->convertFromFloat(pcm16,pcm_s3,n,dev->alsa_channels);

	//
	// Work around an ALSA bug that appends garbage to the end of
	// the final PCM block
	//
	if(dev->codec()->ring()->readSpace()==0) {
	  n=dev->alsa_buffer_size;
	}

	snd_pcm_writei(dev->alsa_pcm,pcm16,n);
	break;

      case AudioDevice::S32_LE:
	memset(pcm32,0,ALSA_MAX_CARD_BUFFER*sizeof(int32_t));
	dev->convertFromFloat(pcm32,pcm_s3,n,dev->alsa_channels);

	//
	// Work around an ALSA bug that appends garbage to the end of
	// the final PCM block
	//
	if(dev->codec()->ring()->readSpace()==0) {
	  n=dev->alsa_buffer_size;
	}

	snd_pcm_writei(dev->alsa_pcm,pcm32,n);
	break;

      case AudioDevice::FLOAT:
	snd_pcm_writei(dev->alsa_pcm,pcm_s3,n);
	break;

      case AudioDevice::LastFormat:
	break;
      }
      dev->peakLevels(lvls,pcm_s3,n,dev->codec()->channels());
      for(i=0;i<dev->codec()->channels();i++) {
	dev->alsa_meter_avg[i]->addValue(lvls[i]);
      }
      dev->setMeterLevels(lvls);
    }
  }

  //
  // Shutdown
  //
  snd_pcm_drain(dev->alsa_pcm);
  snd_pcm_close(dev->alsa_pcm);
  if((pcm_s3!=pcm_s2)&&(pcm_s3!=pcm_s1)) {
    delete pcm_s3;
  }
  pcm_s3=NULL;
  if(pcm_s2!=pcm_s1) {
    delete pcm_s2;
  }
  pcm_s2=NULL;
#endif  // ALSA
  return NULL;
}


DevAlsa::DevAlsa(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
#ifdef ALSA
  alsa_device=ALSA_DEFAULT_DEVICE;
  alsa_pcm_buffer=NULL;
  alsa_stopping=false;

  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    alsa_meter_avg[i]=new MeterAverage(8);
  }
  alsa_meter_timer=new QTimer(this);
  connect(alsa_meter_timer,SIGNAL(timeout()),this,SLOT(meterData()));

  alsa_play_position_timer=new QTimer(this);
  connect(alsa_play_position_timer,SIGNAL(timeout()),
	  this,SLOT(playPositionData()));
#endif  // ALSA
}


DevAlsa::~DevAlsa()
{
#ifdef ALSA
  stop();
  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    delete alsa_meter_avg[i];
  }
  delete alsa_meter_timer;
  if(alsa_pcm_buffer!=NULL) {
    delete alsa_pcm_buffer;
  }
  delete alsa_play_position_timer;
#endif  // ALSA
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
    if(global_log_verbose) {
      Log(LOG_INFO,"using ALSA S32_LE sample format");
    }
  }
  else {
    if(snd_pcm_hw_params_set_format(alsa_pcm,hwparams,
				    SND_PCM_FORMAT_S16_LE)==0) {
      alsa_format=AudioDevice::S16_LE;
      if(global_log_verbose) {
	Log(LOG_INFO,"using ALSA S16_LE sample format");
      }
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
    if(global_log_verbose) {
      Log(LOG_INFO,
	  QString().sprintf("using ALSA sample rate of %u samples/sec",
			    alsa_samplerate));
    }
  }

  //
  // Channels
  //
  alsa_channels=codec()->channels();
  snd_pcm_hw_params_set_channels_near(alsa_pcm,hwparams,&alsa_channels);
  if(alsa_channels!=codec()->channels()) {
    if(global_log_verbose) {
      Log(LOG_INFO,
	  QString().sprintf("using ALSA channel count of %u",alsa_channels));
    }
  }

  //
  // Buffer Parameters
  //
  alsa_period_quantity=ALSA_PERIOD_QUANTITY;
  snd_pcm_hw_params_set_periods_near(alsa_pcm,hwparams,&alsa_period_quantity,
				     &dir);
  if(alsa_period_quantity!=ALSA_PERIOD_QUANTITY) {
    if(global_log_verbose) {
      Log(LOG_INFO,
	  QString().sprintf("using ALSA period quantity of %u",
			    alsa_period_quantity));
    }
  }
  alsa_buffer_size=alsa_samplerate/2;
  if(alsa_buffer_size>ALSA_MAX_CARD_BUFFER) {
    alsa_buffer_size=ALSA_MAX_CARD_BUFFER;
  }
  snd_pcm_hw_params_set_buffer_size_near(alsa_pcm,hwparams,&alsa_buffer_size);
  if(alsa_buffer_size!=(alsa_samplerate/2)) {
    if(global_log_verbose) {
      Log(LOG_INFO,
	  QString().sprintf("using ALSA buffer size of %lu frames",
			    alsa_buffer_size));
    }
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

  alsa_play_position_timer->start(50);
  alsa_meter_timer->start(AUDIO_METER_INTERVAL);

  return true;
#else
  return false;
#endif  // ALSA
}


void DevAlsa::stop()
{
#ifdef ALSA
  alsa_stopping=true;
  pthread_join(alsa_pthread,NULL);
#endif  // ALSA
}


void DevAlsa::playPositionData()
{
#ifdef ALSA
  updatePlayPosition(alsa_play_position);
#endif  // ALSA
}


void DevAlsa::meterData()
{
#ifdef ALSA
  float lvls[MAX_AUDIO_CHANNELS];

  for(unsigned i=0;i<codec()->channels();i++) {
    lvls[i]=alsa_meter_avg[i]->average();
  }
  setMeterLevels(lvls);
#endif  // ALSA
}


void DevAlsa::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
#ifdef ALSA
  if(is_first) {
    hdrs->push_back("Device|Type");
    values->push_back("ALSA");

    hdrs->push_back("Device|Name");
    values->push_back(alsa_device);

    hdrs->push_back("Device|Channels");
    values->push_back(QString().sprintf("%u",alsa_channels));

    hdrs->push_back("Device|Samplerate");
    values->push_back(QString().sprintf("%u",alsa_samplerate));

    hdrs->push_back("Device|Buffer Size");
    values->push_back(QString().sprintf("%lu",alsa_buffer_size));

    hdrs->push_back("Device|Period Quantity");
    values->push_back(QString().sprintf("%u",alsa_period_quantity));
  }

  hdrs->push_back("Device|PLL Offset");
  values->push_back(QString().sprintf("%8.6lf",alsa_pll_offset));

  hdrs->push_back("Device|PLL Setpoint Frames");
  values->push_back(QString().sprintf("%u",alsa_pll_setpoint_frames));
#endif  // ALSA
}
