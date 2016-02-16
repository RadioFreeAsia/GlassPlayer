// glassplayergui.h
//
// glassplayergui(1) Audio Receiver front end
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

#ifndef GLASSGUIPLAYER_H
#define GLASSGUIPLAYER_H

#include <QLabel>
#include <QMainWindow>
#include <QProcess>
#include <QPushButton>

#include "statsdialog.h"

#define GLASSPLAYERGUI_USAGE "[options]\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void showStatsData();
  void processStart(const QString &url);
  void processReadyReadData();
  void processFinishedData(int exit_code,QProcess::ExitStatus status);
  void processErrorData(QProcess::ProcessError err);
  void logoProcessFinishedData(int exit_code,QProcess::ExitStatus status);
  void logoProcessErrorData(QProcess::ProcessError err);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  void ProcessStats(const QString &str);
  void UpdateStat(const QString &category,const QString &param,
		  const QString &value);
  void GetLogo(const QString &url);
  QLabel *gui_title_text;
  QLabel *gui_name_label;
  QLabel *gui_name_text;
  QLabel *gui_description_label;
  QLabel *gui_description_text;
  QLabel *gui_channelurl_label;
  QLabel *gui_channelurl_text;
  QLabel *gui_genre_label;
  QLabel *gui_genre_text;
  QLabel *gui_logo_label;
  QProcess *gui_player_process;
  QProcess *gui_logo_process;
  QString gui_stats_buffer;
  QString gui_url;
  StatsDialog *gui_stats_dialog;
  QPushButton *gui_stats_button;
};


#endif  // GLASSPLAYERGUI_H
