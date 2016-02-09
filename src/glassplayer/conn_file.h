// conn_file.h
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

#ifndef CONN_FILE_H
#define CONN_FILE_H

#include <QTcpSocket>
#include <QTimer>

#include "connector.h"

class File : public Connector
{
  Q_OBJECT;
 public:
  File(const QString &mimetype,QObject *parent=0);
  ~File();
  Connector::ServerType serverType() const;
  void reset();

 private slots:
  void writeData();

 protected:
  void connectToHostConnector();
  void disconnectFromHostConnector();
  void loadStats(QStringList *hdrs,QStringList *values);

 private:
  QTimer *file_write_timer;
  int file_fd;
};


#endif  // CONN_FILE_H
