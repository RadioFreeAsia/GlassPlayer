// conn_hls.cpp
//
// Server connector for HTTP live streams (HLS).
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

#include "conn_hls.h"
#include "logging.h"

Hls::Hls(QObject *parent)
  : Connector(parent)
{
  hls_index_process=NULL;

  //
  // Index Processor
  //
  hls_index_playlist=new M3uPlaylist();
  hls_index_timer=new QTimer(this);
  hls_index_timer->setSingleShot(true);
  connect(hls_index_timer,SIGNAL(timeout()),this,SLOT(indexProcessStartData()));
}


Hls::~Hls()
{
  delete hls_index_playlist;
  if(hls_index_process!=NULL) {
    delete hls_index_process;
  }
}


Connector::ServerType Hls::serverType() const
{
  return Connector::HlsServer;
}


void Hls::reset()
{
}


void Hls::connectToHostConnector(const QString &hostname,uint16_t port)
{
  hls_index_timer->start(0);
}


void Hls::disconnectFromHostConnector()
{
}


void Hls::indexProcessStartData()
{
  QStringList args;

  args.push_back(serverUrl().toString());
  if(hls_index_process!=NULL) {
    delete hls_index_process;
  }
  hls_index_process=new QProcess(this);
  connect(hls_index_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(indexProcessErrorData(QProcess::ProcessError)));
  connect(hls_index_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(indexProcessFinishedData(int,QProcess::ExitStatus)));
  hls_index_process->start("curl",args);
}


void Hls::indexProcessFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status!=QProcess::NormalExit) {
    Log(LOG_WARNING,tr("index process crashed"));
  }
  else {
    if(exit_code!=0) {
      Log(LOG_WARNING,tr("index process returned non-zero exit code")+
	  QString().sprintf(" [%d]",exit_code));
    }
    else {
      M3uPlaylist *playlist=new M3uPlaylist();
      if(playlist->parse(hls_index_process->readAllStandardOutput())) {
	if(*playlist!=*hls_index_playlist) {
	  *hls_index_playlist=*playlist;
	  fprintf(stderr,"PLAYLIST:\n%s\n",
		  (const char *)hls_index_playlist->dump().toUtf8());
	  hls_index_timer->start(1000*hls_index_playlist->targetDuration());
	}
	else {
	  hls_index_timer->start(1000);
	}
	delete playlist;
      }
      else {
	Log(LOG_WARNING,"error parsing playlist");
      }
    }
  }
}


void Hls::indexProcessErrorData(QProcess::ProcessError err)
{
  Log(LOG_WARNING,tr("index process returned error")+
      " ["+Connector::processErrorText(err)+"]");
}
