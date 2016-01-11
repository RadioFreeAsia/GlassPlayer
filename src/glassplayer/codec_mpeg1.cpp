// codec_mpeg1.cpp
//
// MPEG-1/1.5 codec
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

#include <unistd.h>

#include "codec_mpeg1.h"

CodecMpeg1::CodecMpeg1(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeMpeg1,bitrate,parent)
{
  mpeg1_mad_handle=NULL;
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
    *(void **)(&mad_synth_frame)=dlsym(mpeg1_mad_handle,"mad_synth_frame");
    *(void **)(&mad_frame_finish)=dlsym(mpeg1_mad_handle,"mad_frame_finish");
    *(void **)(&mad_stream_finish)=dlsym(mpeg1_mad_handle,"mad_stream_finish");

    //
    // Initialize Instance
    //
    mad_stream_init(&mpeg1_mad_stream);
    mad_frame_init(&mpeg1_mad_frame);
    mad_synth_init(&mpeg1_mad_synth);
  }
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
  if(isFramed()) {
    ProcessBlock(data);
  }
  else {
    for(int i=0;i<(data.length()-4);i++) {
      if(FindHeader((const uint8_t *)data.constData()+i)) {
	ProcessBlock(data.mid(i,data.size()));
	break;
      }
    }
  }
}


void CodecMpeg1::ProcessBlock(const QByteArray &mpeg)
{
  mpeg1_mpeg.append(mpeg);
  float pcm[32768];
  int frame_offset=0;

  mad_stream_buffer(&mpeg1_mad_stream,(const unsigned char *)mpeg1_mpeg.data(),
		    mpeg1_mpeg.length());
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
  write(1,pcm,frame_offset*channels()*sizeof(float));
  mpeg1_mpeg=
    mpeg1_mpeg.right(mpeg1_mad_stream.bufend-mpeg1_mad_stream.next_frame);
}


bool CodecMpeg1::FindHeader(const uint8_t *data)
{
#ifdef HAVE_LIBMAD
  CodecMpeg1::MpegID mpeg_id=CodecMpeg1::NonMpeg;
  unsigned layer=0;
  unsigned bitrate=0;
  unsigned samplerate=0;
  unsigned channels;

  //
  // Sync
  //
  if((data[0]!=0xFF)||((data[1]&0xE0)!=0xE0)) {
    return false;
  }

  //
  // MPEG Id
  //
  switch(data[1]&0x18) {
  case 0x00:
    mpeg_id=CodecMpeg1::Mpeg25;
    break;

  case 0x10:
    mpeg_id=CodecMpeg1::Mpeg2;
    break;

  case 0x18:
    mpeg_id=CodecMpeg1::Mpeg1;
    break;

  default:
    return false;
  }

  //
  // Layer
  //
  switch((data[1]&0x06)>>1) {
  case 1:
    layer=3;
    break;

  case 2:
    layer=2;
    break;

  case 3:
    layer=1;
    break;

  default:
    return false;
  }

  //
  // Bitrate
  //
  switch(mpeg_id) {
  case CodecMpeg1::Mpeg1:
    switch(layer) {
    case 1:
      switch(data[2]>>4) {
      case 1:
	bitrate=32;
	break;
	
      case 2:
	bitrate=64;
	break;
	
      case 3:
	bitrate=96;
	break;
	
      case 4:
	bitrate=128;
	break;
	
      case 5:
	bitrate=160;
	break;
	
      case 6:
	bitrate=192;
	break;
	
      case 7:
	bitrate=224;
	break;
	
      case 8:
	bitrate=256;
	break;
	
      case 9:
	bitrate=288;
	break;
	
      case 10:
	bitrate=320;
	break;
	
      case 11:
	bitrate=352;
	break;
	
      case 12:
	bitrate=384;
	break;
	
      case 13:
	bitrate=416;
	break;
	
      case 14:
	bitrate=448;
	break;

      default:
	return false;
      }
      break;

    case 2:
      switch(data[2]>>4) {
      case 1:
	bitrate=32;
	break;
	
      case 2:
	bitrate=48;
	break;
	
      case 3:
	bitrate=56;
	break;
	
      case 4:
	bitrate=64;
	break;
	
      case 5:
	bitrate=80;
	break;
	
      case 6:
	bitrate=96;
	break;
	
      case 7:
	bitrate=112;
	break;
	
      case 8:
	bitrate=128;
	break;
	
      case 9:
	bitrate=160;
	break;
	
      case 10:
	bitrate=192;
	break;
	
      case 11:
	bitrate=224;
	break;
	
      case 12:
	bitrate=256;
	break;
	
      case 13:
	bitrate=320;
	break;
	
      case 14:
	bitrate=384;
	break;

      default:
	return false;
	break;
      }
      break;

    case 3:
      switch(data[2]>>4) {
      case 1:
	bitrate=32;
	break;
	
      case 2:
	bitrate=40;
	break;
	
      case 3:
	bitrate=48;
	break;
	
      case 4:
	bitrate=56;
	break;
	
      case 5:
	bitrate=64;
	break;
	
      case 6:
	bitrate=80;
	break;
	
      case 7:
	bitrate=96;
	break;
	
      case 8:
	bitrate=112;
	break;
	
      case 9:
	bitrate=128;
	break;
	
      case 10:
	bitrate=160;
	break;
	
      case 11:
	bitrate=192;
	break;
	
      case 12:
	bitrate=224;
	break;
	
      case 13:
	bitrate=256;
	break;
	
      case 14:
	bitrate=320;
	break;

      default:
	return false;
	break;
      }
      break;
    }
    break;

  case CodecMpeg1::Mpeg2:
    switch(layer) {
    case 1:
      switch(data[2]>>4) {
      case 1:
	bitrate=32;
	break;
	
      case 2:
	bitrate=48;
	break;
	
      case 3:
	bitrate=56;
	break;

      case 4:
	bitrate=64;
	break;
	
      case 5:
	bitrate=80;
	break;
	
      case 6:
	bitrate=96;
	break;
	
      case 7:
	bitrate=112;
	break;
	
      case 8:
	bitrate=128;
	break;
	
      case 9:
	bitrate=144;
	break;
	
      case 10:
	bitrate=160;
	break;
	
      case 11:
	bitrate=176;
	break;
	
      case 12:
	bitrate=192;
	break;
	
      case 13:
	bitrate=224;
	break;
	
      case 14:
	bitrate=256;
	break;

      default:
	return false;
      }
      break;

    case 2:
      switch(data[2]>>4) {
      case 1:
	bitrate=8;
	break;
	
      case 2:
	bitrate=16;
	break;
	
      case 3:
	bitrate=24;
	break;
	
      case 4:
	bitrate=32;
	break;
	
      case 5:
	bitrate=40;
	break;
	
      case 6:
	bitrate=48;
	break;
	
      case 7:
	bitrate=56;
	break;
	
      case 8:
	bitrate=64;
	break;
	
      case 9:
	bitrate=80;
	break;
	
      case 10:
	bitrate=96;
	break;
	
      case 11:
	bitrate=112;
	break;

      case 12:
	bitrate=128;
	break;
	
      case 13:
	bitrate=144;
	break;
	
      case 14:
	bitrate=160;
	break;

      default:
	return false;
	break;
      }
      break;

    case 3:
      switch(data[2]>>4) {
      case 1:
	bitrate=8;
	break;
	
      case 2:
	bitrate=16;
	break;
	
      case 3:
	bitrate=24;
	break;
	
      case 4:
	bitrate=32;
	break;
	
      case 5:
	bitrate=40;
	break;
	
      case 6:
	bitrate=48;
	break;
	
      case 7:
	bitrate=56;
	break;
	
      case 8:
	bitrate=64;
	break;
	
      case 9:
	bitrate=80;
	break;
	
      case 10:
	bitrate=96;
	break;
	
      case 11:
	bitrate=112;
	break;
	
      case 12:
	bitrate=128;
	break;
	
      case 13:
	bitrate=144;
	break;

      case 14:
	bitrate=160;
	break;

      default:
	return false;
	break;
      }
      break;
    }
    break;

  default:
    return false;
  }

  //
  // Sample Rate
  //
  switch((data[2]>>2)&0x03) {
  case 0:
    switch(mpeg_id) {
    case CodecMpeg1::Mpeg1:
      samplerate=44100;
      break;

    case CodecMpeg1::Mpeg2:
      samplerate=22050;
      break;

    case CodecMpeg1::Mpeg25:
      samplerate=11025;
      break;

    default:
      return false;
    }
    break;

  case 1:
    switch(mpeg_id) {
    case CodecMpeg1::Mpeg1:
      samplerate=48000;
      break;

    case CodecMpeg1::Mpeg2:
      samplerate=24000;
      break;

    case CodecMpeg1::Mpeg25:
      samplerate=12000;
      break;

    default:
      return false;
    }
    break;

  case 2:
    switch(mpeg_id) {
    case CodecMpeg1::Mpeg1:
      samplerate=32000;
      break;

    case CodecMpeg1::Mpeg2:
      samplerate=16000;
      break;

    case CodecMpeg1::Mpeg25:
      samplerate=8000;
      break;

    default:
      return false;
    }
    break;

  default:
    return false;
    break;
  }

  //
  // Mode
  //
  switch(data[3]>>6) {
  case 0:
    //mode=ACM_MPEG_STEREO;
    channels=2;
    break;

  case 1:
    //mode=ACM_MPEG_JOINTSTEREO;
    channels=2;
    break;

  case 2:
    //mode=ACM_MPEG_DUALCHANNEL;
    channels=2;
    break;

  case 3:
    //mode=ACM_MPEG_SINGLECHANNEL;
    channels=1;
    break;
  }

  setFramed(channels,samplerate,bitrate);
  return true;
#else
  return false;
#endif  // HAVE_LIBMAD
}
