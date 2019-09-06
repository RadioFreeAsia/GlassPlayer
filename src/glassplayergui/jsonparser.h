// jsonparser.h
//
// JSON parser
//
//   (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QByteArray>
#include <QJsonDocument>
#include <QObject>

class JsonParser : public QObject
{
  Q_OBJECT;
 public:
  JsonParser(QObject *parent=0);
  void addData(QByteArray data);

 signals:
  void newDocument(const QJsonDocument &doc);

 private:
  QByteArray json_accum;
  int json_indent_level;
};


#endif  // JSONPARSER_H
