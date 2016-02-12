// statspanel.h
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

#ifndef STATSPANEL_H
#define STATSPANEL_H

#include <map>

#include <QLabel>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QWidget>

class StatsPanel : public QWidget
{
 Q_OBJECT;
 public:
  StatsPanel(const QString &category,QWidget *parent=0);
  QSize sizeHint() const;
  void update(const QString &param,const QString &value);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  QLabel *stats_category_label;
  QTextEdit *stats_text;
  QString stats_category;
  std::map<QString,QString> stats_values;
};


#endif  // STATSPANEL_H
