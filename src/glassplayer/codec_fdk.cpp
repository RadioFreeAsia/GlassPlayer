// codec_fdk.cpp
//
// AAC codec
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

#include "codec_fdk.h"

CodecFdk::CodecFdk(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeAac,bitrate,parent)
{
  fdk_fdkaac_handle=NULL;
  fdk_frame_count=0;

#ifdef HAVE_FDKAAC
  //
  // Load Library
  //
  if((fdk_fdkaac_handle=dlopen("libfdk-aac.so",RTLD_NOW))!=NULL) {
    *(void **)(&aacDecoder_AncDataInit)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_AncDataInit");
    *(void **)(&aacDecoder_AncDataGet)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_AncDataGet");
    *(void **)(&aacDecoder_SetParam)=
      dlsym(fdk_fdkaac_handle,"");
    *(void **)(&aacDecoder_GetFreeBytes)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_GetFreeBytes");
    *(void **)(&aacDecoder_Open)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_Open");
    *(void **)(&aacDecoder_ConfigRaw)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_ConfigRaw");
    *(void **)(&aacDecoder_Fill)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_Fill");
    *(void **)(&aacDecoder_DecodeFrame)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_DecodeFrame");
    *(void **)(&aacDecoder_Close)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_Close");
    *(void **)(&aacDecoder_GetStreamInfo)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_GetStreamInfo");
    *(void **)(&aacDecoder_GetLibInfo)=
      dlsym(fdk_fdkaac_handle,"aacDecoder_GetLibInfo");

    //
    // Initialize Decoder Instance
    //
    fdk_decoder=aacDecoder_Open(TT_MP4_ADTS,1);
  }
#endif  // HAVE_FDKAAC
}


CodecFdk::~CodecFdk()
{
#ifdef HAVE_FDKAAC
  aacDecoder_Close(fdk_decoder);
#endif  // HAVE_FDKAAC
}


bool CodecFdk::isAvailable() const
{
  return fdk_fdkaac_handle!=NULL;
}


QString CodecFdk::defaultExtension() const
{
  return QString("aac");
}


void CodecFdk::process(const QByteArray &data)
{
#ifdef HAVE_FDKAAC
  AAC_DECODER_ERROR err;
  unsigned remaining=data.length();
  unsigned used=0;
  QByteArray buffer;
  int16_t pcm16[4096];
  float pcm[4096];
  unsigned char *bitstream[1];
  unsigned bitstream_length[1];
  CStreamInfo *cinfo=NULL;

  bitstream[0]=new unsigned char[data.length()];
  memcpy(bitstream[0],data.constData(),data.length());
  bitstream_length[0]=data.length();
  do {
    used=data.length()-remaining;
    if((err=aacDecoder_Fill(fdk_decoder,bitstream+used,bitstream_length-used,
			    &remaining))==AAC_DEC_OK) {
      while((err=aacDecoder_DecodeFrame(fdk_decoder,pcm16,2048,0))==
	    AAC_DEC_OK) {
	fdk_frame_count++;
	cinfo=aacDecoder_GetStreamInfo(fdk_decoder);
	if(!isFramed()) {
	  if(fdk_frame_count>50) {
	    setFramed(cinfo->numChannels,cinfo->sampleRate,bitrate());
	  }
	}
	else {
	  src_short_to_float_array(pcm16,pcm,
				   cinfo->frameSize*cinfo->numChannels);
	  ring()->write(pcm,cinfo->frameSize);
	  signalAudioWritten(cinfo->frameSize);
	}
      }
    }
  } while(remaining!=0);
#endif  // HAVE_FDKAAC
}

#ifdef HAVE_FDKAAC
void CodecFdk::SetDecoderParam(const AACDEC_PARAM param,const int value)
{
  AAC_DECODER_ERROR err;

  if((err=aacDecoder_SetParam(fdk_decoder,param,value))!=AAC_DEC_OK) {
    fprintf(stderr,"AAC decoder error %d\n",err);
    exit(256);
  }
}
#endif  // HAVE_FDKAAC
