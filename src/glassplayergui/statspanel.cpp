// statspanel.cpp
//
// Stats viewer section
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

#include "statspanel.h"

StatsPanel::StatsPanel(const QString &category,QWidget *parent)
  : QWidget(parent)
{
  stats_category=category;

  QFont label_font=font();
  label_font.setPointSize(font().pointSize()+2);
  label_font.setWeight(QFont::Bold);

  stats_category_label=new QLabel(category,this);
  stats_category_label->setFont(label_font);
  stats_category_label->setAlignment(Qt::AlignBottom|Qt::AlignLeft);

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
  stats_category_label->setGeometry(10,0,size().width()-20,25);
  stats_text->setGeometry(0,25,size().width(),size().height()-25);
}
