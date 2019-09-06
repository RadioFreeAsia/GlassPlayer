// statspanel.cpp
//
// Stats viewer section
//
//   (C) Copyright 2016-2019 Fred Gleason <fredg@paravelsystems.com>
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

#include "statspanel.h"

StatsPanel::StatsPanel(const QString &category,QWidget *parent)
  : QWidget(parent)
{
  stats_category=category;

  stats_text=new QTextEdit(this);
  stats_text->setReadOnly(true);
}


QSize StatsPanel::sizeHint() const
{
  return QSize(400,300);
}


void StatsPanel::update(const QString &param,const QString &value)
{
  stats_values[param]=value;
  QString text;

  for(std::map<QString,QString>::const_iterator it=stats_values.begin();
      it!=stats_values.end();it++) {
    text+="<strong>"+it->first+": </strong>"+it->second+"<br>";
  }
  stats_text->setText(text);
}


void StatsPanel::resizeEvent(QResizeEvent *e)
{
  stats_text->setGeometry(0,0,size().width(),size().height());
}
