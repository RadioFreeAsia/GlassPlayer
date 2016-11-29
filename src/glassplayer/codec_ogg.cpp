// codec_ogg.cpp
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

#include <stdio.h>

#include <QStringList>

#include "codec_ogg.h"

CodecOgg::CodecOgg(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeOgg,bitrate,parent)
{
#ifdef HAVE_OGG
  ogg_codec_type=CodecOgg::Unknown;
  ogg_istate=0;
  page_granule=0;
  ogg_sync_init(&oy);
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);
#endif  // HAVE_OGG
}


CodecOgg::~CodecOgg()
{
}


bool CodecOgg::isAvailable() const
{
#ifdef HAVE_OGG
  return true;
#else
  return false;
#endif  // HAVE_OGG
}


QString CodecOgg::defaultExtension() const
{
  return QString("ogg");
}


void CodecOgg::process(const QByteArray &data,bool is_last)
{
#ifdef HAVE_OGG
  int err;
  char *os_buffer;
  float **pcm;
  int frames;
  float ipcm[11520];

  if(data.size()==0) {
    return;
  }
  os_buffer=ogg_sync_buffer(&oy,data.length());
  memcpy(os_buffer,data,data.length());
  ogg_sync_wrote(&oy,data.length());
  while(ogg_sync_pageout(&oy,&og)) {
    switch(ogg_istate) {
    case 0:  // OGG: Stream Startup
      ogg_stream_init(&os,ogg_page_serialno(&og));
      if(ogg_stream_pagein(&os,&og)<0) {
	fprintf(stderr,"stream version mismatch!\n");
	return;
      }
      page_granule=ogg_page_granulepos(&og);
      if(!ogg_stream_packetout(&os,&op)) {
	fprintf(stderr,"error reading initial header packet\n");
	return;
      }
      if((op.bytes>=7)&&(memcmp(op.packet+1,"vorbis",6)==0)) {
	ogg_codec_type=CodecOgg::Vorbis;
	if(vorbis_synthesis_headerin(&vi,&vc,&op)) {
	  ogg_istate=1;
	}
      }
      if((op.bytes>=8)&&(memcmp(op.packet,"OpusHead",8)==0)) {
	ogg_codec_type=CodecOgg::Opus;
	if((ogg_opus_decoder=opus_decoder_create(48000,2,&err))==NULL) {
	  fprintf(stderr,"Opus decoder error: %d\n",err);
	}
	setFramed(2,48000,0);
	ogg_istate=10;
      }
      break;

      // *********************************************************************
      // * VORBIS Decode
      // *********************************************************************
    case 1:  // VORBIS: Read info/comment headers
    case 2:
      ogg_stream_pagein(&os,&og);
      if(TriState(ogg_stream_packetout(&os,&op),"Corrupt header packet")) {
	TriState(vorbis_synthesis_headerin(&vi,&vc,&op),"Corrupt header");
	PrintVorbisComments();
	ogg_istate++;
      }
      break;

    case 3:   // VORBIS: Initialize DSP decoder
      if(vorbis_synthesis_init(&vd,&vi)==0) {
	vorbis_block_init(&vd,&vb);
	setFramed(vi.channels,vi.rate,0);
      }
      ogg_istate=4;
      break;

    case 4:    // VORBIS: Decode Loop
      ogg_stream_pagein(&os,&og);
      while(ogg_stream_packetout(&os,&op)) {
	if(vorbis_synthesis(&vb,&op)==0) {
	  vorbis_synthesis_blockin(&vd,&vb);
	  while((frames=vorbis_synthesis_pcmout(&vd,&pcm))>0) {
	    int bout=(frames<4096?frames:4096);
	    Codec::interleave(ipcm,pcm,vi.channels,bout);
	    writePcm(ipcm,bout,false);
	    vorbis_synthesis_read(&vd,bout);
	  }
	}
      }
      break;

      // *********************************************************************
      // * OPUS Decode
      // *********************************************************************
    case 10:
      ogg_stream_pagein(&os,&og);
      while(ogg_stream_packetout(&os,&op)) {
	if((frames=opus_decode_float(ogg_opus_decoder,op.packet,op.bytes,ipcm,5760,0))>0) {
	  writePcm(ipcm,frames,false);
	}
      }
      break;
    }
  }
#endif  // HAVE_OGG
}


void CodecOgg::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
#ifdef HAVE_OGG
  if(is_first) {
    switch(ogg_codec_type) {
    case CodecOgg::Vorbis:
      hdrs->push_back("Codec|Algorithm");
      values->push_back("OggVorbis");
      break;

    case CodecOgg::Opus:
      hdrs->push_back("Codec|Algorithm");
      values->push_back("OggOpus");
      break;

    case CodecOgg::Unknown:
      break;
    }

    hdrs->push_back("Codec|Channels");
    values->push_back(QString().sprintf("%u",channels()));
  }
#endif  // HAVE_OGG
}


bool CodecOgg::TriState(int result,const QString &err_msg)
{
  if(result<0) {
    fprintf(stderr,"Ogg Initialization Error: %s\n",
	    (const char *)err_msg.toUtf8());
    exit(256);
  }
  return result;
}


void CodecOgg::PrintVorbisComments() const
{
#ifdef HAVE_OGG
  char **ptr=vc.user_comments;
  while(*ptr){
    fprintf(stderr,"%s\n",*ptr);
    ++ptr;
  }
  fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
  fprintf(stderr,"Encoded by: %s\n\n",vc.vendor);
#endif  // HAVE_OGG
}
