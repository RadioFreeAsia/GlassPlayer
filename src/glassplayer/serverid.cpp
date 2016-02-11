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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <QProcess>

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
  if(id_url.scheme().isEmpty()||(id_url.scheme().toLower()=="file")) {
    id_content_type=GetContentType(id_url.path());
    if(id_content_type=="text/plain") {  // Could be a playlist
      M3uPlaylist *playlist=new M3uPlaylist();
      if(playlist->parseFile(id_url)) {
	if(playlist->segmentQuantity()>0) {
	  if(playlist->segmentUrl(0).isLocalFile()) {
	    emit typeFound(Connector::FileServer,"",playlist->segmentUrl(0));
	  }
	  else {
	    emit typeFound(Connector::XCastServer,"",playlist->segmentUrl(0));
	  }
	  delete playlist;
	  return;
	}
      }
      delete playlist;
      Log(LOG_ERR,tr("unsupported playlist format")+" ["+id_content_type+"]");
      exit(GLASS_EXIT_UNSUPPORTED_PLAYLIST_ERROR);
    }
    else {
      emit typeFound(Connector::FileServer,id_content_type,id_url);
    }
  }
  else {
    id_socket->connectToHost(url.host(),url.port(80));
  }
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
  id_icy=false;
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
      //
      // M3U Playlist
      //
      if((id_content_type.toLower()=="audio/x-mpegurl")||
	 (id_content_type.toLower()=="application/vnd.apple.mpegurl")||
	 (id_content_type.toLower()=="application/x-mpegurl")) {
	M3uPlaylist *playlist=new M3uPlaylist();
	if(playlist->parse(id_body.constData(),id_url)) {
	  if(playlist->isExtended()) {
	    emit typeFound(Connector::HlsServer,"",id_url);
	  }
	  else {
	    //
	    // XCast Server
	    //
	    if(playlist->segmentQuantity()>0) {
	      if(global_log_verbose) {
		Log(LOG_INFO,tr("using mountpoint")+
		    ": "+playlist->segmentUrl(0).toString());
	      }
	      emit typeFound(Connector::XCastServer,"",playlist->segmentUrl(0));
	    }
	    else {
	      Log(LOG_ERR,"playlist contains no media segments");
	      exit(GLASS_EXIT_INVALID_PLAYLIST_ERROR);
	    }
	  }
	  delete playlist;
	  id_kill_timer->start(0);
	  return;
	}
	Log(LOG_ERR,"invalid M3U list format");
	exit(GLASS_EXIT_INVALID_PLAYLIST_ERROR);
      }

      //
      // Static File
      //
      int fd=-1;
      char tempfile[]={"/tmp/glassplayerXXXXXX"};
      if((fd=mkstemp(tempfile))<0) {
	Log(LOG_ERR,tr("unable to create temporary file")+
	    " ["+strerror(errno)+"]");
	exit(GLASS_EXIT_FILEOPEN_ERROR);
      }
      write(fd,id_body.constData(),id_body.size());
      close(fd);
      emit typeFound(Connector::FileServer,id_content_type,
		     QUrl(QString("file://")+tempfile));
      id_kill_timer->start(0);
    }
    break;

  default:
    Log(LOG_ERR,Connector::socketErrorText(err));
    exit(GLASS_EXIT_NETWORK_ERROR);
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
    if(id_icy) {
      emit typeFound(Connector::XCastServer,id_content_type,id_url);
      id_kill_timer->start(0);
      return;
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
      exit(GLASS_EXIT_SERVER_ERROR);
    }
    break;

  default:
    Log(LOG_ERR,"server returned error ["+id_result_text+"]");
    exit(GLASS_EXIT_HTTP_ERROR);
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
      exit(GLASS_EXIT_SERVER_ERROR);
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
      id_icy=id_icy||(hdr.split("-")[0].toLower()=="icy");
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


QString ServerId::GetContentType(const QString &filename)
{
  QStringList args;
  QString ret;

  args.push_back("-b");
  args.push_back("--mime-type");
  args.push_back(filename);
  QProcess *proc=new QProcess(this);
  proc->start("file",args);
  proc->waitForFinished();
  ret=proc->readAllStandardOutput().trimmed();
  delete proc;

  return ret;
}
