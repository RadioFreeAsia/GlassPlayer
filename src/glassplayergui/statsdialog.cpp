// statsdialog.h
//
// Stats viewer dialog
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

#include "statsdialog.h"

StatsDialog::StatsDialog(QWidget *parent)
  : QDialog(parent)
{
  QFont label_font(font().family(),font().pointSize(),QFont::Bold);

  stats_category_label=new QLabel(tr("View")+":",this);
  stats_category_label->setFont(label_font);
  stats_category_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  stats_category_box=new ComboBox(this);
  connect(stats_category_box,SIGNAL(activated(const QString &)),
	  this,SLOT(categoryActivatedData(const QString &)));

  setWindowTitle("GlassPlayer - "+tr("Stream Statistics"));

  setMinimumSize(sizeHint());
}


QSize StatsDialog::sizeHint() const
{
  return QSize(540,550);
}


void StatsDialog::update(const QString &category,const QString &param,
			 const QString &value)
{
  if(stats_panels[category]==NULL) {
    stats_panels[category]=new StatsPanel(category,this);
    stats_panels.value(category)->hide();
    resizeEvent(NULL);
  }
  stats_panels.value(category)->update(param,value);

  if(!stats_category_box->contains(category)) {
    stats_category_box->
      insertItem(stats_category_box->count(),category,category);
    if(stats_category_box->count()==1) {
      categoryActivatedData(category);
    }
  }
}


void StatsDialog::categoryActivatedData(const QString &str)
{
  for(QMap<QString,StatsPanel *>::const_iterator it=stats_panels.begin();
      it!=stats_panels.end();it++) {
    it.value()->setVisible(it.key()==str);
  }
}


void StatsDialog::closeEvent(QCloseEvent *e)
{
  emit closeClicked();
}


void StatsDialog::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();

  stats_category_label->setGeometry(10,2,95,20);
  stats_category_box->setGeometry(110,2,200,20);

  for(QMap<QString,StatsPanel *>::const_iterator it=stats_panels.begin();
      it!=stats_panels.end();it++) {
    it.value()->setGeometry(0,25,w,h-25);
  }
}
