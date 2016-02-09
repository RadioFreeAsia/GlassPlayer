// conn_file.cpp
//
// Server connector for static files.
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

#include <QByteArray>
#include <QStringList>

#include "conn_file.h"
#include "logging.h"

File::File(QObject *parent)
  : Connector(parent)
{
  file_header="";
  file_header_active=false;
  file_result_code=0;
  file_metadata_istate=0;
  file_metadata_interval=0;
  file_metadata_counter=0;

  file_socket=NULL;
  InitSocket();

  //
  // Watchdog Timer
  //
  file_watchdog_retry_timer=new QTimer(this);
  file_watchdog_retry_timer->setSingleShot(true);
  connect(file_watchdog_retry_timer,SIGNAL(timeout()),
	  this,SLOT(watchdogRetryData()));
}


File::~File()
{
  delete file_socket;
}


Connector::ServerType File::serverType() const
{
  return Connector::FileServer;
}


void File::reset()
{
  file_socket->disconnect();
  setConnected(false);
  file_watchdog_retry_timer->start(FILE_WATCHDOG_RETRY_INTERVAL);
}


void File::connectToHostConnector()
{
  //  file_socket->connectToHost(hostname,port);
}


void File::disconnectFromHostConnector()
{
}


void File::connectedData()
{
  file_header_active=true;
  file_result_code=0;
  file_metadata_interval=0;
  file_metadata_istate=0;
  file_metadata_string="";
  file_metadata_counter=0;
  file_byte_counter=0;
  file_is_shoutcast=false;
  SendHeader("GET "+serverMountpoint()+" HTTP/1.1");
  SendHeader("Host: "+serverUrl().host()+":"+
	     QString().sprintf("%u",serverUrl().port(80)));
  SendHeader(QString().sprintf("icy-metadata: %d",streamMetadataEnabled()));
  SendHeader("Accept: */*");
  SendHeader("User-Agent: glassplayer/"+QString(VERSION));
  SendHeader("Cache-control: no-cache");
  SendHeader("Connection: close");
  SendHeader("");
}


void File::readyReadData()
{
  QByteArray data;

  while(file_socket->bytesAvailable()>0) {
    data=file_socket->read(1024);
    if(file_header_active) {   // Get headers
      for(int i=0;i<data.length();i++) {
	switch(0xFF&data.data()[i]) {
	case 13:
	  if(!file_header.isEmpty()) {
	    ProcessHeader(file_header);
	  }
	  break;

	case 10:
	  if(file_header.isEmpty()) {
	    file_header_active=false;
	    if((file_result_code>=200)&&(file_result_code<300)) {
	      setConnected(true);
	      data=data.right(data.length()-i-1);
	      ProcessFrames(data);
	    }
	    else {
	      reset();
	    }
	    return;
	  }
	  file_header="";
	  break;

	default:
	  file_header+=data.data()[i];
	  break;
	}
      }
    }
    else {
      ProcessFrames(data);
    }
  }
}


void File::errorData(QAbstractSocket::SocketError err)
{
  switch(err) {
  case QAbstractSocket::HostNotFoundError:
    Log(LOG_ERR,"glassplayer: host not found\n");
    exit(255);

  default:
    setConnected(false);
    file_watchdog_retry_timer->start(FILE_WATCHDOG_RETRY_INTERVAL);
    Log(LOG_WARNING,tr("server connection lost")+
	QString().sprintf(" [error: %u], ",err)+tr("attempting reconnect"));
    break;
  }
}


void File::loadStats(QStringList *hdrs,QStringList *values)
{
  hdrs->push_back("ConnectorType");
  if(file_is_shoutcast) {
    values->push_back("Shoutcast");
  }
  else {
    values->push_back("Icecast");
  }

  if(!file_server.isEmpty()) {
    hdrs->push_back("ConnectorServer");
    values->push_back(file_server);
  }

  hdrs->push_back("ConnectorMimetype");
  values->push_back(file_content_type);
}


void File::watchdogRetryData()
{
  InitSocket();
  connectToServer();
}


void File::ProcessFrames(QByteArray &data)
{
  int md_start=0;
  int md_len=0;

  if(file_metadata_interval>0) {
    if(file_metadata_counter+data.length()>file_metadata_interval) {
      md_start=file_metadata_interval-file_metadata_counter;
      md_len=0xFF&data[md_start]*16;
      ProcessMetadata(data.mid(md_start+1,md_len));
      file_metadata_counter=data.size()-(md_start+md_len+1);
      data.remove(md_start,md_len+1);
    }
    else {
      file_metadata_counter+=data.length();
    }
  }
  file_byte_counter+=data.length();
  emit dataReceived(data);
}


void File::SendHeader(const QString &str)
{
  file_socket->write((str+"\r\n").toUtf8(),str.length()+2);
}


void File::ProcessHeader(const QString &str)
{
  QStringList f0;

  //  fprintf(stderr,"%s\n",(const char *)str.toUtf8());

  if(file_result_code==0) {
    f0=str.split(" ",QString::SkipEmptyParts);
    if(f0.size()<3) {
      Log(LOG_ERR,"malformed response from server ["+str+"]");
      exit(256);
    }
    file_is_shoutcast=f0[0]=="ICY";
    file_result_code=f0[1].toInt();
    if((file_result_code<200)||(file_result_code>=300)) {
      f0.erase(f0.begin());
      Log(LOG_ERR,"server returned error ["+f0.join(" ")+"]");
      // exit(256);
    }
  }
  else {
    f0=str.split(":");
    if(f0.size()==2) {
      QString hdr=f0[0].trimmed().toLower();
      QString value=f0[1].trimmed();
      if(hdr=="content-type") {
	file_content_type=value;
	for(int i=0;i<Codec::TypeLast;i++) {
	  if(Codec::acceptsContentType((Codec::Type)i,value)) {
	    setCodecType((Codec::Type)i);
	  }
	}
      }
      if(hdr=="server") {
	file_server=value;
      }
      if(hdr=="icy-br") {
	setAudioBitrate(value.toInt());
      }
      if(hdr=="icy-description") {
	setStreamDescription(value);
      }
      if(hdr=="icy-genre") {
	setStreamGenre(value);
      }
      if(hdr=="icy-metaint") {
	file_metadata_interval=value.toInt();
      }
      if(hdr=="icy-name") {
	setStreamName(value);
      }
      if(hdr=="icy-public") {
	setStreamPublic(value.toInt()!=0);
      }
      if(hdr=="icy-url") {
	setStreamUrl(value);
      }
    }
  }
}


void File::ProcessMetadata(const QByteArray &mdata)
{
  if(mdata.length()>0) {
    //fprintf(stderr,"METADATA: %s\n",(const char *)f0[i].toUtf8());
    MetaEvent *e=new MetaEvent();
    QStringList f0=QString(mdata).split(";",QString::SkipEmptyParts);
    for(int i=0;i<f0.size();i++) {
      QStringList f1=f0[i].split("=");
      if(f1[0]=="StreamTitle") {
	f1.erase(f1.begin());
	QString title=f1.join("=");
	e->setField(MetaEvent::Title,title.mid(1,title.length()-2));
	continue;
      }
      if(f1[0]=="StreamUrl") {
	f1.erase(f1.begin());
	QString url=f1.join("=");
	e->setField(MetaEvent::Url,url.mid(1,url.length()-2));
	continue;
      }
    }
    emit metadataReceived(file_byte_counter,e);
    delete e;
  }
}


void File::InitSocket()
{
  if(file_socket!=NULL) {
    delete file_socket;
  }
  file_socket=new QTcpSocket(this);
  connect(file_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(file_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(file_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}
