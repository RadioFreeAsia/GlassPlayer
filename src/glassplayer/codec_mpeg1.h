// codec_mpeg1.h
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

#ifndef CODEC_MPEG1_H
#define CODEC_MPEG1_H

#ifdef HAVE_LIBMAD
#include <mad.h>
#endif  // HAVE_LIBMAD

#include "codec.h"

class CodecMpeg1 : public Codec
{
  Q_OBJECT;
 public:
  CodecMpeg1(unsigned bitrate,QObject *parent=0);
  ~CodecMpeg1();
  bool isAvailable() const;
  QString defaultExtension() const;
  void process(const QByteArray &data);

 private:
  enum MpegID {NonMpeg=0,Mpeg1=1,Mpeg2=2,Mpeg25=3};
  void ProcessBlock(const QByteArray &mpeg);
  bool FindHeader(const uint8_t *data);
  void *mpeg1_mad_handle;
#ifdef HAVE_LIBMAD
  void (*mad_stream_init)(struct mad_stream *);
  void (*mad_frame_init)(struct mad_frame *);
  void (*mad_synth_init)(struct mad_synth *);
  void (*mad_stream_buffer)(struct mad_stream *,unsigned char const *,
			    unsigned long);
  int (*mad_frame_decode)(struct mad_frame *, struct mad_stream *);
  void (*mad_synth_frame)(struct mad_synth *, struct mad_frame const *);
  void (*mad_frame_finish)(struct mad_frame *);
  void (*mad_stream_finish)(struct mad_stream *);
  struct mad_stream mpeg1_mad_stream;
  struct mad_frame mpeg1_mad_frame;
  struct mad_synth mpeg1_mad_synth;
  //  int mpeg1_frame_size;
  QByteArray mpeg1_mpeg;
#endif  // HAVE_LIBMAD
};


#endif  // CODEC_MPEG1_H
