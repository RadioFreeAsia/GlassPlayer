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

#include <stdlib.h>
#include <unistd.h>

#include "codec_null.h"

CodecNull::CodecNull(unsigned bitrate,QObject *parent)
  : Codec(Codec::TypeNull,bitrate,parent)
{
}


CodecNull::~CodecNull()
{
}


bool CodecNull::isAvailable() const
{
  return true;
}


bool CodecNull::acceptsContentType(const QString &str) const
{
  return true;
}


QString CodecNull::defaultExtension() const
{
  return QString("dat");
}


bool CodecNull::acceptsFormatIdentifier(const QString &str) const
{
  return true;
}


void CodecNull::process(const QByteArray &data,bool is_last)
{
  write(1,data.data(),data.length());
  if(is_last) {
    exit(0);
  }
}


void CodecNull::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
}
