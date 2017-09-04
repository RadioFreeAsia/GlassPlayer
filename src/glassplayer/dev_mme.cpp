// dev_mme.cpp
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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <samplerate.h>

#ifdef MME
#include <mmdeviceapi.h>
#endif  // MME

#include "dev_mme.h"
#include "logging.h"

#ifdef MME

void *__DevMmeWaveCallback(void *ptr)
{
#ifdef MME
  DevMme *dev=(DevMme *)ptr;
  static unsigned hdrptr;
  static Ringbuffer *rb=dev->codec()->ring();

  while(1==1) {
    hdrptr=dev->mme_current_header%MME_PERIOD_QUAN;
    while((!dev->mme_headers[hdrptr].dwUser)&&(rb->readSpace()>=MME_BUFFER_SIZE)) {
      rb->read(dev->mme_pcm_in,MME_BUFFER_SIZE);
      src_float_to_short_array(dev->mme_pcm_in,(short *)dev->mme_headers[hdrptr].lpData,
			       MME_BUFFER_SIZE*dev->codec()->channels());
      dev->mme_headers[hdrptr].dwUser=true;
      waveOutWrite(dev->mme_handle,&(dev->mme_headers[hdrptr]),
		   sizeof(dev->mme_headers[hdrptr]));
      hdrptr=++dev->mme_current_header%MME_PERIOD_QUAN;
      dev->mme_frames_played+=MME_BUFFER_SIZE;
    }
    usleep(40000);
  }
#endif // MME

  return NULL;
}


void CALLBACK __DevMmeCallback(HWAVEOUT hwo,UINT umsg,DWORD_PTR instance,  
			       DWORD_PTR param1,DWORD_PTR param2)
{
  WAVEHDR *hdr=(WAVEHDR *)param1;

  switch(umsg) {
  case WOM_DONE:
    hdr->dwUser=false;
    break;
  }
}
#endif  // MME


DevMme::DevMme(unsigned pregap,Codec *codec,QObject *parent)
  : AudioDevice(pregap,codec,parent)
{
#ifdef MME
  mme_device_id=0;
  mme_current_header=0;
  mme_frames_played=0;

  for(int i=0;i<MME_PERIOD_QUAN;i++) {
    memset(&(mme_headers[i]),0,sizeof(mme_headers[i]));
  }
  mme_pcm_in=new float[MME_BUFFER_SIZE*MAX_AUDIO_CHANNELS*2];
  mme_pcm_out=new float[MME_BUFFER_SIZE*MAX_AUDIO_CHANNELS*2];

  mme_audio_timer=new QTimer(this);
  connect(mme_audio_timer,SIGNAL(timeout()),this,SLOT(audioData()));

  for(unsigned i=0;i<waveOutGetNumDevs();i++) {
    WAVEOUTCAPS caps;
    waveOutGetDevCaps(i,&caps,sizeof(caps));
    mme_device_names.push_back(caps.szPname);
  }
#endif  // MME
}


DevMme::~DevMme()
{
#ifdef MME
  delete mme_pcm_out;
  delete mme_pcm_in;
  for(int i=0;i<MME_PERIOD_QUAN;i++) {
    if(mme_headers[i].lpData!=NULL) {
      delete mme_headers[i].lpData;
    }
  }
#endif  // MME
}


bool DevMme::isAvailable() const
{
#ifdef MME
  return true;
#else
  return false;
#endif  // MME
}


bool DevMme::processOptions(QString *err,const QStringList &keys,
				    const QStringList &values)
{
#ifdef MME
  for(int i=0;i<keys.size();i++) {
    bool processed=false;
    bool ok=false;
    if(keys[i]=="--mme-device-id") {
      mme_device_id=values[i].toUInt(&ok);
      if(ok) {
	if(mme_device_id>=(unsigned)mme_device_names.size()) {
	  *err=tr("no such MME device");
	  return false;
	}
	processed=true;
      }
      else {
	*err=tr("invalid --mme-device value");
	return false;
      }
    }
    if(!processed) {
      *err=tr("unrecognized option")+" "+keys[i]+"\"";
      return false;
    }
  }
  return true;
#else
  return false;
#endif  // MME
}


bool DevMme::start(QString *err)
{
#ifdef MME
  MMRESULT merr;
  WAVEFORMATEX wfx;
  pthread_attr_t pthread_attr;

  //
  // Open the output device
  //
  memset(&wfx,0,sizeof(wfx));
  wfx.wFormatTag=WAVE_FORMAT_PCM;
  wfx.nChannels=codec()->channels();
  wfx.nSamplesPerSec=codec()->samplerate();
  wfx.nBlockAlign=codec()->channels()*sizeof(int16_t);
  wfx.nAvgBytesPerSec=codec()->samplerate()*codec()->channels()*sizeof(int16_t);
  wfx.wBitsPerSample=sizeof(int16_t)*8;

  if((merr=waveOutOpen(&mme_handle,mme_device_id,&wfx,
		       (DWORD_PTR)__DevMmeCallback,
		       (DWORD_PTR)this,CALLBACK_FUNCTION))!=0) {
    *err=MmeError(merr);
    return false;
  }

  //
  // Allocate Buffers
  //
  for(int i=0;i<MME_PERIOD_QUAN;i++) {
    mme_headers[i].lpData=
      (LPSTR)new int16_t[MME_BUFFER_SIZE*codec()->channels()];
    mme_headers[i].dwBufferLength=
      MME_BUFFER_SIZE*codec()->channels()*sizeof(int16_t);
    if((merr=waveOutPrepareHeader(mme_handle,&(mme_headers[i]),sizeof(mme_headers[i])))!=0) {
      *err=MmeError(merr);
      return false;
    }
  }

  //
  // Start Playout
  //
  for(int i=0;i<MME_PERIOD_QUAN;i++) {
    memset(mme_headers[i].lpData,0,MME_BUFFER_SIZE*codec()->channels());
    mme_headers[i].dwUser=true;
    waveOutWrite(mme_handle,&(mme_headers[i]),sizeof(mme_headers[i]));
  }


  //
  // Start the Callback
  //
  pthread_attr_init(&pthread_attr);
  pthread_attr_setschedpolicy(&pthread_attr,SCHED_FIFO);
  pthread_create(&mme_pthread,&pthread_attr,__DevMmeWaveCallback,this);

  mme_audio_timer->start(1000*MME_BUFFER_SIZE/codec()->samplerate());

  return true;
#else
  return false;
#endif  // MME
}


void DevMme::stop()
{
#ifdef MME
  waveOutClose(mme_handle);
#endif  // MME
}


void DevMme::loadStats(QStringList *hdrs,QStringList *values,
			       bool is_first)
{
#ifdef MME
  if(is_first) {
    hdrs->push_back("Device|Type");
    values->push_back("MME");

    hdrs->push_back("Device|Name");
    values->push_back(mme_device_names.at(mme_device_id));

    hdrs->push_back("Device|Samplerate");
    values->push_back(QString().sprintf("%u",codec()->samplerate()));

    hdrs->push_back("Device|Buffer Size");
    values->push_back(QString().sprintf("%u",MME_BUFFER_SIZE));
  }
  /*
  hdrs->push_back("Device|Frames Played");
  values->push_back(QString().sprintf("%lu",jack_play_position));

  hdrs->push_back("Device|PLL Offset");
  values->push_back(QString().sprintf("%8.6lf",jack_pll_offset));

  hdrs->push_back("Device|PLL Setpoint Frames");
  values->push_back(QString().sprintf("%u",jack_pll_setpoint_frames));
  */
#endif  // MME
}


void DevMme::audioData()
{
#ifdef MME
  /*
  MMRESULT merr;
  unsigned hdrptr;
  Ringbuffer *rb=codec()->ring();

  hdrptr=mme_current_header%MME_PERIOD_QUAN;
  while((!mme_headers[hdrptr].dwUser)&&(rb->readSpace()>=MME_BUFFER_SIZE)) {
    rb->read(mme_pcm_in,MME_BUFFER_SIZE);
    src_float_to_short_array(mme_pcm_in,(short *)mme_headers[hdrptr].lpData,
			     MME_BUFFER_SIZE*codec()->channels());
    mme_headers[hdrptr].dwUser=true;
    if((merr=waveOutWrite(mme_handle,&(mme_headers[hdrptr]),
			  sizeof(mme_headers[hdrptr])))!=0) {
      Log(LOG_WARNING,MmeError(merr));
    }
    hdrptr=++mme_current_header%MME_PERIOD_QUAN;
    mme_frames_played+=MME_BUFFER_SIZE;
    updatePlayPosition(mme_frames_played);
  }
  */
  updatePlayPosition(mme_frames_played);
#endif // MME
}


#ifdef MME
QString DevMme::MmeError(MMRESULT err) const
{
  char err_msg[200];

  if(waveOutGetErrorText(err,err_msg,200)==0) {
    return QString(err_msg);
  }
  return tr("Unknown MME error")+QString().sprintf(" [%d]",err);
}
#endif  // MME

