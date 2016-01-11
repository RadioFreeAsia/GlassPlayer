// codec.cpp
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

#include "codec.h"
#include "logging.h"

Codec::Codec(Codec::Type type,unsigned bitrate,QObject *parent)
{
  codec_bitrate=bitrate;
  codec_channels=2;
  codec_quality=0.5;
  codec_samplerate=48000;
  codec_ring1=NULL;
  codec_ring2=NULL;
  codec_is_framed=false;
}


Codec::~Codec()
{
  if(codec_src_state!=NULL) {
    src_delete(codec_src_state);
  }
  if(codec_src_data!=NULL) {
    delete codec_src_data;
  }
  if((codec_ring2!=codec_ring1)&&(codec_ring2!=NULL)) {
    delete codec_ring2;
  }
}


unsigned Codec::bitrate() const
{
  return codec_bitrate;
}


unsigned Codec::channels() const
{
  return codec_channels;
}


void Codec::setChannels(unsigned chans)
{
  codec_channels=chans;
}


double Codec::quality() const
{
  return codec_quality;
}


void Codec::setQuality(double qual)
{
  codec_quality=qual;
}


unsigned Codec::samplerate() const
{
  return codec_samplerate;
}


void Codec::setSamplerate(unsigned rate)
{
  codec_samplerate=rate;
}


bool Codec::isFramed() const
{
  return codec_is_framed;
}


bool Codec::acceptsContentType(Type type,const QString &mimetype)
{
  bool ret=false;

  switch(type) {
  case Codec::TypeNull:
    ret=true;
    break;

  case Codec::TypeMpeg1:
    ret=mimetype.toLower()=="audio/mpeg";
    break;
 
  case Codec::TypeVorbis:
    ret=mimetype.toLower()=="audio/ogg";
    break;

  case Codec::TypeAac:
    ret=mimetype.toLower()=="audio/aac";
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


bool Codec::acceptsFormatIdentifier(Type type,const QString &fmt_id)
{
  bool ret=false;

  switch(type) {
  case Codec::TypeNull:
    ret=true;
    break;

  case Codec::TypeMpeg1:
    ret=(fmt_id=="mp4a.40.32")||(fmt_id=="mp4a.40.33")||(fmt_id=="mp4a.40.34");
    break;
 
  case Codec::TypeVorbis:
    break;

  case Codec::TypeAac:
    ret=(fmt_id=="mp4a.40.1")||(fmt_id=="mp4a.40.2")||(fmt_id=="mp4a.40.5");
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


QString Codec::codecTypeText(Codec::Type type)
{
  QString ret=tr("Unknown");

  switch(type) {
  case Codec::TypeAac:
    ret=tr("AAC");
    break;

  case Codec::TypeMpeg1:
    ret=tr("MPEG-1/1.5");
    break;
 
  case Codec::TypeVorbis:
    ret=tr("Ogg Vorbis");
    break;

  case Codec::TypeNull:
    ret=tr("Null");
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


void Codec::setFramed(unsigned chans,unsigned samprate,unsigned bitrate)
{
  codec_channels=chans;
  codec_samplerate=samprate;
  codec_bitrate=bitrate;
  codec_is_framed=true;
  emit framed();
}


Ringbuffer *Codec::ring()
{
  return codec_ring1;
}
