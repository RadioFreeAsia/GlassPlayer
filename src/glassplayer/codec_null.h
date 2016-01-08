// codec_null.h
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

#ifndef CODEC_NULL_H
#define CODEC_NULL_H

#include "codec.h"

class CodecNull : public Codec
{
  Q_OBJECT;
 public:
  CodecNull(Ringbuffer *ring,QObject *parent=0);
  ~CodecNull();
  bool isAvailable() const;
  QString contentType() const;
  QString defaultExtension() const;
  QString formatIdentifier() const;
  void process(const QByteArray &data);

 protected:
  bool startCodec();
};


#endif  // CODEC_NULL_H
