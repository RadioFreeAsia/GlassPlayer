// codec_ogg.h
//
// OggVorbis and OggOpus Codecs
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

#ifndef CODEC_OGG_H
#define CODEC_OGG_H

#ifdef HAVE_OGG
#include <ogg/ogg.h>
#include <opus/opus.h>
#include <vorbis/vorbisenc.h>
#endif  // HAVE_OGG

#include "codec.h"

class CodecOgg : public Codec
{
  Q_OBJECT;
 public:
  CodecOgg(unsigned bitrate,QObject *parent=0);
  ~CodecOgg();
  bool isAvailable() const;
  QString defaultExtension() const;
  void process(const QByteArray &data,bool is_last);

 protected:
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
  enum OggCodecType {Unknown=0,Vorbis=1,Opus=2};
  OggCodecType ogg_codec_type;
  bool TriState(int result,const QString &err_msg);
  QString CommentString(const unsigned char *str) const;
  bool LoadOgg();
#ifdef HAVE_OGG
  bool ParseOpusHeader(unsigned *samprate,unsigned *chans,ogg_packet *op);
  lt_dlhandle ogg_ogg_handle;
  int (*ogg_sync_init)(ogg_sync_state *);
  int (*ogg_sync_clear)(ogg_sync_state *);
  int (*ogg_sync_reset)(ogg_sync_state *);
  char *(*ogg_sync_buffer)(ogg_sync_state *,long);
  int (*ogg_sync_pageout)(ogg_sync_state *,ogg_page *);
  int (*ogg_sync_pagein)(ogg_stream_state *,ogg_page *);
  int (*ogg_stream_init)(ogg_stream_state *,int);
  int (*ogg_stream_pagein)(ogg_stream_state *,ogg_page *);
  int (*ogg_stream_packetout)(ogg_stream_state *,ogg_packet *);
  int (*ogg_sync_wrote)(ogg_sync_state *,long);
  int (*ogg_page_serialno)(const ogg_page *og);
  int ogg_istate;
  ogg_sync_state ogg_oy;
  ogg_stream_state ogg_os;
  ogg_page ogg_og;
  ogg_packet ogg_op;
  QString ogg_vendor_string;

  lt_dlhandle ogg_vorbis_handle;
  void (*vorbis_info_init)(vorbis_info *);
  void (*vorbis_info_clear)(vorbis_info *);
  void (*vorbis_comment_init)(vorbis_comment *);
  void (*vorbis_comment_clear)(vorbis_comment *);
  int (*vorbis_block_init)(vorbis_dsp_state *,vorbis_block *);
  int (*vorbis_synthesis)(vorbis_block *,ogg_packet *);
  int (*vorbis_synthesis_headerin)(vorbis_info *,vorbis_comment *,
				    ogg_packet *);
  int (*vorbis_synthesis_init)(vorbis_dsp_state *,vorbis_info *);
  int (*vorbis_synthesis_blockin)(vorbis_dsp_state *,vorbis_block *);
  int (*vorbis_synthesis_pcmout)(vorbis_dsp_state *,float ***pcm);
  int (*vorbis_synthesis_read)(vorbis_dsp_state *,int);
  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vd;
  vorbis_block vb;

  lt_dlhandle ogg_opus_handle;
  OpusDecoder *(*opus_decoder_create)(opus_int32,int,int *);
  int (*opus_decode_float)(OpusDecoder *,const unsigned char *,opus_int32,
			   float *,int,int);
  OpusDecoder *ogg_opus_decoder;
#endif  // HAVE_OGG
};


#endif  // CODEC_OGG_H
