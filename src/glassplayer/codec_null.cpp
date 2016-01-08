// codec_null.cpp
//
// Null codec that dumps the bitstream to standard output.
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

#include <unistd.h>

#include "codec_null.h"

CodecNull::CodecNull(Ringbuffer *ring,QObject *parent)
  : Codec(Codec::TypeNull,ring,parent)
{
}


CodecNull::~CodecNull()
{
}


bool CodecNull::isAvailable() const
{
  return true;
}


QString CodecNull::contentType() const
{
  return QString("application/octet-stream");
}


QString CodecNull::defaultExtension() const
{
  return QString("dat");
}


QString CodecNull::formatIdentifier() const
{
  return QString("");
}


void CodecNull::process(const QByteArray &data)
{
  write(1,data.data(),data.length());
}


bool CodecNull::startCodec()
{
  return true;
}
