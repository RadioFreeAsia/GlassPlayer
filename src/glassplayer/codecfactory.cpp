// codecfactory.cpp
//
// Instantiate Codec classes.
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

#include "codec_fdk.h"
#include "codec_mpeg1.h"
#include "codec_null.h"
#include "codec_pass.h"
#include "codecfactory.h"

Codec *CodecFactory(Codec::Type type,unsigned bitrate,QObject *parent)
{
  Codec *codec=NULL;

  switch(type) {
  case Codec::TypeAac:
    codec=new CodecFdk(bitrate,parent);
    break;

  case Codec::TypeMpeg1:
    codec=new CodecMpeg1(bitrate,parent);
    break;
 
  case Codec::TypeVorbis:
    break;

  case Codec::TypeNull:
    codec=new CodecNull(bitrate,parent);
    break;

  case Codec::TypePassthrough:
    codec=new CodecPassthrough(bitrate,parent);
    break;

  case Codec::TypeLast:
    break;
  }

  return codec;
}
