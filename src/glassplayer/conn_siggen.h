// conn_siggen.h
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

#ifndef CONN_SIGGEN_H
#define CONN_SIGGEN_H

#include <sndfile.h>
#include <stdint.h>

#include <QTcpSocket>
#include <QTimer>

#include "connector.h"

#define SIGGEN_SAMPLERATE 48000
#define SIGGEN_CHANNELS 1

class SigGen : public Connector
{
  Q_OBJECT;
 public:
  SigGen(const QString &mimetype,QObject *parent=0);
  ~SigGen();
  Connector::ServerType serverType() const;
  void reset();

 private slots:
  void passthroughData();

 protected:
  void connectToHostConnector();
  void disconnectFromHostConnector();
  void loadStats(QStringList *hdrs,QStringList *values,bool is_first);

 private:
  QTimer *siggen_write_timer;
  unsigned siggen_frequency;
  double siggen_ratio;
  QString siggen_level;
  uint64_t siggen_sample;
};


#endif  // CONN_SIGGEN_H
