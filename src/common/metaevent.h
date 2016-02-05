// metaevent.h
//
// Container class for metadata updates.
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

#ifndef METAEVENT_H
#define METAEVENT_H

#include <stdint.h>

#include <QVariant>

class MetaEvent
{
 public:
  enum Field {Title=0,Url=1,LastField=2};
  MetaEvent();
  MetaEvent(const MetaEvent &e);
  QVariant field(Field f) const;
  void setField(Field f,const QVariant v);

 private:
  QVariant meta_fields[MetaEvent::LastField];
};


#endif  // METAEVENT_H
