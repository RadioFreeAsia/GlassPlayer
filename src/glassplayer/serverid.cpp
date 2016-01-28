// serverid.cpp
//
// Identify remote server
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

#include "logging.h"
#include "serverid.h"

ServerId::ServerId(QObject *parent)
  : QObject(parent)
{
  id_socket=new QTcpSocket(this);
  connect(id_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(id_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(id_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));

  id_kill_timer=new QTimer(this);
  id_kill_timer->setSingleShot(true);
  connect(id_kill_timer,SIGNAL(timeout()),this,SLOT(killData()));
}


ServerId::~ServerId()
{
  if(id_socket!=NULL) {
    delete id_socket;
  }
}


void ServerId::connectToServer(const QUrl &url)
{
  id_url=url;
  id_socket->connectToHost(url.host(),url.port(80));
}


void ServerId::connectedData()
{
  id_header_active=true;
  id_result_code=0;
  id_content_type="";
  SendHeader("GET "+id_url.path()+" HTTP/1.1");
  SendHeader("Host: "+id_url.host()+":"+QString().sprintf("%u",id_url.port()));
  SendHeader("Accept: */*");
  SendHeader("User-Agent: glassplayer/"+QString(VERSION));
  SendHeader("Cache-control: no-cache");
  SendHeader("Connection: close");
  SendHeader("");
}


void ServerId::readyReadData()
{
  QByteArray data;

  while(id_socket->bytesAvailable()>0) {
    data=id_socket->read(1024);
    for(int i=0;i<data.length();i++) {
      if(id_header_active) {   // Get headers
	switch(0xFF&data.data()[i]) {
	case 13:
	  if(!id_header.isEmpty()) {
	    ProcessHeader(id_header);
	  }
	  break;

	case 10:
	  if(id_header.isEmpty()) {
	    id_header_active=false;
	    if((id_result_code>=200)&&(id_result_code<300)) {
	      for(unsigned i=0;i<Connector::LastServer;i++) {
		if(Connector::acceptsContentType((Connector::ServerType)i,
						 id_content_type)) {
		  emit typeFound((Connector::ServerType)i,id_url);
		  id_kill_timer->start(0);
		  return;
		}
	      }
	    }
	  }
	  id_header="";
	  break;

	default:
	  id_header+=data.data()[i];
	  break;
	}
      }
      else {
	id_body+=data.data()[i];
      }
    }
  }
}


void ServerId::errorData(QAbstractSocket::SocketError err)
{
  switch(err) {
  case QAbstractSocket::RemoteHostClosedError:
    if(id_content_type.toLower()=="audio/x-mpegurl") {
      if(global_log_verbose) {
	Log(LOG_INFO,tr("using mountpoint")+": "+id_body.trimmed());
      }
      emit typeFound(Connector::XCastServer,QUrl(id_body.trimmed()));
      id_kill_timer->start(0);
      return;
    }
    break;

  default:
    Log(LOG_ERR,Connector::socketErrorText(err));
    exit(256);
    break;
  }
}


void ServerId::killData()
{
  delete id_socket;
  id_socket=NULL;
}


void ServerId::SendHeader(const QString &str)
{
  id_socket->write((str+"\r\n").toUtf8(),str.length()+2);
}


void ServerId::ProcessHeader(const QString &str)
{
  QStringList f0;

  //  fprintf(stderr,"%s\n",(const char *)str.toUtf8());

  if(id_result_code==0) {
    f0=str.split(" ",QString::SkipEmptyParts);
    if(f0.size()<3) {
      Log(LOG_ERR,"malformed response from server ["+str+"]");
      exit(256);
    }
    id_result_code=f0[1].toInt();
    if((id_result_code<200)||(id_result_code>=300)) {
      f0.erase(f0.begin());
      Log(LOG_ERR,"server returned error ["+f0.join(" ")+"]");
      exit(256);
    }
  }
  else {
    f0=str.split(":");
    if(f0.size()==2) {
      QString hdr=f0[0].trimmed().toLower();
      QString value=f0[1].trimmed();
      if(hdr=="content-type") {
	id_content_type=value;
      }
    }
  }
}
