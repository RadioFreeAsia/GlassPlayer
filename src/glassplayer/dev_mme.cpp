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

#include <samplerate.h>

#ifdef MME
#include <mmdeviceapi.h>
#endif  // MME

#include "dev_mme.h"
#include "logging.h"

#ifdef MME
void CALLBACK __DevMmeCallback(HWAVEOUT hwo,UINT umsg,DWORD_PTR instance,  
			       DWORD_PTR param1,DWORD_PTR param2)
{
  WAVEHDR *hdr=(WAVEHDR *)param1;

  if(umsg==WOM_DONE) {
    hdr->dwUser=false;
  }
}
#endif  // MME


DevMme::DevMme(Codec *codec,QObject *parent)
  : AudioDevice(codec,parent)
{
#ifdef MME
  mme_current_header=0;

  for(int i=0;i<MME_PERIOD_QUAN;i++) {
    memset(&(mme_headers[i]),0,sizeof(mme_headers[i]));
  }

  mme_audio_timer=new QTimer(this);
  connect(mme_audio_timer,SIGNAL(timeout()),this,SLOT(audioData()));
#endif  // MME
}


DevMme::~DevMme()
{
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
  return true;
}


bool DevMme::start(QString *err)
{
#ifdef MME
  MMRESULT merr;
  UINT device=0;
  WAVEFORMATEX wfx;

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

  if((merr=waveOutOpen(&mme_handle,device,&wfx,(DWORD_PTR)__DevMmeCallback,
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

  mme_audio_timer->start(1000*MME_BUFFER_SIZE/codec()->samplerate());

  return true;
#else
  return false;
#endif  // MME
}


void DevMme::stop()
{
}


void DevMme::loadStats(QStringList *hdrs,QStringList *values,
			       bool is_first)
{
}


void DevMme::audioData()
{
#ifdef MME
  MMRESULT merr;
  float pcm[MME_BUFFER_SIZE*codec()->channels()];
  Ringbuffer *rb=codec()->ring();

  unsigned hdrptr=mme_current_header%MME_PERIOD_QUAN;
  while((!mme_headers[hdrptr].dwUser)&&(rb->readSpace()>=MME_BUFFER_SIZE)) {
    rb->read(pcm,MME_BUFFER_SIZE);
    src_float_to_short_array(pcm,(short *)mme_headers[hdrptr].lpData,
			     MME_BUFFER_SIZE*codec()->channels());
    mme_headers[hdrptr].dwUser=true;
    if((merr=waveOutWrite(mme_handle,&(mme_headers[hdrptr]),
			  sizeof(mme_headers[hdrptr])))!=0) {
      Log(LOG_WARNING,MmeError(merr));
    }
    hdrptr=++mme_current_header%MME_PERIOD_QUAN;
  }

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

