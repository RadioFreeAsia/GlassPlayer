// codec.h
//
// Abstract base class for audio codecs.
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

#ifndef CODEC_H
#define CODEC_H

#include <queue>
#include <vector>

#include <dlfcn.h>
#include <syslog.h>

#include <samplerate.h>

#include <QObject>

//#include "connector.h"
#include "glasslimits.h"
#include "ringbuffer.h"

#define MAX_AUDIO_BUFFER 4096
#define CODEC_RINGBUFFER_SIZE 33554432

class Codec : public QObject
{
  Q_OBJECT;
 public:
  enum Type {TypeNull=0,TypeMpeg1=1,TypeVorbis=2,TypeAac=3,TypeLast=4};
  Codec(Codec::Type type,unsigned bitrate,QObject *parent=0);
  ~Codec();
  Type type() const;
  unsigned bitrate() const;
  unsigned channels() const;
  void setChannels(unsigned chans);
  double quality() const;
  void setQuality(double qual);
  unsigned samplerate() const;
  void setSamplerate(unsigned rate);
  bool isFramed() const;
  Ringbuffer *ring();
  virtual bool isAvailable() const=0;
  virtual QString defaultExtension() const=0;
  static bool acceptsContentType(Type type,const QString &mimetype);
  static bool acceptsFormatIdentifier(Type type,const QString &fmt_id);
  static bool acceptsExtension(Type type,const QString &ext);
  static QString typeText(Codec::Type type);
  static QString optionKeyword(Codec::Type type);

 signals:
  void framed(unsigned chans,unsigned samprate,unsigned bitrate,
	      Ringbuffer *ring);
  void audioWritten(unsigned frames);

 public slots:
  virtual void process(const QByteArray &data)=0;

 protected:
  virtual void setFramed(unsigned chans,unsigned samprate,unsigned bitrate);

 private:
  Ringbuffer *codec_ring;
  unsigned codec_bitrate;
  unsigned codec_channels;
  double codec_quality;
  unsigned codec_samplerate;
  float *codec_pcm_in;
  float *codec_pcm_out;
  float *codec_pcm_buffer[2];
  bool codec_is_framed;
  Codec::Type codec_type;
};


#endif  // CODEC_H
