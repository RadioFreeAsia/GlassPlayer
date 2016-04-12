// dev_jack.cpp
//
// JACK audio device for glassplayer(1)
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

#include "dev_jack.h"
#include "logging.h"

//
// JACK Callback
//
#ifdef JACK
jack_default_audio_sample_t *jack_cb_buffers[MAX_AUDIO_CHANNELS];
float jack_cb_interleave_buffer[MAX_AUDIO_CHANNELS*RINGBUFFER_SIZE];

int JackBufferSizeChanged(jack_nframes_t frames,void *arg)
{
  static DevJack *dev=(DevJack *)arg;
  dev->jack_buffer_size=frames;

  return 0;
}


int JackProcess(jack_nframes_t nframes, void *arg)
{
  DevJack *dev=(DevJack *)arg;
  static unsigned i;
  static jack_nframes_t j;
  static float pcm_s1[MAX_AUDIO_CHANNELS*RINGBUFFER_SIZE];
  static float pcm_s2[MAX_AUDIO_CHANNELS*RINGBUFFER_SIZE];
  static unsigned ring_frames;
  static unsigned n;
  static int err;
  static unsigned pcm_start=0;
  static int pcm_offset=0;
  static float lvls[MAX_AUDIO_CHANNELS];

  //
  // Wait for PCM Buffer to Fill
  //
  if(!dev->jack_started) {
    if(dev->codec()->ring()->readSpace()<2*dev->codec()->samplerate()) {
      return 0;
    }
    dev->jack_pll_setpoint_frames=2*dev->codec()->samplerate();
    dev->jack_started=true;
  }

  //
  // Update PLL
  //
  ring_frames=dev->codec()->ring()->readSpace();
  if(ring_frames>dev->jack_pll_setpoint_frames) {
    if(dev->jack_pll_offset>(-PLL_CORRECTION_LIMIT)) {
      dev->jack_pll_offset-=PLL_CORRECTION;
    }
  }
  else {
    if(dev->jack_pll_offset<(PLL_CORRECTION_LIMIT)) {
      dev->jack_pll_offset+=PLL_CORRECTION;
    }
  }
  dev->jack_data.src_ratio=dev->jack_pll_setpoint_ratio+dev->jack_pll_offset;
  src_set_ratio(dev->jack_src,dev->jack_data.src_ratio);

  //
  // Get Buffers
  //
  for(i=0;i<dev->codec()->channels();i++) {
    jack_cb_buffers[i]=(jack_default_audio_sample_t *)
      jack_port_get_buffer(dev->jack_jack_ports[i],nframes);
  }

  //
  // Read Codec Output
  //
  n=nframes/dev->jack_data.src_ratio+pcm_offset;
  if(dev->codec()->ring()->readSpace()>=n) {
    n=dev->codec()->ring()->read(pcm_s1,n);
    //
    // SRC
    //
    dev->jack_data.data_in=pcm_s1;
    dev->jack_data.input_frames=n;
    dev->jack_data.data_out=pcm_s2+pcm_start*dev->codec()->channels();
    dev->jack_data.output_frames=
      MAX_AUDIO_CHANNELS*RINGBUFFER_SIZE/dev->codec()->channels()-pcm_start;
    dev->jack_play_position+=n;
    if((err=src_process(dev->jack_src,&dev->jack_data))<0) {
      fprintf(stderr,"SRC processing error [%s]\n",src_strerror(err));
      exit(GLASS_EXIT_SRC_ERROR);
    }
    n=dev->jack_data.output_frames_gen+pcm_start;

    //
    // De-interleave Channels and Write to Jack Buffers
    //
    for(i=0;i<dev->codec()->channels();i++) {
      if(jack_cb_buffers[i]!=NULL) {
	for(j=0;j<nframes;j++) {
	  jack_cb_buffers[i][j]=pcm_s2[dev->codec()->channels()*j+i];
	}
      }
    }

    //
    // Update Meters
    //
    dev->peakLevels(lvls,pcm_s2,nframes,dev->codec()->channels());
    for(i=0;i<dev->codec()->channels();i++) {
      dev->jack_meter_avg[i]->addValue(lvls[i]);
    }
    dev->setMeterLevels(lvls);

    //
    // Move left-overs to start of buffer
    //
    if(n>nframes) {
      pcm_start=n-nframes;
      for(i=0;i<pcm_start;i++) {
	for(j=0;j<dev->codec()->channels();j++) {
	  pcm_s2[dev->codec()->channels()*i+j]=
	    pcm_s2[(nframes+i)*dev->codec()->channels()+j];
	}
      }
    }
    else {
      pcm_start=0;
    }
    if(pcm_start<2) {
      pcm_offset=1;
    }
    else {
      pcm_offset=0;
    }
  }

  return 0;
}
#endif  // JACK


DevJack::DevJack(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
#ifdef JACK
  jack_jack_client=NULL;
  jack_src=NULL;
  jack_server_name="";
  jack_client_name=DEFAULT_JACK_CLIENT_NAME;
  jack_started=false;

  //
  // Metering
  //
  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    jack_meter_avg[i]=new MeterAverage(8);
  }
  jack_meter_timer=new QTimer(this);
  connect(jack_meter_timer,SIGNAL(timeout()),this,SLOT(meterData()));

  jack_play_position_timer=new QTimer(this);
  connect(jack_play_position_timer,SIGNAL(timeout()),
	  this,SLOT(playPositionData()));
#endif  // JACK
}


DevJack::~DevJack()
{
#ifdef JACK
  if(jack_jack_client!=NULL) {
    jack_deactivate(jack_jack_client);
    jack_client_close(jack_jack_client);
  }
  delete jack_play_position_timer;
  delete jack_meter_timer;
  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    delete jack_meter_avg[i];
  }
  if(jack_src!=NULL) {
    src_delete(jack_src);
  }
  
#endif  // JACK
}


bool DevJack::isAvailable() const
{
  return true;
}


bool DevJack::processOptions(QString *err,const QStringList &keys,
			     const QStringList &values)
{
#ifdef JACK
  for(int i=0;i<keys.size();i++) {
    bool processed=false;
    if(keys[i]=="--jack-server-name") {
      jack_server_name=values[i];
      processed=true;
    }
    if(keys[i]=="--jack-client-name") {
      jack_client_name=values[i];
      processed=true;
    }
    if(!processed) {
      *err=tr("unrecognized option")+" "+keys[i]+"\"";
      return false;
    }
  }
  return true;
#else
  *err=tr("device not supported");
  return false;
#endif  // JACK
}


bool DevJack::start(QString *err)
{
#ifdef JACK
  jack_options_t jackopts=JackNullOption;
  jack_status_t jackstat=JackFailure;
  int srcerr;

  //
  // Connect to JACK Instance
  //
  if(jack_server_name.isEmpty()) {
    jack_jack_client=
      jack_client_open(jack_client_name.toAscii(),jackopts,&jackstat);
  }
  else {
    jack_jack_client=
      jack_client_open(jack_client_name.toAscii(),jackopts,&jackstat,
		       (const char *)jack_server_name.toAscii());
  }
  if(jack_jack_client==NULL) {
    if((jackstat&JackInvalidOption)!=0) {
      *err=tr("invalid or unsupported JACK option");
    }
    if((jackstat&JackServerError)!=0) {
      *err=tr("communication error with the JACK server");
    }
    if((jackstat&JackNoSuchClient)!=0) {
      *err=tr("requested JACK client does not exist");
    }
    if((jackstat&JackLoadFailure)!=0) {
      *err=tr("unable to load internal JACK client");
    }
    if((jackstat&JackInitFailure)!=0) {
      *err=tr("unable to initialize JACK client");
    }
    if((jackstat&JackShmFailure)!=0) {
      *err=tr("unable to access JACK shared memory");
    }
    if((jackstat&JackVersionError)!=0) {
      *err=tr("JACK protocol version mismatch");
    }
    if((jackstat&JackServerStarted)!=0) {
      *err=tr("JACK server started");
    }
    if((jackstat&JackServerFailed)!=0) {
      fprintf (stderr, "unable to communication with JACK server\n");
      *err=tr("unable to communicate with JACK server");
    }
    if((jackstat&JackNameNotUnique)!=0) {
      *err=tr("JACK client name not unique");
    }
    if((jackstat&JackFailure)!=0) {
      *err=tr("JACK general failure");
    }
    *err=tr("no connection to JACK server");
  printf("~DevJack::start() Ends FALSE 1\n");
    return false;
  }
  jack_set_buffer_size_callback(jack_jack_client,JackBufferSizeChanged,this);
  jack_set_process_callback(jack_jack_client,JackProcess,this);

  //
  // Join the Graph
  //
  if(jack_activate(jack_jack_client)) {
    *err=tr("unable to join JACK graph");
  printf("~DevJack::start() Ends FALSE 2\n");
    return false;
  }
  jack_jack_sample_rate=jack_get_sample_rate(jack_jack_client);

  //
  // Register Ports
  //
  for(unsigned i=0;i<codec()->channels();i++) {
    QString name=QString().sprintf("output_%d",i+1);
    jack_jack_ports[i]=
      jack_port_register(jack_jack_client,name.toAscii(),JACK_DEFAULT_AUDIO_TYPE,
			 JackPortIsOutput|JackPortIsTerminal,0);
  }
  Log(LOG_INFO,QString().sprintf("connected to JACK graph at %u samples/sec.",
				 jack_jack_sample_rate));

  //  jack_meter_timer->start(AUDIO_METER_INTERVAL);

  //
  // Initialize SRC
  //
  jack_pll_setpoint_ratio=
    (double)jack_jack_sample_rate/(double)codec()->samplerate();
  memset(&jack_data,0,sizeof(jack_data));
  jack_data.src_ratio=jack_pll_setpoint_ratio;
  if((jack_src=src_new(SRC_LINEAR,codec()->channels(),&srcerr))==NULL) {
    fprintf(stderr,"SRC initialization error [%s]\n",src_strerror(srcerr));
    exit(GLASS_EXIT_SRC_ERROR);
  }
  jack_play_position=0;
  jack_pll_setpoint_frames=0;
  jack_pll_offset=0.0;

  jack_play_position_timer->start(50);
  jack_meter_timer->start(AUDIO_METER_INTERVAL);

  return true;

#endif  // JACK
  return false;
}


void DevJack::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
  if(is_first) {
    hdrs->push_back("Device|Type");
    values->push_back("JACK");

    hdrs->push_back("Device|Name");
    values->push_back(jack_client_name);

    hdrs->push_back("Device|Samplerate");
    values->push_back(QString().sprintf("%u",jack_jack_sample_rate));

    hdrs->push_back("Device|Buffer Size");
    values->push_back(QString().sprintf("%u",jack_buffer_size));
  }
  hdrs->push_back("Device|Frames Played");
  values->push_back(QString().sprintf("%lu",jack_play_position));

  hdrs->push_back("Device|PLL Offset");
  values->push_back(QString().sprintf("%8.6lf",jack_pll_offset));

  hdrs->push_back("Device|PLL Setpoint Frames");
  values->push_back(QString().sprintf("%u",jack_pll_setpoint_frames));
}


void DevJack::playPositionData()
{
  updatePlayPosition(jack_play_position);
}


void DevJack::meterData()
{
  float lvls[MAX_AUDIO_CHANNELS];

  for(unsigned i=0;i<codec()->channels();i++) {
    lvls[i]=jack_meter_avg[i]->average();
  }
  setMeterLevels(lvls);
}
