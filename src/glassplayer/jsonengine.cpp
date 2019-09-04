// jsonengine.cpp
//
// JSON update generator
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

#include "jsonengine.h"

JsonEngine::JsonEngine()
{
}


void JsonEngine::addEvent(const QString &str)
{
  QStringList f0=str.split("|",QString::KeepEmptyParts);
  QString key=f0.at(0);
  f0.removeFirst();
  QStringList f1=f0.join("|").split(":",QString::KeepEmptyParts);
  for(int i=2;i<f1.size();i++) {
    f1[1]+=":"+f1.at(i);
  }
  for(int i=2;i<f1.size();i++) {
    f1.removeLast();
  }
  f1[1]=f1.at(1).trimmed();
  json_events.insert(key,f1);
}


void JsonEngine::addEvents(const QString &str)
{
  QStringList f0=str.split("\n",QString::SkipEmptyParts);
  for(int i=0;i<f0.size();i++) {
    addEvent(f0.at(i));
  }
}


QString JsonEngine::generate() const
{
  QString key="";
  QString json;
  bool ok=false;

  json="{\r\n";
  for(QMultiMap<QString,QStringList>::const_iterator it=json_events.begin();
      it!=json_events.end();it++) {
    if(key!=it.key()) {
      if(!key.isEmpty()) {
	json=json.left(json.length()-3)+"\r\n";
	json+="    }\r\n";
	json+="}\r\n";
	if((it+1)!=json_events.end()) {
	  json+="{\r\n";
	}
      }
      key=it.key();
      json+="    \""+JsonEngine::escape(key)+"\": {\r\n";
    }
    int num=it.value().at(1).toInt(&ok);
    if(ok) {
      json+="        \""+JsonEngine::escape(it.value().at(0))+"\": "+
	QString().sprintf("%d,\r\n",num);
    }
    else {
      json+="        \""+JsonEngine::escape(it.value().at(0))+"\": "+
	"\""+JsonEngine::escape(it.value().at(1))+"\",\r\n";
    }
  }
  // Close the last item
  json=json.left(json.length()-3)+"\r\n";
  json+="    }\r\n";
  json+="}\r\n";

  return json;
}


void JsonEngine::clear()
{
  json_events.clear();
}


QString JsonEngine::escape(const QString &str)
{
  QString ret="";

  for(int i=0;i<str.length();i++) {
    QChar c=str.at(i);
    switch(c.category()) {
    case QChar::Other_Control:
      ret+=QString().sprintf("\\u%04X",c.unicode());
      break;

    default:
      switch(c.unicode()) {
      case 0x22:   // Quote
	ret+="\\\"";
	break;

      case 0x5C:   // Backslash
	ret+="\\\\";
	break;

      default:
	ret+=c;
	break;
      }
      break;
    }
  }

  return ret;
}
