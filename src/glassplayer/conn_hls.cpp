// conn_hls.cpp
//
// Server connector for HTTP live streams (HLS).
//
//   (C) Copyright 2014-2019 Fred Gleason <fredg@paravelsystems.com>
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

#include <tbytevector.h>

#include "codec.h"
#include "conn_hls.h"
#include "logging.h"

#ifdef CONN_HLS_DUMP_SEGMENTS
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif  // CONN_HLS_DUMP_SEGMENTS

Hls::Hls(const QString &mimetype,QObject *parent)
  : Connector(mimetype,parent)
{
#ifdef CONN_HLS_DUMP_SEGMENTS
  hls_segment_fd=-1;
#endif  // CONN_HLS_DUMP_SEGMENTS
  hls_index_process=NULL;
  hls_media_process=NULL;

  //
  // Index Processor
  //
  hls_index_playlist=new M3uPlaylist();
  hls_id3_parser=new Id3Parser();
  connect(hls_id3_parser,SIGNAL(tagReceived(uint64_t,Id3Tag *)),
	  this,SLOT(tagReceivedData(uint64_t,Id3Tag *)));
  hls_index_timer=new QTimer(this);
  hls_index_timer->setSingleShot(true);
  connect(hls_index_timer,SIGNAL(timeout()),this,SLOT(indexProcessStartData()));

  //
  // Media Processor
  //
  hls_download_average=new MeterAverage(3);

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


void Hls::connectToHostConnector()
{
  hls_index_url=serverUrl();
  hls_index_timer->start(0);
}


void Hls::disconnectFromHostConnector()
{
  StopProcess(hls_media_process);
  StopProcess(hls_index_process);
}


void Hls::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
  if(is_first) {
    hdrs->push_back("Connector|Type");
    values->push_back("HLS");

    hdrs->push_back("Connector|Server");
    values->push_back(hls_server);

    hdrs->push_back("Connector|ContentType");
    values->push_back(hls_content_type);
  }

  hdrs->push_back("Connector|Download Speed");
  values->push_back(QString().sprintf("%7.0f kbit/sec",
			 hls_download_average->average()/1000.0).trimmed());

  hdrs->push_back("Connector|HLS Version");
  values->push_back(QString().sprintf("%d",hls_index_playlist->version()));

  hdrs->push_back("Connector|HLS Target Duration");
  values->
    push_back(QString().sprintf("%d",hls_index_playlist->targetDuration()));

  hdrs->push_back("Connector|HLS Media Sequence");
  values->
    push_back(QString().sprintf("%d",hls_index_playlist->mediaSequence()));

  hdrs->push_back("Connector|HLS Segment Quantity");
  values->
    push_back(QString().sprintf("%u",hls_index_playlist->segmentQuantity()));

  for(unsigned i=0;i<hls_index_playlist->segmentQuantity();i++) {
    if(!hls_index_playlist->segmentTitle(i).isEmpty()) {
      hdrs->push_back(QString().sprintf("Connector|HLS Segment%u Title",i+1));
      values->push_back(hls_index_playlist->segmentTitle(i));
    }
    hdrs->push_back(QString().sprintf("Connector|HLS Segment%u Url",i+1));
    values->push_back(hls_index_playlist->segmentUrl(i).toString());

    hdrs->push_back(QString().sprintf("Connector|HLS Segment%u Duration",i+1));;
    values->push_back(QString().
		      sprintf("%8.5lf",hls_index_playlist->segmentDuration(i)));

    if(hls_index_playlist->segmentDateTime(i).isValid()) {
      hdrs->push_back(QString().sprintf("Connector|HLS Segment%u DateTime",i+1));
      values->push_back(hls_index_playlist->segmentDateTime(i).
			toString("yyyy-mm-dd hh::mm:ss"));
    }
  }
}


void Hls::tagReceivedData(uint64_t bytes,Id3Tag *tag)
{
  TagLib::ID3v2::FrameList frames=tag->frameList();
  bool initialize=false;

  if(hls_meta_event.isEmpty()) {
    initialize=true;
  }
  hls_meta_event.clear();
  for(unsigned i=0;i<frames.size();i++) {
    TagLib::ByteVector raw_bytes=frames[i]->frameID();
    QString id(QByteArray(raw_bytes.data(),raw_bytes.size()).constData());
    QString str=QString::fromUtf8(frames[i]->toString().toCString(true));
    hls_meta_event.setField(id,str);
    if(initialize) {
      setMetadataField(0,id,str);
    }
    /*
    printf("frame[%u]: %s|%s\n",i,
	   (const char *)id.toUtf8(),
	   frames[i]->toString().toCString(true));
    */
  }
  emit metadataReceived(bytes,&hls_meta_event);
}


void Hls::indexProcessStartData()
{
  QStringList args;

  args.push_back("--user-agent");
  args.push_back(GLASSPLAYER_USER_AGENT);
  args.push_back("-D");
  args.push_back("-");
  if((!serverUsername().isEmpty())||(!serverPassword().isEmpty())) {
    args.push_back("--user");
    args.push_back(serverUsername()+":"+serverPassword());
  }
  args.push_back(hls_index_url.toString());
  if(hls_index_process!=NULL) {
    delete hls_index_process;
  }
  hls_index_process=new QProcess(this);
  connect(hls_index_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(indexProcessFinishedData(int,QProcess::ExitStatus)));
  hls_index_process->start("curl",args);
}


void Hls::indexProcessFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status==QProcess::NormalExit) {
    if(exit_code!=0) {
      Log(LOG_WARNING,tr("index process returned non-zero exit code")+
	  QString().sprintf(" [%d]",exit_code));
    }
    else {
      QByteArray data=hls_index_process->readAllStandardOutput();
      data=ReadHeaders(data);
      M3uPlaylist *playlist=new M3uPlaylist();
      if(playlist->parse(data,hls_index_url)) {
	if(playlist->isMaster()) {  // Recurse to target playlist
	  hls_index_url=playlist->target();
	  hls_index_timer->start(0);
	}
	else {
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
	}
      }
      else {
	Log(LOG_WARNING,"error parsing playlist");
      }
      delete playlist;
    }
  }
}


void Hls::mediaProcessStartData()
{
  if(hls_media_process!=NULL) {
    if(hls_media_process->state()!=QProcess::NotRunning) {
      hls_media_timer->start(500);
      return;
    }
  }

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

#ifdef CONN_HLS_DUMP_SEGMENTS
    QString filename=
      CONN_HLS_DUMP_SEGMENTS+"/"+
      hls_current_media_segment.toString().split("/",QString::SkipEmptyParts).back();
    hls_segment_fd=open(filename.toUtf8(),O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR);
    if(hls_segment_fd>=0) {
      fprintf(stderr,
	      "creating media segment: %s\n",(const char *)filename.toUtf8()); 
    }
    else {
      fprintf(stderr,
        "unable to open media segment: %s [%s]\n",
        (const char *)filename.toUtf8(),strerror(errno)); 
    }
 #endif  // CONN_HLS_DUMP_SEGMENTS

    QStringList args;
    args.push_back("--user-agent");
    args.push_back(GLASSPLAYER_USER_AGENT);
    if((!serverUsername().isEmpty())||(!serverPassword().isEmpty())) {
      args.push_back("--user");
      args.push_back(serverUsername()+":"+serverPassword());
    }
    args.push_back(hls_current_media_segment.toString());
    if(hls_media_process!=NULL) {
      delete hls_media_process;
    }
    hls_media_process=new QProcess(this);
    hls_media_process->setReadChannel(QProcess::StandardOutput);
    connect(hls_media_process,SIGNAL(readyReadStandardOutput()),
	    this,SLOT(mediaReadyReadData()));
    connect(hls_media_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	    this,SLOT(mediaProcessFinishedData(int,QProcess::ExitStatus)));
    hls_media_process->start("curl",args);
    hls_download_start_datetime=QDateTime::currentDateTime();
  }
}


void Hls::mediaReadyReadData()
{
  QByteArray data;

  while(hls_media_process->bytesAvailable()>0) {
    data=hls_media_process->read(1024);
#ifdef CONN_HLS_DUMP_SEGMENTS
    if(hls_segment_fd>=0) {
      if(write(hls_segment_fd,data.data(),data.size())<0) {
	fprintf(stderr,"  ERROR writing media segment data: %s\n",strerror(errno));
      }
    }
#endif  // CONN_HLS_DUMP_SEGMENTS
    hls_media_segment_data+=data;
  }
}


void Hls::mediaProcessFinishedData(int exit_code,QProcess::ExitStatus status)
{
#ifdef CONN_HLS_DUMP_SEGMENTS
  if(hls_segment_fd>=0) {
    close(hls_segment_fd);
    hls_segment_fd=-1;
  }
#endif  // CONN_HLS_DUMP_SEGMENTS

  //
  // Calculate Download Speed
  //
  float msecs=hls_download_start_datetime.msecsTo(QDateTime::currentDateTime());
  float bytes_per_sec=
    (float)hls_media_segment_data.size()/(msecs/1000.0);
  hls_download_average->addValue(bytes_per_sec);

  //
  // Extract Timed Metadata
  //
  hls_id3_parser->parse(hls_media_segment_data);

  //
  // Forward Data
  //
  while(hls_media_segment_data.size()>=1024) {
    emit dataReceived(hls_media_segment_data.left(1024),false);
    hls_media_segment_data.remove(0,1024);
  }

  //  hls_id3_parser->reset();
  if(status==QProcess::NormalExit) {
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
	  Log(LOG_ERR,tr("unsupported codec")+" ["+f0[f0.size()-1]+"]");
	  exit(GLASS_EXIT_UNSUPPORTED_CODEC_ERROR);
	}
      }
      hls_last_media_segment=hls_current_media_segment;
      hls_current_media_segment="";
      hls_media_timer->start(0);
    }
  }
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


void Hls::StopProcess(QProcess *proc)
{
  if((proc!=NULL)&&(proc->state()!=QProcess::NotRunning)) {
    proc->disconnect();
    proc->kill();
    proc->waitForFinished();
  }
}
