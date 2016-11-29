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

#include "codec_ogg.h"

CodecOgg::CodecOgg(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeOgg,bitrate,parent)
{
#ifdef HAVE_OGG
  ogg_istate=0;
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
  char *os_buffer;
  float ipcm[4096];

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
      if(!ogg_stream_packetout(&os,&op)) {
	fprintf(stderr,"error reading initial header packet\n");
	return;
      }
      if(vorbis_synthesis_headerin(&vi,&vc,&op)) {
	ogg_istate=1;
      }
      break;

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
	float **pcm;
	int samples;

	if(vorbis_synthesis(&vb,&op)==0) {
	  vorbis_synthesis_blockin(&vd,&vb);
	  while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0) {
	    int bout=(samples<4096?samples:4096);
	    Codec::interleave(ipcm,pcm,vi.channels,bout);
	    writePcm(ipcm,bout,false);
	    vorbis_synthesis_read(&vd,bout);
	  }
	}
      }
      break;
    }
  }
#endif  // HAVE_OGG
}


void CodecOgg::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
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
