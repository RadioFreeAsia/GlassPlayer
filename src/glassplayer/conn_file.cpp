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

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QByteArray>
#include <QProcess>
#include <QStringList>

#include "conn_file.h"
#include "logging.h"

File::File(const QString &mimetype,QObject *parent)
  : Connector(mimetype,parent)
{
  file_fd=-1;
  file_sf=NULL;

  file_write_timer=new QTimer(this);
  file_write_timer->setSingleShot(true);
}


File::~File()
{
  delete file_write_timer;
}


Connector::ServerType File::serverType() const
{
  return Connector::FileServer;
}


void File::reset()
{
}


void File::passthroughData()
{
  float pcm[2048];
  sf_count_t n;
  unsigned frames=2048/audioChannels();

  if((n=sf_readf_float(file_sf,pcm,frames))>=0) {
    emit dataReceived(QByteArray((char *)pcm,
				 n*audioChannels()*sizeof(float)),n!=(frames));
  }
  if(n==frames) {
    file_write_timer->start(0);
  }
  else {
    sf_close(file_sf);
    if(serverUrl().path()!=publicUrl().path()) {
      unlink(serverUrl().path().toUtf8());
    }
  }
}


void File::writeData()
{
  char data[1024];
  int n;

  if((n=read(file_fd,data,1024))>=0) {
    emit dataReceived(QByteArray(data,n),n!=1024);
  }
  if(n==1024) {
    file_write_timer->start(0);
  }
  else {
    close(file_fd);
    if(serverUrl().path()!=publicUrl().path()) {
      unlink(serverUrl().path().toUtf8());
    }
  }
}


void File::connectToHostConnector()
{
  if(codecType()==Codec::TypePassthrough) {
    memset(&file_sfinfo,0,sizeof(file_sfinfo));
    if((file_sf=sf_open(serverUrl().path().toUtf8(),
			SFM_READ,&file_sfinfo))==NULL) {
      Log(LOG_ERR,tr("unable to open downloaded file")+
	  " \""+serverUrl().path()+"\" ["+sf_strerror(file_sf)+"]");
      exit(256);
    }
    setAudioChannels(file_sfinfo.channels);
    setAudioSamplerate(file_sfinfo.samplerate);
    connect(file_write_timer,SIGNAL(timeout()),this,SLOT(passthroughData()));
  }
  else {
    if((file_fd=open(serverUrl().path().toUtf8(),O_RDONLY))<0) {
      Log(LOG_ERR,tr("unable to open downloaded file")+
	  " \""+serverUrl().path()+"\" ["+strerror(errno)+"]");
      exit(256);
    }
    connect(file_write_timer,SIGNAL(timeout()),this,SLOT(writeData()));
  }
  setConnected(true);
  file_write_timer->start(0);
}


void File::disconnectFromHostConnector()
{
}


void File::loadStats(QStringList *hdrs,QStringList *values)
{
  hdrs->push_back("ConnectorType");
  values->push_back("File");

  hdrs->push_back("ConnectorContentType");
  values->push_back(contentType());
}
