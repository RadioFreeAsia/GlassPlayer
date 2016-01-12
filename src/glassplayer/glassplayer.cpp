// glassplayer.cpp
//
// glassplayer(1) Audio Encoder
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

#include <QCoreApplication>

#include "audiodevicefactory.h"
#include "cmdswitch.h"
#include "codecfactory.h"
#include "connectorfactory.h"
#include "glasslimits.h"
#include "glassplayer.h"
#include "logging.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  sir_codec=NULL;
  sir_ring=NULL;

  audio_device_type=AudioDevice::Stdout;
  dump_bitstream=false;
  server_type=Connector::XCastServer;

  CmdSwitch *cmd=
    new CmdSwitch(qApp->argc(),qApp->argv(),"glassplayer",GLASSPLAYER_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
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
    if(cmd->key(i)=="--server-type") {
      for(int j=0;j<Connector::LastServer;j++) {
	if(Connector::optionKeyword((Connector::ServerType)j)==
	   cmd->value(i).toLower()) {
	  server_type=(Connector::ServerType)j;
	  cmd->setProcessed(i,true);
	}
      }
      if(!cmd->processed(i)) {
	Log(LOG_ERR,
	    QString().sprintf("unknown --server-type value \"%s\"",
			      (const char *)cmd->value(i).toAscii()));
	exit(256);
      }
    }
    if(cmd->key(i)=="--server-url") {
      server_url.setUrl(cmd->value(i));
      if(!server_url.isValid()) {
	Log(LOG_ERR,"invalid argument for --server-url");
	exit(256);
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

  //
  // Sanity Checks
  //
  if(server_url.host().isEmpty()) {
    Log(LOG_ERR,"you must specify a --server-url\n");
    exit(256);
  }
  if((device_keys.size()!=0)||(device_values.size()!=0)) {
    Log(LOG_ERR,"unknown option\n");
    exit(256);
  }

  StartServerConnection();
}


void MainObject::serverConnectedData(bool state)
{
  if(state) {
    if(dump_bitstream) {  // Dump raw bitstream to standard output
      sir_codec=
	CodecFactory(Codec::TypeNull,sir_connector->audioBitrate(),this);
    }
    else {   // Normal codec initialization here
      for(int i=1;i<Codec::TypeLast;i++) {
	if(Codec::acceptsContentType((Codec::Type)i,
				     sir_connector->contentType())) {
	  sir_codec=
	    CodecFactory((Codec::Type)i,sir_connector->audioBitrate(),this);
	}
      }
    }
    if((sir_codec==NULL)||(!sir_codec->isAvailable())) {
      Log(LOG_ERR,"unsupported codec ["+sir_connector->contentType()+"]");
      exit(256);
    }
    if(global_log_verbose) {
      Log(LOG_INFO,"Streaming from "+
	  Connector::serverTypeText(sir_connector->serverType())+" server");
    }
    connect(sir_connector,SIGNAL(dataReceived(const QByteArray &)),
	    sir_codec,SLOT(process(const QByteArray &)));
    connect(sir_codec,SIGNAL(framed(unsigned,unsigned,unsigned,Ringbuffer *)),
	    this,
	    SLOT(codecFramedData(unsigned,unsigned,unsigned,Ringbuffer *)));
  }
}


void MainObject::codecFramedData(unsigned chans,unsigned samprate,
				 unsigned bitrate,Ringbuffer *ring)
{
  if(global_log_verbose) {
    Log(LOG_INFO,"Using "+Codec::typeText(sir_codec->type())+
	QString().sprintf(" decoder, %u channels, %u samples/sec, %u kbps\n",
			  chans,samprate,bitrate));
  }
  if((sir_audio_device=
      AudioDeviceFactory(audio_device_type,sir_codec,this))==NULL) {
    Log(LOG_ERR,"unsupported audio device");
    exit(256);
  }
}


void MainObject::streamMetadataChangedData(const QString &str)
{
  if(global_log_verbose) {
    Log(LOG_INFO,"Stream Now Playing: "+str);
  }
}


void MainObject::StartServerConnection()
{
  uint16_t port=server_url.port();
  if(port==65535) {
    port=DEFAULT_SERVER_PORT;
  }
  sir_connector=ConnectorFactory(server_type,this);
  connect(sir_connector,SIGNAL(connected(bool)),
	  this,SLOT(serverConnectedData(bool)));
  connect(sir_connector,SIGNAL(streamMetadataChanged(const QString &)),
	  this,SLOT(streamMetadataChangedData(const QString &)));
  if(server_url.path().isEmpty()) {
    sir_connector->setServerMountpoint("/");
  }
  else {
    sir_connector->setServerMountpoint(server_url.path());
  }
  sir_connector->connectToServer(server_url.host(),port);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
