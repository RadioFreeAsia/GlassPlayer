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
  //  CStreamInfo *cinfo=NULL;

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
	  ring()->write(pcm,fdk_cinfo->frameSize);
	  signalAudioWritten(fdk_cinfo->frameSize);
	}
      }
    }
  } while(remaining!=0);
#endif  // HAVE_FDKAAC
}


void CodecFdk::loadStats(QStringList *hdrs,QStringList *values)
{
#ifdef HAVE_FDKAAC
  switch(fdk_cinfo->aot) {
  case AOT_AAC_MAIN:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC");
    break;

  case AOT_AAC_SSR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-SSR");
    break;

  case AOT_AAC_LC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-LC");
    break;

  case AOT_AAC_LTP:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-LTP");
    break;

  case AOT_SBR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-SBR");
    break;

  case AOT_AAC_SCAL:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-SCAL");
    break;

  case AOT_TWIN_VQ:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("TwinVQ");
    break;

  case AOT_CELP:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("CELP");
    break;

  case AOT_HVXC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("HVXC");
    break;

  case AOT_TTSI:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("TTSI");
    break;

  case AOT_MAIN_SYNTH:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MainSynth");
    break;

  case AOT_WAV_TAB_SYNTH:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("WaveTableSynth");
    break;

  case AOT_GEN_MIDI:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("GeneralMIDI");
    break;

  case AOT_ALG_SYNTH_AUD_FX:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AltSynth/FX");
    break;

  case AOT_ER_AAC_LC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-ER/LC");
    break;

  case AOT_ER_AAC_LTP:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-ER/LTP");
    break;

  case AOT_ER_AAC_SCAL:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-ER/SCALE");
    break;

  case AOT_ER_TWIN_VQ:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("TwinVQ-ER");
    break;

  case AOT_ER_BSAC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("BSAC-ER");
    break;

  case AOT_ER_AAC_LD:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-LD/ER");
    break;

  case AOT_ER_CELP:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("CELP-ER");
    break;

  case AOT_ER_HVXC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("HVXC-ER");
    break;

  case AOT_ER_HILN:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("HILN-ER");
    break;

  case AOT_ER_PARA:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("PARA-ER");
    break;

  case AOT_PS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-PS");
    break;

  case AOT_MPEGS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG Surround");
    break;

  case AOT_MP3ONMP4_L1:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG Layer 1 in MP4");
    break;

  case AOT_MP3ONMP4_L2:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG Layer 2 in MP4");
    break;

  case AOT_MP3ONMP4_L3:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG Layer 3 in MP4");
    break;

  case AOT_AAC_SLS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-SLS");
    break;

  case AOT_SLS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("SLS");
    break;

  case AOT_ER_AAC_ELD:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-ELD");
    break;

  case AOT_USAC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("USAC");
    break;

  case AOT_SAOC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("SAOC");
    break;

  case AOT_LD_MPEGS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("LowDelay MPEG Surround");
    break;

  case AOT_MP2_AAC_MAIN:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-MP2-Main");
    break;

  case AOT_MP2_AAC_LC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-MP2-LC");
    break;

  case AOT_MP2_AAC_SSR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("AAC-MP2-SSR");
    break;

  case AOT_MP2_SBR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MP2-SBR");
    break;

  case AOT_DAB:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DAB");
    break;

  case AOT_DABPLUS_AAC_LC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DAB-AAC-LC");
    break;

  case AOT_DABPLUS_SBR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DAB-SBR");
    break;

  case AOT_DABPLUS_PS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DAB-PS");
    break;

  case AOT_PLAIN_MP1:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG-1 Layer 1");
    break;

  case AOT_PLAIN_MP2:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG-1 Layer 2");
    break;

  case AOT_PLAIN_MP3:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG-1 Layer 3");
    break;

  case AOT_DRM_AAC:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DRM-AAC");
    break;

  case AOT_DRM_SBR:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DRM-SBR");
    break;

  case AOT_DRM_MPEG_PS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DRM-MPEG-PS");
    break;

  case AOT_DRM_SURROUND:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("DRM-Surround");
    break;

  case AOT_MP2_PS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MP2-PS");
    break;

  case AOT_MPEGS_RESIDUALS:
    hdrs->push_back("CodecAlgorithm");
    values->push_back("MPEG Surround Residuals");
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

  hdrs->push_back("CodecChannels");
  values->push_back(QString().sprintf("%u",fdk_cinfo->numChannels));

  //  hdrs->push_back("CodecBitrate");
  //  values->push_back(QString().sprintf("%d",fdk_cinfo->bitRate));
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
