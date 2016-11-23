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

#include <stdio.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QStringList>

#include "codec.h"
#include "logging.h"

Codec::Codec(Codec::Type type,unsigned bitrate,QObject *parent)
{
  codec_type=type;
  codec_bitrate=bitrate;
  codec_ring=NULL;
  codec_channels=2;
  codec_quality=0.5;
  codec_samplerate=48000;
  codec_is_framed=false;
  codec_is_framed_changed=true;
  codec_bytes_processed=0;
  codec_bytes_processed_changed=true;
  codec_pcm_in=NULL;
  codec_pcm_out=NULL;
  codec_pcm_buffer[0]=NULL;
  codec_pcm_buffer[1]=NULL;
}


Codec::~Codec()
{
  if(codec_ring!=NULL) {
    delete codec_ring;
  }
}


Codec::Type Codec::type() const
{
  return codec_type;
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


uint64_t Codec::bytesProcessed() const
{
  return codec_bytes_processed;
}


uint64_t Codec::framesGenerated() const
{
  return codec_frames_generated;
}


Ringbuffer *Codec::ring()
{
  return codec_ring;
}


void Codec::getStats(QStringList *hdrs,QStringList *values,bool is_first)
{
  if(codec_is_framed_changed) {
    hdrs->push_back("Codec|Framed");
    if(isFramed()) {
      values->push_back("Yes");
    }
    else {
      values->push_back("No");
    }
    codec_is_framed_changed=false;
  }

  if(is_first) {
    hdrs->push_back("Codec|Channels");
    values->push_back(QString().sprintf("%u",codec_channels));

    hdrs->push_back("Codec|Samplerate");
    values->push_back(QString().sprintf("%u",codec_samplerate));
  }

  if(codec_bytes_processed_changed) {
    hdrs->push_back("Codec|Bytes Processed");
    values->push_back(QString().sprintf("%lu",codec_bytes_processed));
    codec_bytes_processed_changed=false;
  }

  loadStats(hdrs,values,is_first);
}


bool Codec::acceptsContentType(Type type,const QString &mimetype)
{
  bool ret=false;
  QString mime=mimetype.toLower().trimmed();

  switch(type) {
  case Codec::TypeNull:
    ret=true;
    break;

  case Codec::TypeMpeg1:
    ret=(mime=="audio/mpeg")||(mime=="audio/mpeg3")||(mime=="audio/x-mpeg3")||
      (mime=="audio/x-mpeg");
    break;
 
  case Codec::TypeOgg:
    ret=(mime=="audio/ogg")||(mime=="application/ogg");
    break;

  case Codec::TypeAac:
    ret=mime=="audio/aacp";
    break;

  case Codec::TypePassthrough:
    ret=(mime=="audio/x-wav")||(mime=="audio/wav")||(mime=="audio/wave")||
      (mime=="audio/vnd.wave")||(mime=="audio/aiff")||(mime=="audio/x-aiff")||
      (mime=="audio/basic")||(mime=="audio/x-au")||(mime=="audio/voc")||
      (mime=="audio/x-voc")||(mime=="audio/x-adpcm")||(mime=="audio/x-flac");
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
 
  case Codec::TypeOgg:
    break;

  case Codec::TypeAac:
    ret=(fmt_id=="mp4a.40.1")||(fmt_id=="mp4a.40.2")||(fmt_id=="mp4a.40.5");
    break;

  case Codec::TypePassthrough:
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


bool Codec::acceptsExtension(Type type,const QString &ext)
{
  bool ret=false;

  switch(type) {
  case Codec::TypeNull:
    ret=true;
    break;

  case Codec::TypeMpeg1:
    ret=(ext.toLower()=="mp2")||(ext.toLower()=="mp3");
    break;
 
  case Codec::TypeOgg:
    break;

  case Codec::TypeAac:
    ret=ext.toLower()=="aac";
    break;

  case Codec::TypePassthrough:
    ret=ext.toLower()=="wav";
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


QString Codec::typeText(Codec::Type type)
{
  QString ret=tr("Unknown");

  switch(type) {
  case Codec::TypeAac:
    ret=tr("AAC");
    break;

  case Codec::TypeMpeg1:
    ret=tr("MPEG-1");
    break;
 
  case Codec::TypeOgg:
    ret=tr("Ogg Bitstream");
    break;

  case Codec::TypeNull:
    ret=tr("Null");
    break;

  case Codec::TypePassthrough:
    ret=tr("PCM Passthrough");
    break;

  case Codec::TypeLast:
    break;
  }

  return ret;
}


QString Codec::optionKeyword(Codec::Type type)
{
  QString ret;

  switch(type) {
  case Codec::TypeAac:
    ret="aac";
    break;

  case Codec::TypeMpeg1:
    ret="mpeg";
    break;
 
  case Codec::TypeOgg:
    ret="ogg";
    break;
 
  case Codec::TypePassthrough:
    ret="wav";
    break;
 
  case Codec::TypeNull:
  case Codec::TypeLast:
    break;
  }

  return ret;
}


void Codec::processBitstream(const QByteArray &data,bool is_last)
{
  codec_bytes_processed+=data.length();
  codec_bytes_processed_changed=true;
  process(data,is_last);
  while((codec_metadata_bytes.size()>0)&&
	(codec_metadata_bytes.front()<codec_bytes_processed)) {
    emit metadataReceived(codec_frames_generated,codec_metadata_events.front());
    codec_metadata_bytes.pop();
    delete codec_metadata_events.front();
    codec_metadata_events.pop();
  }
}


void Codec::processMetadata(uint64_t bytes,MetaEvent *e)
{
  codec_metadata_bytes.push(bytes);
  codec_metadata_events.push(new MetaEvent(*e));
}


void Codec::setFramed(unsigned chans,unsigned samprate,unsigned bitrate)
{
  codec_channels=chans;
  codec_samplerate=samprate;
  codec_bitrate=bitrate;
  codec_is_framed=true;
  codec_is_framed_changed=true;
  codec_bytes_processed=0;
  codec_bytes_processed_changed=true;
  codec_frames_generated=0;

  codec_ring=new Ringbuffer(CODEC_RINGBUFFER_SIZE,chans);

  emit framed(chans,samprate,bitrate,codec_ring);
}


void Codec::writePcm(float *pcm,unsigned frames,bool is_last)
{

  while(codec_ring->writeSpace()<frames) {
    usleep(100000);
  }
  codec_ring->write(pcm,frames);
  codec_frames_generated+=frames;
  emit audioWritten(frames,is_last);
  if(is_last) {
    ring()->setFinished();
  }
}
