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
#ifdef HAVE_OGG
  bool TriState(int result,const QString &err_msg);
  bool ParseOpusHeader(unsigned *samprate,unsigned *chans,ogg_packet *op);
  QString CommentString(const unsigned char *str) const;
  int ogg_istate;
  ogg_sync_state ogg_oy;
  ogg_stream_state ogg_os;
  ogg_page ogg_og;
  ogg_packet ogg_op;

  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vd;
  vorbis_block vb;

  OpusDecoder *ogg_opus_decoder;
  QString ogg_vendor_string;
#endif  // HAVE_OGG
};


#endif  // CODEC_OGG_H
