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
#include "logging.h"

CodecOgg::CodecOgg(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeOgg,bitrate,parent)
{
#ifdef HAVE_OGG
  ogg_ogg_handle=NULL;
  ogg_vorbis_handle=NULL;
  ogg_opus_handle=NULL;
  LoadOgg();
#endif  // HAVE_OGG
}


CodecOgg::~CodecOgg()
{
}


bool CodecOgg::isAvailable() const
{
#ifdef HAVE_OGG
  return ogg_vorbis_handle!=NULL;
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
  unsigned chans=0;
  unsigned samprate=0;

  if(data.size()==0) {
    return;
  }
  os_buffer=ogg_sync_buffer(&ogg_oy,data.length());
  memcpy(os_buffer,data,data.length());
  ogg_sync_wrote(&ogg_oy,data.length());
  while(ogg_sync_pageout(&ogg_oy,&ogg_og)) {
    switch(ogg_istate) {
    case 0:  // OGG: Stream Startup
    case 1:
      ogg_stream_init(&ogg_os,ogg_page_serialno(&ogg_og));
      if(ogg_stream_pagein(&ogg_os,&ogg_og)<0) {
	Log(LOG_ERR,"Ogg stream version mismatch");
	exit(3);
      }
      if(!ogg_stream_packetout(&ogg_os,&ogg_op)) {
	Log(LOG_ERR,"Ogg stream error reading initial header packet");
	exit(3);
      }
      if((ogg_op.bytes>=7)&&(memcmp(ogg_op.packet+1,"vorbis",6)==0)) {
	ogg_codec_type=CodecOgg::Vorbis;
	if(vorbis_synthesis_headerin(&vi,&vc,&ogg_op)) {
	  ogg_istate=10;
	}
      }
      if((ogg_op.bytes>=8)&&(memcmp(ogg_op.packet,"OpusHead",8)==0)) {
	if(!ParseOpusHeader(&samprate,&chans,&ogg_op)) {
	  Log(LOG_ERR,"Invalid OggOpus header");
	  exit(3);
	}
	ogg_codec_type=CodecOgg::Opus;
	if((ogg_opus_decoder=opus_decoder_create(samprate,chans,&err))==NULL) {
	  Log(LOG_ERR,QString().sprintf("OggOpus decoder error %d",err));
	  exit(3);
	}
	setFramed(chans,samprate,0);
	ogg_istate=20;
      }
      if(ogg_istate<10) {
	ogg_istate++;
      }
      break;

      // *********************************************************************
      // * ERROR HANDLER - We found no info header that we recognized
      // *********************************************************************
    case 2:
      for(int i=0;i<ogg_op.bytes;i++) {
	if(isprint(0xFF&ogg_op.packet[i])) {
	  for(int j=i;j<ogg_op.bytes;j++) {
	    if(!isprint(0xFF&ogg_op.packet[j])) {
	      QByteArray str((const char *)ogg_op.packet+i,j-i);
	      Log(LOG_ERR,
		  QString().sprintf("Unsupported Ogg-encoded codec [%s]",
				    str.constData()));
	      exit(1);
	    }
	  }
	}
      }
      Log(LOG_ERR,"Unsupported Ogg-encoded codec");
      exit(1);
      break;

      // *********************************************************************
      // * VORBIS Decode
      // *********************************************************************
    case 10:  // VORBIS: Read info/comment headers
    case 11:
      ogg_stream_pagein(&ogg_os,&ogg_og);
      if(TriState(ogg_stream_packetout(&ogg_os,&ogg_op),"Corrupt header packet")) {
	TriState(vorbis_synthesis_headerin(&vi,&vc,&ogg_op),"Corrupt header");
	ogg_istate++;
      }
      break;

    case 12:   // VORBIS: Initialize DSP decoder
      ogg_vendor_string=vc.vendor;
      if(vorbis_synthesis_init(&vd,&vi)==0) {
	vorbis_block_init(&vd,&vb);
	setFramed(vi.channels,vi.rate,0);
      }
      ogg_istate=13;
      break;

    case 13:    // VORBIS: Decode Loop
      ogg_stream_pagein(&ogg_os,&ogg_og);
      while(ogg_stream_packetout(&ogg_os,&ogg_op)) {
	if(vorbis_synthesis(&vb,&ogg_op)==0) {
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
    case 20:   // OPUS: Get comment header
      ogg_stream_pagein(&ogg_os,&ogg_og);
      if(ogg_stream_packetout(&ogg_os,&ogg_op)) {
	ogg_vendor_string=CommentString(ogg_op.packet+8);
	ogg_istate=21;
      }
      break;

    case 21:   // OPUS: Decode Loop
      ogg_stream_pagein(&ogg_os,&ogg_og);
      while(ogg_stream_packetout(&ogg_os,&ogg_op)) {
	if((frames=opus_decode_float(ogg_opus_decoder,ogg_op.packet,ogg_op.bytes,ipcm,5760,0))>0) {
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

    if(!QString(ogg_vendor_string).isEmpty()) {
      hdrs->push_back("Codec|Encoder");
      values->push_back(ogg_vendor_string);
    }
  }
#endif  // HAVE_OGG
}


bool CodecOgg::TriState(int result,const QString &err_msg)
{
  if(result<0) {
    Log(LOG_ERR,"Ogg Initialization Error: "+err_msg);
    exit(3);
  }
  return result;
}


QString CodecOgg::CommentString(const unsigned char *str) const
{
  uint32_t len=(0xFF&str[0])+((0xFF&str[1])<<8)+
    ((0xFF&str[2])<<16)+((0xFF&str[3])<<24);
  return QString::fromUtf8((const char *)str+4,len);
}


bool CodecOgg::LoadOgg()
{
#ifdef HAVE_OGG
  //
  // Initialize Libogg
  //
  if((ogg_ogg_handle=dlopen("libogg.so.0",RTLD_NOW))!=NULL) {
    *(void **)(&ogg_sync_init)=dlsym(ogg_ogg_handle,"ogg_sync_init");
    *(void **)(&ogg_sync_clear)=dlsym(ogg_ogg_handle,"ogg_sync_clear");
    *(void **)(&ogg_sync_reset)=dlsym(ogg_ogg_handle,"ogg_sync_reset");
    *(void **)(&ogg_sync_buffer)=dlsym(ogg_ogg_handle,"ogg_sync_buffer");
    *(void **)(&ogg_sync_pageout)=dlsym(ogg_ogg_handle,"ogg_sync_pageout");
    *(void **)(&ogg_sync_pagein)=dlsym(ogg_ogg_handle,"ogg_sync_pagein");
    *(void **)(&ogg_sync_wrote)=dlsym(ogg_ogg_handle,"ogg_sync_wrote");
    *(void **)(&ogg_stream_init)=dlsym(ogg_ogg_handle,"ogg_stream_init");
    *(void **)(&ogg_stream_pagein)=dlsym(ogg_ogg_handle,"ogg_stream_pagein");
    *(void **)(&ogg_stream_packetout)=
      dlsym(ogg_ogg_handle,"ogg_stream_packetout");
    *(void **)(&ogg_page_serialno)=dlsym(ogg_ogg_handle,"ogg_page_serialno");

  //
  // Initialize Libvorbis
  //
    if((ogg_vorbis_handle=dlopen("libvorbis.so.0",RTLD_NOW))!=NULL) {
      *(void **)(&vorbis_info_init)=
	dlsym(ogg_vorbis_handle,"vorbis_info_init");
      *(void **)(&vorbis_info_clear)=
	dlsym(ogg_vorbis_handle,"vorbis_info_clear");
      *(void **)(&vorbis_comment_init)=
	dlsym(ogg_vorbis_handle,"vorbis_comment_init");
      *(void **)(&vorbis_comment_clear)=
	dlsym(ogg_vorbis_handle,"vorbis_comment_clear");
      *(void **)(&vorbis_block_init)=
	dlsym(ogg_vorbis_handle,"vorbis_block_init");
      *(void **)(&vorbis_synthesis)=dlsym(ogg_vorbis_handle,"vorbis_synthesis");
      *(void **)(&vorbis_synthesis_headerin)=
	dlsym(ogg_vorbis_handle,"vorbis_synthesis_headerin");
      *(void **)(&vorbis_synthesis_init)=
	dlsym(ogg_vorbis_handle,"vorbis_synthesis_init");
      *(void **)(&vorbis_synthesis_blockin)=
	dlsym(ogg_vorbis_handle,"vorbis_synthesis_blockin");
      *(void **)(&vorbis_synthesis_pcmout)=
	dlsym(ogg_vorbis_handle,"vorbis_synthesis_pcmout");
      *(void **)(&vorbis_synthesis_read)=
	dlsym(ogg_vorbis_handle,"vorbis_synthesis_read");

      if((ogg_opus_handle=dlopen("libopus.so.0",RTLD_NOW))!=NULL) {
	*(void **)(&opus_decoder_create)=
	  dlsym(ogg_opus_handle,"opus_decoder_create");
	*(void **)(&opus_decode_float)=
	  dlsym(ogg_opus_handle,"opus_decode_float");

	ogg_codec_type=CodecOgg::Unknown;
	ogg_istate=0;
	ogg_sync_init(&ogg_oy);
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);
	return true;
      }
    }
  }
  return false;
#endif  // HAVE_OGG
  return false;
}


#ifdef HAVE_OGG
bool CodecOgg::ParseOpusHeader(unsigned *samprate,unsigned *chans,
			       ogg_packet *op)
{
  if(op->bytes<18) {
    Log(LOG_ERR,"Invalid/truncated OggOpus header");
    exit(3);
  }
  *chans=0xFF&op->packet[9];
  *samprate=((0xFF&op->packet[15])<<24)+
    ((0xFF&op->packet[14])<<16)+
    ((0xFF&op->packet[13])<<8)+
    (0xFF&op->packet[12]);
  if((0xFF&op->packet[18])!=0) {
    Log(LOG_ERR,"Unsupported channel count");
    exit(14);
  }
  return true;
}
#endif  // HAVE_OGG
