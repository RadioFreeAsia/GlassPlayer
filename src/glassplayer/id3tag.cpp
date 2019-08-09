// id3tag.cpp
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

#include <tbytevector.h>

#include "id3tag.h"

Id3Tag::Id3Tag(const QByteArray &data)
  : TagLib::ID3v2::Tag()
{
  TagLib::ByteVector bytes(data.constData(),data.size());
  header()->setData(bytes.mid(0,10));
  parse(bytes.mid(10));
}
