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

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <QStringList>

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
  FreeLibmad();
}


bool CodecMpeg1::isAvailable() const
{
  return mpeg1_mad_handle!=NULL;
}


QString CodecMpeg1::defaultExtension() const
{
  return QString("mpg");
}


void CodecMpeg1::process(const QByteArray &data,bool is_last)
{
#ifdef HAVE_LIBMAD
  float pcm[32768*4];
  int frame_offset=0;
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
    if(frame_offset>0) {
      mpeg1_mad_header=mpeg1_mad_frame.header;
      if(!isFramed()) {
	int channels=2;
	if(mpeg1_mad_frame.header.mode==MAD_MODE_SINGLE_CHANNEL) {
	  channels=1;
	}
	setFramed(channels,mpeg1_mad_frame.header.samplerate,
		  mpeg1_mad_frame.header.bitrate/1000);
      }
      else {
	writePcm(pcm,frame_offset/channels(),is_last);
      }
    }
    mpeg1_mpeg=
      mpeg1_mpeg.right(mpeg1_mad_stream.bufend-mpeg1_mad_stream.next_frame);
  }
  if(is_last) {
    frame_offset=0;
    for(int i=0;i<MAD_BUFFER_GUARD;i++) {
      mpeg1_mpeg.append((char)0);
    }
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
    writePcm(pcm,frame_offset/channels(),is_last);
  }
#endif  // HAVE_LIBMAD
}


void CodecMpeg1::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
#ifdef HAVE_LIBMAD
  if(is_first) {
    hdrs->push_back("Codec|Algorithm");
    values->push_back("MPEG-1");

    hdrs->push_back("Codec|Layer");
    values->push_back(QString().sprintf("%u",mpeg1_mad_header.layer));

    switch(mpeg1_mad_header.mode) {
    case MAD_MODE_STEREO:
      hdrs->push_back("Codec|Mode");
      values->push_back("Stereo");
      break;

    case MAD_MODE_JOINT_STEREO:
      hdrs->push_back("Codec|Mode");
      values->push_back("JointStereo");
      break;

    case MAD_MODE_DUAL_CHANNEL:
      hdrs->push_back("Codec|Mode");
      values->push_back("DualChannel");
      break;

    case MAD_MODE_SINGLE_CHANNEL:
      hdrs->push_back("Codec|Mode");
      values->push_back("SingleChannel");
      break;
    }

    hdrs->push_back("Codec|Channels");
    values->push_back(QString().sprintf("%u",channels()));

    hdrs->push_back("Codec|Bitrate");
    values->push_back(QString().sprintf("%lu",mpeg1_mad_header.bitrate));

    switch(mpeg1_mad_header.emphasis) {
    case MAD_EMPHASIS_NONE:
      hdrs->push_back("Codec|Emphasis");
      values->push_back("None");
      break;

    case MAD_EMPHASIS_50_15_US:
      hdrs->push_back("Codec|Emphasis");
      values->push_back("50/15 uS");
      break;

    case MAD_EMPHASIS_CCITT_J_17:
      hdrs->push_back("Codec|Emphasis");
      values->push_back("CCITT J.17");
      break;

    case MAD_EMPHASIS_RESERVED:
    default:
      hdrs->push_back("Codec|Emphasis");
      values->push_back("Unknown");
      break;
    }
  }
#endif  // HAVE_LIBMAD
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
  lt_dlinit();
  if((mpeg1_mad_handle=lt_dlopen("libmad.so.0"))!=NULL) {
    //
    // Initialize Library
    //
    *(void **)(&mad_stream_init)=lt_dlsym(mpeg1_mad_handle,"mad_stream_init");
    *(void **)(&mad_frame_init)=lt_dlsym(mpeg1_mad_handle,"mad_frame_init");
    *(void **)(&mad_synth_init)=lt_dlsym(mpeg1_mad_handle,"mad_synth_init");
    *(void **)(&mad_stream_buffer)=
      lt_dlsym(mpeg1_mad_handle,"mad_stream_buffer");
    *(void **)(&mad_frame_decode)=lt_dlsym(mpeg1_mad_handle,"mad_frame_decode");
    *(void **)(&mad_header_decode)=
      lt_dlsym(mpeg1_mad_handle,"mad_header_decode");
    *(void **)(&mad_synth_frame)=lt_dlsym(mpeg1_mad_handle,"mad_synth_frame");
    *(void **)(&mad_frame_mute)=lt_dlsym(mpeg1_mad_handle,"mad_frame_mute");
    *(void **)(&mad_synth_mute)=lt_dlsym(mpeg1_mad_handle,"mad_synth_mute");
    *(void **)(&mad_stream_sync)=lt_dlsym(mpeg1_mad_handle,"mad_stream_sync");
    *(void **)(&mad_frame_finish)=lt_dlsym(mpeg1_mad_handle,"mad_frame_finish");
    *(void **)(&mad_stream_finish)=
      lt_dlsym(mpeg1_mad_handle,"mad_stream_finish");
    *(void **)(&mad_header_init)=lt_dlsym(mpeg1_mad_handle,"mad_header_init");

    //
    // Initialize Instance
    //
    mad_stream_init(&mpeg1_mad_stream);
    mad_synth_init(&mpeg1_mad_synth);
    mad_frame_init(&mpeg1_mad_frame);
    memset(&mpeg1_mad_header,0,sizeof(mpeg1_mad_header));

    return true;
  }
#endif  // HAVE_LIBMAD
  return false;
}


void CodecMpeg1::FreeLibmad()
{
#ifdef HAVE_LIBMAD
  mad_frame_finish(&mpeg1_mad_frame);
  mad_synth_finish(&mpeg1_mad_synth);
  mad_stream_finish(&mpeg1_mad_stream);
  lt_dlclose(mpeg1_mad_handle);
#endif  // HAVE_LIBMAD
}
