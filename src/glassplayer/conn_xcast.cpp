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

#include <QStringList>

#include "conn_xcast.h"
#include "logging.h"

XCast::XCast(QObject *parent)
  : Connector(parent)
{
  xcast_header_active=false;
  xcast_header="";

  xcast_socket=new QTcpSocket(this);
  connect(xcast_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(xcast_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(xcast_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
}


XCast::~XCast()
{
  delete xcast_socket;
}


Connector::ServerType XCast::serverType() const
{
  return Connector::XCastServer;
}


void XCast::connectToHostConnector(const QString &hostname,uint16_t port)
{
  xcast_socket->connectToHost(hostname,port);
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
  SendHeader("GET "+serverMountpoint()+" HTTP/1.1");
  SendHeader("Host: "+hostHostname()+":"+QString().sprintf("%u",hostPort()));
  SendHeader("icy-metadata: 1");
  SendHeader("Accept: */*");
  SendHeader("User-Agent: glassplayer/"+QString(VERSION));
  SendHeader("Cache-control: no-cache");
  SendHeader("Connection: close");
  SendHeader("");
}


void XCast::readyReadData()
{
  char data[1501];
  int n;

  while((n=xcast_socket->read(data,1500))>0) {
    data[n]=0;
    if(xcast_header_active) {   // Get headers
      for(int i=0;i<n;i++) {
	switch(0xFF&data[i]) {
	case 13:
	  if(!xcast_header.isEmpty()) {
	    ProcessHeader(xcast_header);
	  }
	  break;

	case 10:
	  if(xcast_header.isEmpty()) {
	    xcast_header_active=false;
	    return;
	  }
	  xcast_header="";
	  break;

	default:
	  xcast_header+=data[i];
	  break;
	}
      }
    }
    else {   // Scan for metadata updates
      for(int i=0;i<n;i++) {
	switch(xcast_metadata_istate) {
	case 0:
	  if(data[i]=='S') {
	    xcast_metadata_istate=1;
	  }
	  break;

	case 1:
	  if(data[i]=='t') {
	    xcast_metadata_istate=2;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 2:
	  if(data[i]=='r') {
	    xcast_metadata_istate=3;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 3:
	  if(data[i]=='e') {
	    xcast_metadata_istate=4;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 4:
	  if(data[i]=='a') {
	    xcast_metadata_istate=5;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 5:
	  if(data[i]=='m') {
	    xcast_metadata_istate=6;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 6:
	  if(data[i]=='T') {
	    xcast_metadata_istate=7;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 7:
	  if(data[i]=='i') {
	    xcast_metadata_istate=8;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 8:
	  if(data[i]=='t') {
	    xcast_metadata_istate=9;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 9:
	  if(data[i]=='l') {
	    xcast_metadata_istate=10;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 10:
	  if(data[i]=='e') {
	    xcast_metadata_istate=11;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 11:
	  if(data[i]=='=') {
	    xcast_metadata_istate=12;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 12:
	  if(data[i]=='\'') {
	    xcast_metadata_istate=13;
	  }
	  else {
	    xcast_metadata_istate=0;
	  }
	  break;

	case 13:
	  if((data[i]==';')&&(xcast_metadata_string.right(1)=="'")) {
	    setStreamNowPlaying(xcast_metadata_string.
				left(xcast_metadata_string.length()-1));
	    xcast_metadata_string="";
	    xcast_metadata_istate=0;
	  }
	  else {
	    xcast_metadata_string+=data[i];
	  }
	  break;
	}
      }
      emit dataReceived(data,n);
    }
  }
}


void XCast::errorData(QAbstractSocket::SocketError err)
{
}


void XCast::SendHeader(const QString &str)
{
  xcast_socket->write((str+"\r\n").toUtf8(),str.length()+2);
}


void XCast::ProcessHeader(const QString &str)
{
  printf("%s\n",(const char *)str.toUtf8());

  QStringList f0;

  if(xcast_result_code==0) {
    f0=str.split(" ",QString::SkipEmptyParts);
    if(f0.size()!=3) {
      Log(LOG_ERR,"malformed response from server ["+str+"]");
      exit(256);
    }
    xcast_result_code=f0[1].toInt();
    if((xcast_result_code<200)||(xcast_result_code>=300)) {
      Log(LOG_ERR,"server returned error ["+f0[1]+" "+f0[2]+"]");
      exit(256);
    }
  }
  else {
    f0=str.split(":");
    if(f0.size()==2) {
      QString hdr=f0[0].trimmed().toLower();
      QString value=f0[1].trimmed();
      if(hdr=="content-type") {
	setFormatIdentifier(value);
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
