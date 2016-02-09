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
#include "m3uplaylist.h"
#include "serverid.h"

ServerId::ServerId(QObject *parent)
  : QObject(parent)
{
  id_socket=CreateSocket();
  id_restarting=false;

  id_restart_timer=new QTimer(this);
  id_restart_timer->setSingleShot(true);
  connect(id_restart_timer,SIGNAL(timeout()),this,SLOT(restartData()));

  id_kill_timer=new QTimer(this);
  id_kill_timer->setSingleShot(true);
  connect(id_kill_timer,SIGNAL(timeout()),this,SLOT(killData()));
}


ServerId::~ServerId()
{
  if(id_socket!=NULL) {
    delete id_socket;
  }
  delete id_restart_timer;
  delete id_kill_timer;
}


void ServerId::connectToServer(const QUrl &url)
{
  id_url=url;
  if(id_url.path().isEmpty()) {
    id_url.setPath("/");
  }
  id_socket->connectToHost(url.host(),url.port(80));
}


void ServerId::connectedData()
{
  id_header_active=true;
  id_result_code=0;
  id_result_text="";
  id_body="";
  id_content_type="";
  id_location="";
  id_restarting=false;
  SendHeader("GET "+id_url.path()+" HTTP/1.1");
  SendHeader("Host: "+id_url.host()+":"+QString().sprintf("%u",id_url.port(80)));
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
	    ProcessResult();
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
    if(!id_restarting) {
      if((id_content_type.toLower()=="audio/x-mpegurl")||
	 (id_content_type.toLower()=="application/vnc.appl.mpegurl")||
	 (id_content_type.toLower()=="application/x-mpegurl")) {
	M3uPlaylist *playlist=new M3uPlaylist();
	if(playlist->parse(id_body.toUtf8(),id_url)) {
	  if(playlist->isExtended()) {
	    emit typeFound(Connector::HlsServer,id_url);
	  }
	  else {
	    if(playlist->segmentQuantity()>0) {
	      if(global_log_verbose) {
		Log(LOG_INFO,tr("using mountpoint")+
		    ": "+playlist->segmentUrl(0).toString());
	      }
	      emit typeFound(Connector::XCastServer,playlist->segmentUrl(0));
	    }
	    else {
	      Log(LOG_ERR,"playlist contains no media segments");
	      exit(256);
	    }
	  }
	  delete playlist;
	  id_kill_timer->start(0);
	  return;
	}
	else {
	  Log(LOG_ERR,"invalid M3U list format");
	  exit(256);
	}
	exit(0);
      }
      Log(LOG_ERR,tr("unsupported stream type")+" ["+id_content_type+"]");
      exit(256);
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


void ServerId::restartData()
{
  delete id_socket;
  id_socket=CreateSocket();
  
  id_socket->connectToHost(id_url.host(),id_url.port(80));
}


void ServerId::ProcessResult()
{
  switch(id_result_code) {
  case 100:   // Continue
  case 200:   // OK
  case 203:   // Non-Authoritative Information
    for(unsigned i=0;i<Connector::LastServer;i++) {
      if(Connector::acceptsContentType((Connector::ServerType)i,
				       id_content_type)) {
	emit typeFound((Connector::ServerType)i,id_url);
	id_kill_timer->start(0);
	return;
      }
    }
    break;

  case 301:   // Moved Permanently
  case 302:   // Found
  case 303:   // See Other
  case 307:   // Temporary Redirect
    if(!id_location.isEmpty()) {
      id_url=QUrl(id_location);
      if(id_url.path().isEmpty()) {
	id_url.setPath("/");
      }
      if(global_log_verbose) {
	Log(LOG_INFO,tr("redirecting to")+" "+id_url.toString());
      }
      id_restarting=true;
      id_restart_timer->start(0);
    }
    else {
      Log(LOG_ERR,
	  tr("server returned")+" "+id_result_text+", "+
	  tr("but redirected URI is empty."));
      exit(256);
    }
    break;

  default:
    Log(LOG_ERR,"server returned error ["+id_result_text+"]");
    exit(256);
  }
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
    f0.erase(f0.begin());
    id_result_text=f0.join(" ");
  }
  else {
    f0=str.split(":");
    if(f0.size()>=2) {
      QString hdr=f0[0].trimmed().toLower();
      f0.erase(f0.begin());
      QString value=f0.join(":").trimmed();
      if(hdr=="content-type") {
	id_content_type=value;
      }
      if(hdr=="location") {
	id_location=value;
      }
    }
  }
}


QTcpSocket *ServerId::CreateSocket()
{
  QTcpSocket *sock=new QTcpSocket(this);
  connect(sock,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(sock,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(sock,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  return sock;
}
