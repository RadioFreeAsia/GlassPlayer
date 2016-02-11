// codec_pass.h
//
// Passthrough Codec
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

#ifndef CODEC_PASS_H
#define CODEC_PASS_H

#include "codec.h"

class CodecPassthrough : public Codec
{
  Q_OBJECT;
 public:
  CodecPassthrough(unsigned bitrate,QObject *parent=0);
  ~CodecPassthrough();
  bool isAvailable() const;
  QString defaultExtension() const;
  void process(const QByteArray &data,bool is_last);

 protected:
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);
};


#endif  // CODEC_PASS_H
