// glassplayer.cpp
//
// glassplayer(1) Audio Encoder
//
//   (C) Copyright 2014-2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <signal.h>

#include <QCoreApplication>

#include "audiodevicefactory.h"
#include "cmdswitch.h"
#include "codecfactory.h"
#include "connectorfactory.h"
#include "glasslimits.h"
#include "glassplayer.h"
#include "logging.h"

//
// Globals
//
bool global_exiting=false;

void SigHandler(int signo)
{
  switch(signo) {
  case SIGINT:
  case SIGTERM:
    global_exiting=true;
    break;
  }
}


MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  sir_connector=NULL;
  sir_codec=NULL;
  sir_ring=NULL;
  sir_audio_device=NULL;
  sir_meter_data=false;
  sir_server_id=NULL;
  sir_first_stats=true;
  bool ok=false;

  audio_device_type=DEFAULT_AUDIO_DEVICE;
  dump_bitstream=false;
  list_codecs=false;
  list_devices=false;
  pregap=0;
  sir_stats_out=false;
  sir_metadata_out=false;
  sir_json=false;
  server_type=Connector::XCastServer;
  dump_headers=false;

  CmdSwitch *cmd=new CmdSwitch("glassplayer",GLASSPLAYER_USAGE);
  if(getenv("HOME")!=NULL) {
    cmd->addOverlay(getenv("HOME")+QString("/.glassplayerrc"));
  }
  cmd->addOverlay("/etc/glassplayer.conf");
  if(cmd->keys()==0) {
    fprintf(stderr,"glassplayer: no stream-URL specified\n");
    exit(GLASS_EXIT_ARGUMENT_ERROR);
  }
  for(unsigned i=0;i<(cmd->keys()-1);i++) {
    if(cmd->key(i)=="--audio-device") {
      for(int j=0;j<AudioDevice::LastType;j++) {
	if(cmd->value(i).toLower()==
	   AudioDevice::optionKeyword((AudioDevice::Type)j)) {
	  audio_device_type=(AudioDevice::Type)j;
	  cmd->setProcessed(i,true);
	}
      }
    }
    if(cmd->key(i)=="--dump-bitstream") {
      dump_bitstream=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-headers") {
      dump_headers=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--json") {
      sir_json=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--list-codecs") {
      list_codecs=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--list-devices") {
      list_devices=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--metadata-out") {
      sir_metadata_out=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--meter-data") {
      sir_meter_data=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--post-data") {
      post_data=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--pregap") {
      pregap=cmd->value(i).toUInt(&ok);
      if(!ok) {
	fprintf(stderr,"glassplayer: invalid argument to --pregap\n");
	exit(GLASS_EXIT_ARGUMENT_ERROR);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--server-script-down") {
      sir_server_script_down=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--server-script-up") {
      sir_server_script_up=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--stats-out") {
      sir_stats_out=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--user") {
      QStringList f0=cmd->value(i).split(":");
      sir_user=f0.at(0);
      if(f0.size()>1) {
	sir_password=f0.at(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--verbose") {
      global_log_verbose=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      device_keys.push_back(cmd->key(i));
      device_values.push_back(cmd->value(i));
    }
  }
  if(cmd->key(cmd->keys()-1)=="--list-codecs") {
    list_codecs=true;
  }
  else {
    if(cmd->key(cmd->keys()-1)=="--list-devices") {
      list_devices=true;
    }
    else {
      server_url.setUrl(cmd->key(cmd->keys()-1));
      if(!server_url.isValid()) {
	Log(LOG_ERR,"invalid stream URL");
	exit(GLASS_EXIT_ARGUMENT_ERROR);
      }
    }
  }

  //
  // Resource Enumerations
  //
  if(list_codecs) {
    ListCodecs();
    exit(0);
  }
  if(list_devices) {
    ListDevices();
    exit(0);
  }

  //
  // JSON Generator
  //
  sir_json_engine=new JsonEngine();

  //
  // Starvation Watchdog
  //
  sir_starvation_timer=new QTimer(this);
  connect(sir_starvation_timer,SIGNAL(timeout()),this,SLOT(starvationData()));

  //
  // Sanity Checks
  //
  int stdout_count=0;
  if(sir_stats_out) {
    stdout_count++;
  }
  if(dump_bitstream) {
    stdout_count++;
  }
  if(audio_device_type==AudioDevice::Stdout) {
    stdout_count++;
  }
  if(stdout_count>1) {
    Log(LOG_ERR,"only one option using STDOUT may be specified at a time");
    exit(GLASS_EXIT_ARGUMENT_ERROR);
  }

  sir_stats_timer=new QTimer(this);
  connect(sir_stats_timer,SIGNAL(timeout()),this,SLOT(statsData()));

  //
  // Set Signals
  //
  QTimer *timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(exitData()));
  timer->start(200);
  ::signal(SIGINT,SigHandler);
  ::signal(SIGTERM,SigHandler);

  //
  // Attempt to auto-detect remote server type
  //
  sir_server_id=new ServerId(this);
  connect(sir_server_id,
	  SIGNAL(typeFound(Connector::ServerType,const QString &,const QUrl &)),
	  this,
	  SLOT(serverTypeFoundData(Connector::ServerType,const QString &,
				   const QUrl &)));
  sir_server_id->connectToServer(server_url,post_data,sir_user,sir_password);
}


void MainObject::serverTypeFoundData(Connector::ServerType type,
				     const QString &mimetype,const QUrl &url)
{
  //  printf("serverTypeFound(%d,%s,%s)\n",type,
  //  	 mimetype.toUtf8().constData(),url.toString().toUtf8().constData());
  sir_connector=ConnectorFactory(type,mimetype,this);
  sir_connector->setStreamMetadataEnabled(sir_metadata_out);
  connect(sir_connector,SIGNAL(connected(bool)),
	  this,SLOT(serverConnectedData(bool)));
  sir_connector->setServerUrl(url);
  sir_connector->setServerUsername(sir_user);
  sir_connector->setServerPassword(sir_password);
  sir_connector->setPublicUrl(server_url);
  sir_connector->setPostData(post_data);
  sir_connector->setDumpHeaders(dump_headers);
  sir_connector->connectToServer();
}


void MainObject::serverConnectedData(bool state)
{
  if(state) {
    if(dump_bitstream) {  // Dump raw bitstream to standard output
       sir_codec=
	CodecFactory(Codec::TypeNull,sir_connector->audioBitrate(),this);
    }
    else {   // Normal codec initialization here
      if(sir_connector->codecType()==Codec::TypeNull) {
	Log(LOG_ERR,tr("unsupported codec")+
	    " ["+sir_connector->contentType()+"]");
	exit(GLASS_EXIT_UNSUPPORTED_CODEC_ERROR);
      }
      sir_codec=CodecFactory(sir_connector->codecType(),
			     sir_connector->audioBitrate(),this);
    }
    if((sir_codec==NULL)||(!sir_codec->isAvailable())) {
      Log(LOG_ERR,tr("codec unavailable")+
	  " ["+Codec::typeText(sir_connector->codecType())+"]");
      exit(GLASS_EXIT_UNSUPPORTED_CODEC_ERROR);
    }
    sir_codec->setChannels(sir_connector->audioChannels());
    sir_codec->setSamplerate(sir_connector->audioSamplerate());
    if(global_log_verbose) {
      Log(LOG_INFO,"Streaming from "+
	  Connector::serverTypeText(sir_connector->serverType())+" server");
    }
    connect(sir_connector,SIGNAL(dataReceived(const QByteArray &,bool)),
    	    sir_codec,SLOT(processBitstream(const QByteArray &,bool)));
    connect(sir_connector,SIGNAL(metadataReceived(uint64_t,MetaEvent *)),
	    sir_codec,SLOT(processMetadata(uint64_t,MetaEvent *)));
    connect(sir_codec,SIGNAL(framed(unsigned,unsigned,unsigned,Ringbuffer *)),
	    this,
	    SLOT(codecFramedData(unsigned,unsigned,unsigned,Ringbuffer *)));
    if(!sir_server_script_up.isEmpty()) {
      RunScript(sir_server_script_up);
    }
  }
  else {
    sir_starvation_timer->stop();
    sir_meter_timer->stop();
    if(sir_audio_device!=NULL) {
      delete sir_audio_device;
      sir_audio_device=NULL;
    }
    if(sir_codec!=NULL) {
      delete sir_codec;
      sir_codec=NULL;
    }
    if(!sir_server_script_down.isEmpty()) {
      RunScript(sir_server_script_down);
    }
  }
}


void MainObject::codecFramedData(unsigned chans,unsigned samprate,
				 unsigned bitrate,Ringbuffer *ring)
{
  QString err;

  if(global_log_verbose) {
    if(bitrate==0) {
      Log(LOG_INFO,"Using "+Codec::typeText(sir_codec->type())+
	  QString().sprintf(" decoder, %u channels, %u samples/sec",
			    chans,samprate));
    }
    else {
      Log(LOG_INFO,"Using "+Codec::typeText(sir_codec->type())+
	  QString().sprintf(" decoder, %u channels, %u samples/sec, %u kbps",
			    chans,samprate,bitrate));
    }
  }
  if((sir_audio_device=
      AudioDeviceFactory(audio_device_type,pregap,sir_codec,this))==NULL) {
    Log(LOG_ERR,"unsupported audio device");
    exit(GLASS_EXIT_UNSUPPORTED_DEVICE_ERROR);
  }
  if(!sir_audio_device->processOptions(&err,device_keys,device_values)) {
    Log(LOG_ERR,err);
    exit(GLASS_EXIT_ARGUMENT_ERROR);
  }
  connect(sir_codec,SIGNAL(metadataReceived(uint64_t,MetaEvent *)),
	  sir_audio_device,SLOT(processMetadata(uint64_t,MetaEvent *)));
  connect(sir_audio_device,SIGNAL(metadataReceived(MetaEvent *)),
	  this,SLOT(metadataReceivedData(MetaEvent *)));
  if(!sir_audio_device->start(&err)) {
    Log(LOG_ERR,err);
    exit(GLASS_EXIT_GENERAL_DEVICE_ERROR);
  }
  sir_connector->startMetadata();
  if(sir_stats_out) {
    sir_stats_timer->start(2000);
  }
  sir_meter_timer=new QTimer(this);
  connect(sir_meter_timer,SIGNAL(timeout()),this,SLOT(meterData()));
  if(sir_meter_data) {
    sir_meter_timer->start(AUDIO_METER_INTERVAL);
  }
  sir_starvation_timer->start(2000);
}


void MainObject::metadataReceivedData(MetaEvent *e)
{
  QStringList hdrs;
  QStringList values;

  sir_meta_event=*e;

  if(global_log_verbose) {
    QStringList keys=e->fieldKeys();
    for(int i=0;i<keys.size();i++) {
      Log(LOG_INFO,keys.at(i)+": "+e->field(keys.at(i)));
    }
  }
  if(sir_metadata_out) {
    if(sir_json) {
      sir_json_engine->addEvents(e->exportFields());
      printf("%s\n",(const char *)sir_json_engine->generate().toUtf8());
      sir_json_engine->clear();
    }
    else {
      QString data=e->exportFields();
      data.replace("\r","\\r");
      data.replace("\n","\\n");
      printf("%s\n",(const char *)e->exportFields().toUtf8());
    }
    fflush(stdout);
  }
}


void MainObject::starvationData()
{
  if(sir_codec!=NULL) {
    if(sir_codec->ring()!=NULL) {
      if(sir_codec->ring()->isReset()) {
	if(sir_codec->ring()->isFinished()) {
	  exit(0);
	}
	else {
	  sir_connector->reset();
	  Log(LOG_WARNING,"stream data starvation detected, connection reset");
	}
      }
    }
  }
}


void MainObject::statsData()
{
  QStringList hdrs;
  QStringList values;

  if(sir_connector!=NULL) {
    sir_connector->getStats(&hdrs,&values,sir_first_stats);
  }
  if(sir_codec!=NULL) {
    sir_codec->getStats(&hdrs,&values,sir_first_stats);
  }
  if(sir_audio_device!=NULL) {
    sir_audio_device->getStats(&hdrs,&values,sir_first_stats);
  }
  if(sir_json) {
    for(int i=0;i<hdrs.size();i++) {
      sir_json_engine->addEvents(hdrs.at(i)+": "+values.at(i));
    }
    printf("%s\n",(const char *)sir_json_engine->generate().toUtf8());
    sir_json_engine->clear();
  }
  else {
    for(int i=0;i<hdrs.size();i++) {
      printf("%s: %s\n",(const char *)hdrs[i].toUtf8(),
	     (const char *)values[i].toUtf8());
    }
    printf("\n");
  }
  fflush(stdout);
  sir_first_stats=false;
}


void MainObject::meterData()
{
  int lvls[MAX_AUDIO_CHANNELS];
  QString hex;

  sir_audio_device->meterLevels(lvls);
  switch(sir_codec->channels()) {
  case 1:
    hex=QString().sprintf("ME %04X%04X",0xFFFF&lvls[0],0xFFFF&lvls[0]);
    break;

  case 2:
    hex=QString().sprintf("ME %04X%04X",0xFFFF&lvls[0],0xFFFF&lvls[1]);
    break;
  }
  if(sir_json) {
    sir_json_engine->addEvent("Meter|Update: "+hex);
    printf("%s\n",(const char *)sir_json_engine->generate().toUtf8());
    sir_json_engine->clear();
  }
  else {
    printf("%s\n",(const char *)hex.toUtf8());
  }
  fflush(stdout);
}


void MainObject::exitData()
{
  if(global_exiting) {
    if((sir_connector!=NULL)&&(!sir_server_script_down.isEmpty())) {
      if(sir_connector->isConnected()) {
	RunScript(sir_server_script_down);
      }
    }
    if(sir_connector!=NULL) {
      sir_connector->stop();
      delete sir_connector;
    }
    if(sir_codec!=NULL) {
      delete sir_codec;
    }
    if(sir_audio_device!=NULL) {
      sir_audio_device->stop();
      delete sir_audio_device;
    }
    exit(0);
  }
}


void MainObject::ListCodecs()
{
  Codec *codec=NULL;
  QString keyword;

  for(int i=0;i<Codec::TypeLast;i++) {
    if((codec=CodecFactory((Codec::Type)i,64,this))!=NULL) {
      if(codec->isAvailable()) {
	keyword=Codec::optionKeyword((Codec::Type)i);
	if(!keyword.isEmpty()) {
	  printf("%s\n",
		 (const char *)Codec::optionKeyword((Codec::Type)i).toUtf8());
	}
      }
    }
  }
}


void MainObject::ListDevices()
{
  for(int i=0;i<AudioDevice::LastType;i++) {
    if(AudioDeviceFactory((AudioDevice::Type)i,0,NULL,this)!=NULL) {
      printf("%s\n",(const char *)AudioDevice::optionKeyword((AudioDevice::Type)i).toUtf8());
    }
  }
}


void MainObject::RunScript(const QString &cmd)
{
  QStringList args;
  QString prog;
  QProcess *proc;

  args=cmd.split(" ");
  prog=args[0];
  args.erase(args.begin());
  proc=new QProcess(this);
  proc->start(prog,args);
  proc->waitForFinished();
  delete proc;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
