// id3parser.cpp
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

#include <stdio.h>

#include "id3parser.h"

Id3Parser::Id3Parser()
{
  reset();
}


void Id3Parser::parse(QByteArray &data)
{
  /*
  int offset=-3;

  while((offset=data.indexOf("ID3",offset+3))>=0) {
    if(data.size()>offset+10) {
      if((0xFF&data[offset+3])==0x04) {  // ID3v2.4
	printf("ID3 at %d, size: %d\n",parser_bytes_processed+offset,
	       ((0xFF&data[offset+6])*2048383)+
	       ((0xFF&data[offset+7])*16129)+
	       ((0xFF&data[offset+8])*127)+
	       (0xFF&data[offset+9])+
	       10);
      }
    }
  }
  */
  int offset=0;
  int bytes_removed=0;

  //
  // Find ID3 Tags
  //
  while((offset=data.indexOf("ID3",offset))>=0) {
    if((data.size()>offset+10)&&
       ((0xFF&data[offset+3])==0x04)&&   // From ID3v2.4 Section 3.1
       ((0xFF&data[offset+4])<0xFF)&&
       ((0xFF&data[offset+6])<0x80)&&
       ((0xFF&data[offset+7])<0x80)&&
       ((0xFF&data[offset+8])<0x80)&&
       ((0xFF&data[offset+9])<0x80)) {
      int tag_size=((0xFF&data[offset+6])*2048383)+  // Synchsafe integer
	((0xFF&data[offset+7])*16129)+
	((0xFF&data[offset+8])*127)+
	(0xFF&data[offset+9])+
	10;
      if((0x08&data[offset+5])!=0) {  // Check for Footer
	tag_size+=10;
      }
      printf("ID3 at: %d:%d\n",offset,tag_size);
      //      data.remove(offset,tag_size);
      //bytes_removed+=tag_size;
      offset+=tag_size;
    }
    else {
      offset+=3;
    }
  }

  parser_bytes_processed+=data.size();
}


void Id3Parser::reset()
{
  parser_bytes_processed=0;
}
