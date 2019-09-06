// jsonparser.cpp
//
// JSON parser
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

#include "jsonparser.h"

JsonParser::JsonParser(QObject *parent)
  : QObject(parent)
{
  json_indent_level=0;
}


void JsonParser::addData(QByteArray data)
{
  int next_open;
  int next_close;
  bool process_close;

  //  printf("JsonParser::addData(\"%s\")\n",(const char *)data);
  while(data.size()>0) {
    //printf("ACCUM: |%s| size: %d\n",(const char *)data,data.size());
    next_open=data.indexOf("{");
    next_close=data.indexOf("}");

    if((next_open<0)&&(next_close<0)) {
      json_accum+=data;
      data.clear();
      return;
    }
    if((next_open>=0)&&(next_close>=0)) {
      process_close=next_close<next_open;
    }
    else {
      process_close=next_open<0;
    }

    if(process_close) {
      json_indent_level--;
      if(json_indent_level<0) {
	printf("ERROR: negative level!\n");
	exit(1);
      }
      json_accum+=data.left(next_close+1);
      data.remove(0,next_close+1);
      if(json_indent_level==0) {
	emit newDocument(QJsonDocument::fromJson(json_accum));
	/*
	QJsonDocument doc=QJsonDocument::fromJson(json_accum);
	printf("*** START ***\n");
	printf("%s\n",(const char *)doc.toJson());
	printf("*** END ***\n");
	*/	
	json_accum.clear();
      }
    }
    else {
      json_indent_level++;
      json_accum+=data.left(next_open+1);
      data.remove(0,next_open+1);
    }
  }
}
