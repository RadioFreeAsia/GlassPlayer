// statsdialog.h
//
// Stats viewer dialog
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

#include "statsdialog.h"

StatsDialog::StatsDialog(QWidget *parent)
  : QDialog(parent)
{
}


QSize StatsDialog::sizeHint() const
{
  return QSize(400,600);
}


void StatsDialog::update(const QString &category,const QString &param,
			 const QString &value)
{
  if(stats_panels[category]==NULL) {
    stats_panels[category]=new StatsPanel(category,this);
    resizeEvent(NULL);
  }
  stats_panels[category]->update(param,value);
}


void StatsDialog::resizeEvent(QResizeEvent *e)
{
  int count=0;
  int w=size().width();
  int h=size().height();

  for(std::map<QString,StatsPanel *>::const_iterator it=stats_panels.begin();
      it!=stats_panels.end();it++) {
    it->second->setGeometry(0,
			    count*h/(stats_panels.size()),
			    w,
			    h/stats_panels.size());
    count++;
  }
}
