// id3parser.h
//
// Extract ID3 tags from an MPEG/AAC Bitstream
//
//   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ID3PARSER_H
#define ID3PARSER_H

#include <QByteArray>

class Id3Parser
{
 public:
  Id3Parser();
  void parse(QByteArray &data);
  void reset();

 private:
  int parser_bytes_processed;
};


#endif  // ID3PARSER_H
