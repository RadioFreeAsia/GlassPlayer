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

#include "codec.h"
#include "conn_hls.h"
#include "logging.h"

Hls::Hls(QObject *parent)
  : Connector(parent)
{
  hls_index_process=NULL;
  hls_media_process=NULL;

  //
  // Index Processor
  //
  hls_index_playlist=new M3uPlaylist();
  hls_index_timer=new QTimer(this);
  hls_index_timer->setSingleShot(true);
  connect(hls_index_timer,SIGNAL(timeout()),this,SLOT(indexProcessStartData()));

  //
  // Media Processor
  //
  hls_media_timer=new QTimer(this);
  hls_media_timer->setSingleShot(true);
  connect(hls_media_timer,SIGNAL(timeout()),this,SLOT(mediaProcessStartData()));
}


Hls::~Hls()
{
  delete hls_media_timer;
  delete hls_index_timer;
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


void Hls::loadStats(QStringList *hdrs,QStringList *values)
{
  hdrs->push_back("ConnectorType");
  values->push_back("HLS");

  hdrs->push_back("ConnectorServer");
  values->push_back(hls_server);

  hdrs->push_back("ConnectorMimetype");
  values->push_back(hls_content_type);

  hdrs->push_back("ConnectorHLSVersion");
  values->push_back(QString().sprintf("%d",hls_index_playlist->version()));

  hdrs->push_back("ConnectorHLSTargetDuration");
  values->
    push_back(QString().sprintf("%d",hls_index_playlist->targetDuration()));

  hdrs->push_back("ConnectorHLSMediaSequence");
  values->
    push_back(QString().sprintf("%d",hls_index_playlist->mediaSequence()));

  hdrs->push_back("ConnectorHLSSegmentQuantity");
  values->
    push_back(QString().sprintf("%u",hls_index_playlist->segmentQuantity()));

  for(unsigned i=0;i<hls_index_playlist->segmentQuantity();i++) {
    if(!hls_index_playlist->segmentTitle(i).isEmpty()) {
      hdrs->push_back(QString().sprintf("ConnectorHLSSegment%uTitle",i+1));
      values->push_back(hls_index_playlist->segmentTitle(i));
    }
    hdrs->push_back(QString().sprintf("ConnectorHLSSegment%uUrl",i+1));
    values->push_back(hls_index_playlist->segmentUrl(i).toString());

    hdrs->push_back(QString().sprintf("ConnectorHLSSegment%uDuration",i+1));;
    values->push_back(QString().
		      sprintf("%8.5lf",hls_index_playlist->segmentDuration(i)));

    if(hls_index_playlist->segmentDateTime(i).isValid()) {
      hdrs->push_back(QString().sprintf("ConnectorHLSSegment%uDateTime",i+1));
      values->push_back(hls_index_playlist->segmentDateTime(i).
			toString("yyyy-mm-dd hh::mm:ss"));
    }
  }
}


void Hls::indexProcessStartData()
{
  QStringList args;

  args.push_back("-D");
  args.push_back("-");
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
      QByteArray data=hls_index_process->readAllStandardOutput();
      data=ReadHeaders(data);
      M3uPlaylist *playlist=new M3uPlaylist();
      if(playlist->parse(data,serverUrl())) {
	if(*playlist!=*hls_index_playlist) {
	  *hls_index_playlist=*playlist;
	  if(isConnected()||hls_index_playlist->segmentQuantity()>=3) {
	    hls_media_timer->start(0);
	  }
	  else {
	    if(global_log_verbose) {
	      Log(LOG_INFO,"waiting from stream to fill");
	    }
	  }
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


void Hls::mediaProcessStartData()
{
  //
  // Find next media segment
  //
  unsigned segno=0;
  for(unsigned i=0;i<hls_index_playlist->segmentQuantity();i++) {
    if(hls_index_playlist->segmentUrl(i)==hls_last_media_segment) {
      segno=i+1;
    }
  }
  if(segno<hls_index_playlist->segmentQuantity()) {
    hls_current_media_segment=hls_index_playlist->segmentUrl(segno);
    QStringList args;
    args.push_back(hls_current_media_segment.toString());
    if(hls_media_process!=NULL) {
      delete hls_media_process;
    }
    hls_new_segment=true;
    hls_media_process=new QProcess(this);
    hls_media_process->setReadChannel(QProcess::StandardOutput);
    connect(hls_media_process,SIGNAL(readyReadStandardOutput()),
	    this,SLOT(mediaReadyReadData()));
    connect(hls_media_process,SIGNAL(error(QProcess::ProcessError)),
	    this,SLOT(mediaProcessErrorData(QProcess::ProcessError)));
    connect(hls_media_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	    this,SLOT(mediaProcessFinishedData(int,QProcess::ExitStatus)));
    hls_media_process->start("curl",args);
  }
}


void Hls::mediaReadyReadData()
{
  QByteArray data;

  while(hls_media_process->bytesAvailable()>0) {
    data=hls_media_process->read(1024);
    if(hls_new_segment) {
      if((data.constData()[0]=='I')&&
	 (data.constData()[1]=='D')&&
	 (data.constData()[2]=='3')) {
	//
	// FIXME: Extract ID3 timestamp info here
	//
	data=data.right(data.length()-(10+data.constData()[9]));
      }
      hls_new_segment=false;
    }
    emit dataReceived(data);
  }
}


void Hls::mediaProcessFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status!=QProcess::NormalExit) {
    Log(LOG_WARNING,tr("media process crashed"));
  }
  else {
    if(exit_code!=0) {
      Log(LOG_WARNING,tr("media process returned non-zero exit code")+
	  QString().sprintf(" [%d]",exit_code));
    }
    else {
      if(!isConnected()) {
	QStringList f0=hls_current_media_segment.toString().split(".");
	for(int i=1;i<Codec::TypeLast;i++) {
	  if(Codec::acceptsExtension((Codec::Type)i,f0[f0.size()-1])) {
	    setCodecType((Codec::Type)i);
	    setConnected(true);
	  }
	}
	if(!isConnected()) {
	  Log(LOG_ERR,tr("no such codec")+" ["+f0[f0.size()-1]+"]");
	  exit(256);
	}
      }
      hls_last_media_segment=hls_current_media_segment;
      hls_current_media_segment="";
      hls_media_timer->start(0);
    }
  }
}


void Hls::mediaProcessErrorData(QProcess::ProcessError err)
{
  Log(LOG_WARNING,tr("media process returned error")+
      " ["+Connector::processErrorText(err)+"]");
}


QByteArray Hls::ReadHeaders(QByteArray &data)
{
  QString line;
  unsigned used=0;

  for(int i=0;i<data.size();i++) {
    switch(0xFF&data.constData()[i]) {
    case 13:
      break;

    case 10:
      if(line.isEmpty()) {
	return data.right(data.length()-used);
      }
      ProcessHeader(line);
      line="";
      break;

    default:
      line+=data.constData()[i];
      break;
    }
    used++;
  }
  return QByteArray();
}


void Hls::ProcessHeader(const QString &str)
{
  //fprintf(stderr,"HEADER: %s\n",(const char *)str.toUtf8());
  QStringList f0=str.split(":");

  if(f0.size()>=2) {
    QString hdr=f0[0].toLower().trimmed();
    f0.erase(f0.begin());
    QString value=f0.join(":").trimmed();

    if(hdr=="server") {
      hls_server=value;
    }
    if(hdr=="content-type") {
      hls_content_type=value;
    }
  }
}
