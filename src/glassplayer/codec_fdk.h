// codec_fdk.h
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

#ifndef CODEC_FDK_H
#define CODEC_FDK_H

#include <stdint.h>

#ifdef HAVE_FDKAAC
#include <fdk-aac/aacdecoder_lib.h>
#endif  // HAVE_FDKAAC

#include "codec.h"

class CodecFdk : public Codec
{
  Q_OBJECT;
 public:
  CodecFdk(unsigned bitrate,QObject *parent=0);
  ~CodecFdk();
  bool isAvailable() const;
  QString defaultExtension() const;
  void process(const QByteArray &data,bool is_last);

 protected:
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
  QString GetAotText(int aot);
  lt_dlhandle fdk_fdkaac_handle;
  uint64_t fdk_frame_count;
#ifdef HAVE_FDKAAC
  void SetDecoderParam(const AACDEC_PARAM param,const int value);
  AAC_DECODER_ERROR (*aacDecoder_AncDataInit)(HANDLE_AACDECODER,UCHAR *,int);
  AAC_DECODER_ERROR (*aacDecoder_AncDataGet)(HANDLE_AACDECODER,int,UCHAR **,
    int);
  AAC_DECODER_ERROR (*aacDecoder_SetParam)(const HANDLE_AACDECODER,
    const AACDEC_PARAM,const INT);
  AAC_DECODER_ERROR (*aacDecoder_GetFreeBytes)(const HANDLE_AACDECODER,UINT *);
  HANDLE_AACDECODER (*aacDecoder_Open)(TRANSPORT_TYPE,UINT);
  AAC_DECODER_ERROR (*aacDecoder_ConfigRaw)(HANDLE_AACDECODER,UCHAR *[],
    const UINT []);
  AAC_DECODER_ERROR (*aacDecoder_Fill)(HANDLE_AACDECODER,UCHAR *[],
    const UINT [],UINT *);
  AAC_DECODER_ERROR (*aacDecoder_DecodeFrame)(HANDLE_AACDECODER,INT_PCM *,
    const INT,const UINT);
  void (*aacDecoder_Close)(HANDLE_AACDECODER);
  CStreamInfo* (*aacDecoder_GetStreamInfo)(HANDLE_AACDECODER);
  INT (*aacDecoder_GetLibInfo)(LIB_INFO *);
  HANDLE_AACDECODER fdk_decoder;
  CStreamInfo *fdk_cinfo;
#endif  // HAVE_FDKAAC
};


#endif  // CODEC_FDK_H
