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

#ifndef STATSDIALOG_H
#define STATSDIALOG_H

#include <map>

#include <QPushButton>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QTextEdit>

#include "statspanel.h"

class StatsDialog : public QDialog
{
 Q_OBJECT;
 public:
  StatsDialog(QWidget *parent=0);
  QSize sizeHint() const;
  void update(const QString &category,const QString &param,
	      const QString &value);

 signals:
  void closeClicked();

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  QTextEdit *stats_text;
  std::map<QString,StatsPanel *> stats_panels;
};


#endif  // STATSDIALOG_H
