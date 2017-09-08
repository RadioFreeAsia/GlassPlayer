// conn_siggen.cpp
//
// Server connector for synthesized waveforms
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QByteArray>
#include <QProcess>
#include <QStringList>

#include "conn_siggen.h"
#include "logging.h"

SigGen::SigGen(const QString &mimetype,QObject *parent)
  : Connector(mimetype,parent)
{
  siggen_write_timer=new QTimer(this);
  siggen_write_timer->setSingleShot(true);
  siggen_sample=0;
  siggen_ratio=1.0;
}


SigGen::~SigGen()
{
  if(serverUrl().path()!=publicUrl().path()) {
    unlink(serverUrl().path().toUtf8());
  }
  delete siggen_write_timer;
}


Connector::ServerType SigGen::serverType() const
{
  return Connector::SignalGenerator;
}


void SigGen::reset()
{
}


void SigGen::passthroughData()
{
  float pcm[1024];

  for(int i=0;i<1024;i++) {
    pcm[i]=siggen_ratio*sin(6.2832*(double)(i+siggen_sample)*
			    (double)siggen_frequency/(double)SIGGEN_SAMPLERATE);
  }
  siggen_sample+=1024;
  emit dataReceived(QByteArray((char *)pcm,1024),false);
  siggen_write_timer->start(0);
}


void SigGen::connectToHostConnector()
{
  bool ok=false;

  //
  // Get parameters
  //
  siggen_frequency=serverUrl().host().toUInt(&ok);
  if(!ok) {
    fprintf(stderr,"glassplayer: invalid url\n");
    exit(GLASS_EXIT_ARGUMENT_ERROR);
  }
  if((siggen_frequency>0)&&((siggen_frequency%125)!=0)) {
    fprintf(stderr,"glassplayer: unsupported tone frequency\n");
    exit(GLASS_EXIT_ARGUMENT_ERROR);
  }
  QStringList f0=serverUrl().path().split("/",QString::KeepEmptyParts);
  if(f0.size()>1) {
    siggen_level=f0.at(1);
    siggen_ratio=exp(2.303*siggen_level.toDouble(&ok)/(20.0));
    if(!ok) {
      fprintf(stderr,"glassplayer: invalid url\n");
      exit(GLASS_EXIT_ARGUMENT_ERROR);
    }
    if(siggen_ratio>1.0) {
      fprintf(stderr,"glassplayer: invalid tone gain\n");
      exit(GLASS_EXIT_ARGUMENT_ERROR);
    }
  }

  setAudioChannels(SIGGEN_CHANNELS);
  setAudioSamplerate(SIGGEN_SAMPLERATE);
  connect(siggen_write_timer,SIGNAL(timeout()),this,SLOT(passthroughData()));
  setConnected(true);
  siggen_write_timer->start(0);
}


void SigGen::disconnectFromHostConnector()
{
}


void SigGen::loadStats(QStringList *hdrs,QStringList *values,bool is_first)
{
  if(is_first) {
    hdrs->push_back("Connector|Type");
    values->push_back("SigGen");

    hdrs->push_back("Connector|Frequency");
    values->push_back(QString().sprintf("%u",siggen_frequency));

    hdrs->push_back("Connector|Level");
    values->push_back(siggen_level);
  }
}
