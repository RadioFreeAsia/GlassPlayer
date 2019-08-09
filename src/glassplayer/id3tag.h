// id3tag.h
//
// ID3 tag container class.
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

#ifndef ID3TAG_H
#define ID3TAG_H

#include <QByteArray>

#include <id3v2tag.h>

class Id3Tag : public TagLib::ID3v2::Tag
{
 public:
  Id3Tag(const QByteArray &data);
};


#endif  // ID3TAG_H
