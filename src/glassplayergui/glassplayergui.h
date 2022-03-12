// glassplayergui.h
//
// glassplayergui(1) Audio Receiver front end
//
//   (C) Copyright 2016-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QJsonObject>
#include <QLabel>
#include <QMainWindow>
#include <QProcess>
#include <QPushButton>
#include <QStringList>

#include "glasslimits.h"
#include "jsonparser.h"
#include "playmeter.h"
#include "statsdialog.h"

#define GLASSPLAYERGUI_METADATA_FIELD_QUAN 4
#define GLASSPLAYERGUI_USAGE "[options]\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void showStatsData();
  void processStart(const QString &url, const QStringList &pt_args);
  void processReadyReadData();
  void processFinishedData(int exit_code,QProcess::ExitStatus status);
  void processErrorData(QProcess::ProcessError err);
  void newJsonDocumentData(const QJsonDocument &doc);
  void logoProcessFinishedData(int exit_code,QProcess::ExitStatus status);
  void logoProcessErrorData(QProcess::ProcessError err);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  void ProcessMetadataUpdates(const QJsonObject &obj);
  void ProcessMeterUpdates(const QString &values);
  void GetLogo(const QString &url);
  QLabel *gui_title_text;
  QLabel *gui_metadata_labels[GLASSPLAYERGUI_METADATA_FIELD_QUAN];
  QLabel *gui_metadata_texts[GLASSPLAYERGUI_METADATA_FIELD_QUAN];
  QLabel *gui_logo_label;
  QProcess *gui_player_process;
  QProcess *gui_logo_process;
  JsonParser *gui_json_parser;
  QStringList gui_stats_list;
  QString gui_url;
  PlayMeter *gui_meters[MAX_AUDIO_CHANNELS];
  StatsDialog *gui_stats_dialog;
  QPushButton *gui_stats_button;
};


#endif  // GLASSPLAYERGUI_H
