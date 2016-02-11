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

#include <QStringList>

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


void CodecFdk::process(const QByteArray &data,bool is_last)
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
	fdk_cinfo=aacDecoder_GetStreamInfo(fdk_decoder);
	if(!isFramed()) {
	  if(fdk_frame_count>50) {
	    setFramed(fdk_cinfo->numChannels,fdk_cinfo->sampleRate,bitrate());
	  }
	}
	else {
	  src_short_to_float_array(pcm16,pcm,
				   fdk_cinfo->frameSize*fdk_cinfo->numChannels);
	  writePcm(pcm,fdk_cinfo->frameSize,is_last);
	}
      }
    }
  } while(remaining!=0);
  if(is_last) {
    ring()->setFinished();
  }
#endif  // HAVE_FDKAAC
}


void CodecFdk::loadStats(QStringList *hdrs,QStringList *values)
{
#ifdef HAVE_FDKAAC
  hdrs->push_back("Codec|Algorithm");
  values->push_back(GetAotText(fdk_cinfo->aot));

  hdrs->push_back("Codec|Channels");
  values->push_back(QString().sprintf("%u",fdk_cinfo->numChannels));
#endif  // HAVE_FDKAAC
}


QString CodecFdk::GetAotText(int aot)
{
  QString ret=tr("Unknown");

#ifdef HAVE_FDKAAC
  switch(fdk_cinfo->aot) {
  case AOT_AAC_MAIN:
    ret="AAC";
    break;

  case AOT_AAC_SSR:
    ret="AAC-SSR";
    break;

  case AOT_AAC_LC:
    ret="AAC-LC";
    break;

  case AOT_AAC_LTP:
    ret="AAC-LTP";
    break;

  case AOT_SBR:
    ret="AAC-SBR";
    break;

  case AOT_AAC_SCAL:
    ret="AAC-SCAL";
    break;

  case AOT_TWIN_VQ:
    ret="TwinVQ";
    break;

  case AOT_CELP:
    ret="CELP";
    break;

  case AOT_HVXC:
    ret="HVXC";
    break;

  case AOT_TTSI:
    ret="TTSI";
    break;

  case AOT_MAIN_SYNTH:
    ret="MainSynth";
    break;

  case AOT_WAV_TAB_SYNTH:
    ret="WaveTableSynth";
    break;

  case AOT_GEN_MIDI:
    ret="GeneralMIDI";
    break;

  case AOT_ALG_SYNTH_AUD_FX:
    ret="AltSynth/FX";
    break;

  case AOT_ER_AAC_LC:
    ret="AAC-ER/LC";
    break;

  case AOT_ER_AAC_LTP:
    ret="AAC-ER/LTP";
    break;

  case AOT_ER_AAC_SCAL:
    ret="AAC-ER/SCALE";
    break;

  case AOT_ER_TWIN_VQ:
    ret="TwinVQ-ER";
    break;

  case AOT_ER_BSAC:
    ret="BSAC-ER";
    break;

  case AOT_ER_AAC_LD:
    ret="AAC-LD/ER";
    break;

  case AOT_ER_CELP:
    ret="CELP-ER";
    break;

  case AOT_ER_HVXC:
    ret="HVXC-ER";
    break;

  case AOT_ER_HILN:
    ret="HILN-ER";
    break;

  case AOT_ER_PARA:
    ret="PARA-ER";
    break;

  case AOT_PS:
    ret="AAC-PS";
    break;

  case AOT_MPEGS:
    ret="MPEG Surround";
    break;

  case AOT_MP3ONMP4_L1:
    ret="MPEG Layer 1 in MP4";
    break;

  case AOT_MP3ONMP4_L2:
    ret="MPEG Layer 2 in MP4";
    break;

  case AOT_MP3ONMP4_L3:
    ret="MPEG Layer 3 in MP4";
    break;

  case AOT_AAC_SLS:
    ret="AAC-SLS";
    break;

  case AOT_SLS:
    ret="SLS";
    break;

  case AOT_ER_AAC_ELD:
    ret="AAC-ELD";
    break;

  case AOT_USAC:
    ret="USAC";
    break;

  case AOT_SAOC:
    ret="SAOC";
    break;

  case AOT_LD_MPEGS:
    ret="LowDelay MPEG Surround";
    break;

  case AOT_MP2_AAC_MAIN:
    ret="AAC-MP2-Main";
    break;

  case AOT_MP2_AAC_LC:
    ret="AAC-MP2-LC";
    break;

  case AOT_MP2_AAC_SSR:
    ret="AAC-MP2-SSR";
    break;

  case AOT_MP2_SBR:
    ret="MP2-SBR";
    break;

  case AOT_DAB:
    ret="DAB";
    break;

  case AOT_DABPLUS_AAC_LC:
    ret="DAB-AAC-LC";
    break;

  case AOT_DABPLUS_SBR:
    ret="DAB-SBR";
    break;

  case AOT_DABPLUS_PS:
    ret="DAB-PS";
    break;

  case AOT_PLAIN_MP1:
    ret="MPEG-1 Layer 1";
    break;

  case AOT_PLAIN_MP2:
    ret="MPEG-1 Layer 2";
    break;

  case AOT_PLAIN_MP3:
    ret="MPEG-1 Layer 3";
    break;

  case AOT_DRM_AAC:
    ret="DRM-AAC";
    break;

  case AOT_DRM_SBR:
    ret="DRM-SBR";
    break;

  case AOT_DRM_MPEG_PS:
    ret="DRM-MPEG-PS";
    break;

  case AOT_DRM_SURROUND:
    ret="DRM-Surround";
    break;

  case AOT_MP2_PS:
    ret="MP2-PS";
    break;

  case AOT_MPEGS_RESIDUALS:
    ret="MPEG Surround Residuals";
    break;

  case AOT_RSVD_10:
  case AOT_RSVD_11:
  case AOT_RSVD_18:
  case AOT_RSVD_28:
  case AOT_RSVD_35:
  case AOT_RSVD_36:
  case AOT_RSVD50:
  case AOT_ESCAPE:
  case AOT_NONE:
  case AOT_NULL_OBJECT:
    break;
  }
#endif  // HAVE_FDKAAC

  return ret;
}


#ifdef HAVE_FDKAAC
void CodecFdk::SetDecoderParam(const AACDEC_PARAM param,const int value)
{
  AAC_DECODER_ERROR err;

  if((err=aacDecoder_SetParam(fdk_decoder,param,value))!=AAC_DEC_OK) {
    fprintf(stderr,"AAC decoder error %d\n",err);
    exit(GLASS_EXIT_DECODER_ERROR);
  }
}
#endif  // HAVE_FDKAAC
