// conn_xcast.cpp
//
// Server connector for Icecast/Shoutcast streams.
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

#include <QByteArray>
#include <QStringList>

#include "conn_xcast.h"
#include "logging.h"

XCast::XCast(const QString &mimetype,QObject *parent)
  : Connector(mimetype,parent)
{
  xcast_header="";
  xcast_header_active=false;
  xcast_result_code=0;
  xcast_metadata_istate=0;
  xcast_metadata_interval=0;
  xcast_metadata_counter=0;

  xcast_socket=NULL;
  InitSocket();

  //
  // Watchdog Timer
  //
  xcast_watchdog_retry_timer=new QTimer(this);
  xcast_watchdog_retry_timer->setSingleShot(true);
  connect(xcast_watchdog_retry_timer,SIGNAL(timeout()),
	  this,SLOT(watchdogRetryData()));
}


XCast::~XCast()
{
  delete xcast_socket;
}


Connector::ServerType XCast::serverType() const
{
  return Connector::XCastServer;
}


void XCast::reset()
{
  xcast_socket->disconnect();
  setConnected(false);
  xcast_watchdog_retry_timer->start(XCAST_WATCHDOG_RETRY_INTERVAL);
}


void XCast::connectToHostConnector()
{
  xcast_socket->connectToHost(serverUrl().host(),serverUrl().port(80));
}


void XCast::disconnectFromHostConnector()
{
}


void XCast::connectedData()
{
  xcast_header_active=true;
  xcast_result_code=0;
  xcast_metadata_interval=0;
  xcast_metadata_istate=0;
  xcast_metadata_string="";
  xcast_metadata_counter=0;
  xcast_byte_counter=0;
  xcast_is_shoutcast=false;
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


void XCast::readyReadData()
{
  QByteArray data;

  while(xcast_socket->bytesAvailable()>0) {
    data=xcast_socket->read(1024);
    if(xcast_header_active) {   // Get headers
      for(int i=0;i<data.length();i++) {
	switch(0xFF&data.data()[i]) {
	case 13:
	  if(!xcast_header.isEmpty()) {
	    ProcessHeader(xcast_header);
	  }
	  break;

	case 10:
	  if(xcast_header.isEmpty()) {
	    xcast_header_active=false;
	    if((xcast_result_code>=200)&&(xcast_result_code<300)) {
	      setConnected(true);
	      data=data.right(data.length()-i-1);
	      ProcessFrames(data);
	    }
	    else {
	      reset();
	    }
	    return;
	  }
	  xcast_header="";
	  break;

	default:
	  xcast_header+=data.data()[i];
	  break;
	}
      }
    }
    else {
      ProcessFrames(data);
    }
  }
}


void XCast::errorData(QAbstractSocket::SocketError err)
{
  switch(err) {
  case QAbstractSocket::HostNotFoundError:
    Log(LOG_ERR,"glassplayer: host not found\n");
    exit(GLASS_EXIT_HTTP_ERROR);

  default:
    setConnected(false);
    xcast_watchdog_retry_timer->start(XCAST_WATCHDOG_RETRY_INTERVAL);
    Log(LOG_WARNING,tr("server connection lost")+
	QString().sprintf(" [error: %u], ",err)+tr("attempting reconnect"));
    break;
  }
}


void XCast::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
  if(is_first) {
    hdrs->push_back("Connector|Type");
    if(xcast_is_shoutcast) {
      values->push_back("Shoutcast");
    }
    else {
      values->push_back("Icecast");
    }

    if(!xcast_server.isEmpty()) {
      hdrs->push_back("Connector|Server");
      values->push_back(xcast_server);
    }

    hdrs->push_back("Connector|ContentType");
    values->push_back(xcast_content_type);
  }
}


void XCast::watchdogRetryData()
{
  InitSocket();
  connectToServer();
}


void XCast::ProcessFrames(QByteArray &data)
{
  int md_start=0;
  int md_len=0;

  if(xcast_metadata_interval>0) {
    if(xcast_metadata_counter+data.length()>xcast_metadata_interval) {
      md_start=xcast_metadata_interval-xcast_metadata_counter;
      md_len=0xFF&data[md_start]*16;
      ProcessMetadata(data.mid(md_start+1,md_len));
      xcast_metadata_counter=data.size()-(md_start+md_len+1);
      data.remove(md_start,md_len+1);
    }
    else {
      xcast_metadata_counter+=data.length();
    }
  }
  xcast_byte_counter+=data.length();
  emit dataReceived(data,false);
}


void XCast::SendHeader(const QString &str)
{
  xcast_socket->write((str+"\r\n").toUtf8(),str.length()+2);
}


void XCast::ProcessHeader(const QString &str)
{
  QStringList f0;

  //  fprintf(stderr,"%s\n",(const char *)str.toUtf8());

  if(xcast_result_code==0) {
    f0=str.split(" ",QString::SkipEmptyParts);
    if(f0.size()<3) {
      Log(LOG_ERR,"malformed response from server ["+str+"]");
      exit(GLASS_EXIT_SERVER_ERROR);
    }
    xcast_is_shoutcast=f0[0]=="ICY";
    xcast_result_code=f0[1].toInt();
    if((xcast_result_code<200)||(xcast_result_code>=300)) {
      f0.erase(f0.begin());
      Log(LOG_ERR,"server returned error ["+f0.join(" ")+"]");
    }
  }
  else {
    f0=str.split(":");
    if(f0.size()==2) {
      QString hdr=f0[0].trimmed().toLower();
      QString value=f0[1].trimmed();
      if(hdr=="content-type") {
	xcast_content_type=value;
	for(int i=0;i<Codec::TypeLast;i++) {
	  if(Codec::acceptsContentType((Codec::Type)i,value)) {
	    setCodecType((Codec::Type)i);
	  }
	}
      }
      if(hdr=="server") {
	xcast_server=value;
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
	xcast_metadata_interval=value.toInt();
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


void XCast::ProcessMetadata(const QByteArray &mdata)
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
    emit metadataReceived(xcast_byte_counter,e);
    delete e;
  }
}


void XCast::InitSocket()
{
  if(xcast_socket!=NULL) {
    delete xcast_socket;
  }
  xcast_socket=new QTcpSocket(this);
  connect(xcast_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(xcast_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(xcast_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}
