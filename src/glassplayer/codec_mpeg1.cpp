// codec_mpeg1.cpp
//
// MPEG-1 codec
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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "codec_mpeg1.h"
#include "logging.h"

CodecMpeg1::CodecMpeg1(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeMpeg1,bitrate,parent)
{
  mpeg1_mad_handle=NULL;
#ifdef HAVE_LIBMAD
  LoadLibmad();
#endif  // HAVE_LIBMAD
}


CodecMpeg1::~CodecMpeg1()
{
}


bool CodecMpeg1::isAvailable() const
{
  return mpeg1_mad_handle!=NULL;
}


QString CodecMpeg1::defaultExtension() const
{
  return QString("mpg");
}


void CodecMpeg1::process(const QByteArray &data)
{
  float pcm[32768*4];
  int frame_offset=0;
  int lost_frames=0;
  int err_count=0;

  mpeg1_mpeg.append(data);
  mad_stream_buffer(&mpeg1_mad_stream,(const unsigned char *)mpeg1_mpeg.data(),
		    mpeg1_mpeg.length());
  do {
    while(mad_frame_decode(&mpeg1_mad_frame,&mpeg1_mad_stream)==0) {  
      mad_synth_frame(&mpeg1_mad_synth,&mpeg1_mad_frame);
      for(int i=0;i<mpeg1_mad_synth.pcm.length;i++) {
	for(int j=0;j<mpeg1_mad_synth.pcm.channels;j++) {
	  pcm[frame_offset+i*mpeg1_mad_synth.pcm.channels+j]=
	    (float)mad_f_todouble(mpeg1_mad_synth.pcm.samples[j][i]);
	}
      }
      frame_offset+=(mpeg1_mad_synth.pcm.length*mpeg1_mad_synth.pcm.channels);
    }
    if(MAD_RECOVERABLE(mpeg1_mad_stream.error)!=0) {
      if(err_count++>10) {
	Reset();
	break;
      }
    }
  } while(mpeg1_mad_stream.error!=MAD_ERROR_BUFLEN);

  if(mpeg1_mad_stream.error==MAD_ERROR_BUFLEN) {
    if(ring()!=NULL) {
      if((lost_frames=frame_offset-ring()->writeSpace())>0) {
	Log(LOG_WARNING,QString().sprintf("XRUN: possible loss of %d frames",
					  lost_frames));
      }
    }
    if(frame_offset>0) {
      if(!isFramed()) {
	setFramed(2,mpeg1_mad_frame.header.samplerate,
		  mpeg1_mad_frame.header.bitrate/1000);
      }
      else {
	ring()->write(pcm,frame_offset/channels());
	emit audioWritten(frame_offset/channels());
      }
    }
    mpeg1_mpeg=
      mpeg1_mpeg.right(mpeg1_mad_stream.bufend-mpeg1_mad_stream.next_frame);
  }
}


void CodecMpeg1::Reset()
{
  FreeLibmad();
  LoadLibmad();
  Log(LOG_WARNING,Codec::typeText(type())+" codec reset");
}


bool CodecMpeg1::LoadLibmad()
{
#ifdef HAVE_LIBMAD
  if((mpeg1_mad_handle=dlopen("libmad.so",RTLD_NOW))!=NULL) {
    //
    // Initialize Library
    //
    *(void **)(&mad_stream_init)=dlsym(mpeg1_mad_handle,"mad_stream_init");
    *(void **)(&mad_frame_init)=dlsym(mpeg1_mad_handle,"mad_frame_init");
    *(void **)(&mad_synth_init)=dlsym(mpeg1_mad_handle,"mad_synth_init");
    *(void **)(&mad_stream_buffer)=dlsym(mpeg1_mad_handle,"mad_stream_buffer");
    *(void **)(&mad_frame_decode)=dlsym(mpeg1_mad_handle,"mad_frame_decode");
    *(void **)(&mad_header_decode)=dlsym(mpeg1_mad_handle,"mad_header_decode");
    *(void **)(&mad_synth_frame)=dlsym(mpeg1_mad_handle,"mad_synth_frame");
    *(void **)(&mad_frame_mute)=dlsym(mpeg1_mad_handle,"mad_frame_mute");
    *(void **)(&mad_synth_mute)=dlsym(mpeg1_mad_handle,"mad_synth_mute");
    *(void **)(&mad_stream_sync)=dlsym(mpeg1_mad_handle,"mad_stream_sync");
    *(void **)(&mad_frame_finish)=dlsym(mpeg1_mad_handle,"mad_frame_finish");
    *(void **)(&mad_stream_finish)=dlsym(mpeg1_mad_handle,"mad_stream_finish");
    *(void **)(&mad_header_init)=dlsym(mpeg1_mad_handle,"mad_header_init");

    //
    // Initialize Instance
    //
    mad_stream_init(&mpeg1_mad_stream);
    mad_synth_init(&mpeg1_mad_synth);
    mad_frame_init(&mpeg1_mad_frame);
    return true;
  }
#endif  // HAVE_LIBMAD
  return false;
}


void CodecMpeg1::FreeLibmad()
{
  mad_frame_finish(&mpeg1_mad_frame);
  mad_synth_finish(&mpeg1_mad_synth);
  mad_stream_finish(&mpeg1_mad_stream);
  dlclose(mpeg1_mad_handle);
}
