// glassplayergui.cpp
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

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPixmap>

#include "cmdswitch.h"
#include "connector.h"
#include "glassplayergui.h"

//#include "../../icons/glassplayer-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent)
{
  gui_player_process=NULL;
  gui_logo_process=NULL;

  CmdSwitch *cmd=new CmdSwitch(qApp->argc(),qApp->argv(),"glassplayergui",
			       GLASSPLAYERGUI_USAGE);
  if(cmd->keys()>0) {
    for(unsigned i=0;i<(cmd->keys()-1);i++) {
      if(!cmd->processed(i)) {
	QMessageBox::critical(this,"GlassPlayer - "+tr("Error"),
			      tr("Unknown argument")+" \""+cmd->key(i)+"\".");
	exit(256);
      }
    }
    gui_url=cmd->key(cmd->keys()-1);
  }

  QFont title_font=font();
  title_font.setPointSize(font().pointSize()+2);
  title_font.setWeight(QFont::Bold);

  QFont bold_font=font();
  bold_font.setWeight(QFont::Bold);

  setWindowTitle(QString("GlassPlayer - v")+VERSION);

  gui_stats_dialog=new StatsDialog(this);

  //
  // Title
  //
  gui_title_text=new QLabel(tr("The GlassPlayer"),this);
  gui_title_text->setFont(title_font);

  //
  // Stream Name
  //
  gui_name_label=new QLabel(tr("Stream Name")+":",this);
  gui_name_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  gui_name_label->setFont(bold_font);

  gui_name_text=new QLabel(this);
  gui_name_text->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Stream Description
  //
  gui_description_label=new QLabel(tr("Description")+":",this);
  gui_description_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  gui_description_label->setFont(bold_font);

  gui_description_text=new QLabel(this);
  gui_description_text->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Channel URL
  //
  gui_channelurl_label=new QLabel(tr("Channel URL")+":",this);
  gui_channelurl_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  gui_channelurl_label->setFont(bold_font);

  gui_channelurl_text=new QLabel(this);
  gui_channelurl_text->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Genre
  //
  gui_genre_label=new QLabel(tr("Genre")+":",this);
  gui_genre_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  gui_genre_label->setFont(bold_font);

  gui_genre_text=new QLabel(this);
  gui_genre_text->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

  //
  // Stats Button
  //
  gui_stats_button=new QPushButton(tr("Show Stats"),this);
  gui_stats_button->setFont(bold_font);
  connect(gui_stats_button,SIGNAL(clicked()),this,SLOT(showStatsData()));
  connect(gui_stats_dialog,SIGNAL(closeClicked()),this,SLOT(showStatsData()));

  //
  // Logo
  //
  gui_logo_label=new QLabel(this);
  gui_logo_label->setAlignment(Qt::AlignCenter);
  gui_logo_label->setScaledContents(true);

  //
  // Meters
  //
  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    gui_meters[i]=new PlayMeter(SegMeter::Up,this);
    gui_meters[i]->setRange(-3000,0);
    gui_meters[i]->setHighThreshold(-800);
    gui_meters[i]->setClipThreshold(-100);
    gui_meters[i]->setMode(SegMeter::Peak);
  }
  gui_meters[0]->setLabel(tr("L"));
  gui_meters[1]->setLabel(tr("R"));

  if(!gui_url.isEmpty()) {
    processStart(gui_url);
  }

  setMinimumSize(sizeHint());
  setMaximumHeight(sizeHint().height());
}


QSize MainWidget::sizeHint() const
{
  return QSize(540,175);
}


void MainWidget::showStatsData()
{
  if(gui_stats_dialog->isVisible()) {
    gui_stats_dialog->hide();
    gui_stats_button->setText(tr("Show Stats"));
  }
  else {
    gui_stats_dialog->show();
    gui_stats_button->setText(tr("Hide Stats"));
  }
}


void MainWidget::processStart(const QString &url)
{
  QStringList args;

  args.push_back("--meter-data");
  args.push_back("--stats-out");
  args.push_back(url);
  if(gui_player_process!=NULL) {
    delete gui_player_process;
  }
  gui_player_process=new QProcess(this);
  gui_player_process->setReadChannel(QProcess::StandardOutput);
  connect(gui_player_process,SIGNAL(readyRead()),this,SLOT(processReadyReadData()));
  connect(gui_player_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(processErrorData(QProcess::ProcessError)));
  connect(gui_player_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(processFinishedData(int,QProcess::ExitStatus)));
  gui_player_process->
    start("glassplayer",args,QIODevice::Unbuffered|QIODevice::ReadOnly);
}


void MainWidget::processReadyReadData()
{
  QString line;
  QStringList f0;

  while(gui_player_process->canReadLine()) {
    line=gui_player_process->readLine().trimmed();
    if(line.isEmpty()) {
      ProcessStats(gui_stats_list);
      gui_stats_list.clear();
    }
    else {
      f0=line.split(" ");
      if(f0[0]=="ME") {
	if(f0.size()==2) {
	  ProcessMeterUpdates(f0[1]);
	}
      }
      else {
	gui_stats_list.push_back(line);
      }
    }
  }
}


void MainWidget::processFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status!=QProcess::NormalExit) {
    QMessageBox::critical(this,"GlassPlayer",tr("Player process crashed!"));
    exit(256);
  }
  else {
    if(exit_code!=0) {
      QMessageBox::critical(this,"GlassPlayer",
			    tr("Player process exited with non-zero exit code")+
			    QString().sprintf(" [%d]!",exit_code));
      exit(256);
    }
    else {
      exit(0);
    }
  }
}


void MainWidget::processErrorData(QProcess::ProcessError err)
{
  QMessageBox::critical(this,"GlassPlayer",tr("Player process error")+" ["+
			Connector::processErrorText(err)+"]!");
  exit(256);
}


void MainWidget::logoProcessFinishedData(int exit_code,QProcess::ExitStatus status)
{
  if(status!=QProcess::NormalExit) {
    fprintf(stderr,"glassplayergui: %s\n",
	    (const char *)tr("logo download process crashed").toUtf8());
  }
  else {
    if(exit_code!=0) {
      fprintf(stderr,"glassplayergui: %s\n",(const char *)
	      (tr("logo download process exited with non-zero exit code")+
	       QString().sprintf(" [%d]!",exit_code)).toUtf8());
    }
    else {
      QPixmap *pix=new QPixmap();
      if(pix->loadFromData(gui_logo_process->readAllStandardOutput())) {
	gui_logo_label->setPixmap(*pix);
	delete pix;
	resizeEvent(NULL);
      }
    }
  }
}


void MainWidget::logoProcessErrorData(QProcess::ProcessError err)
{
  QMessageBox::critical(this,"GlassPlayer",tr("Logo download process error")+
			" ["+Connector::processErrorText(err)+"]!");
  exit(256);
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  if(gui_player_process!=NULL) {
    if(gui_player_process->state()!=QProcess::NotRunning) {
      gui_player_process->terminate();
      gui_player_process->waitForFinished(5000);
      if(gui_player_process->state()!=QProcess::NotRunning) {
	gui_player_process->kill();
      }
    }
  }

  e->accept();
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int ypos=10;

  gui_title_text->setGeometry(10,ypos,size().width()-20,20);
  ypos+=22;

  int edge=size().height()-42;
  int right=size().width()-145;
  if(gui_logo_label->pixmap()!=NULL) {
    right-=edge+10;
  }

  if(gui_name_text->text().isEmpty()) {
    gui_name_label->hide();
    gui_name_text->hide();
  }
  else {
    gui_name_label->show();
    gui_name_text->show();
    gui_name_label->setGeometry(10,ypos,120,20);
    gui_name_text->setGeometry(135,ypos,size().width()-225,20);
    ypos+=20;
  }

  if(gui_description_text->text().isEmpty()) {
    gui_description_label->hide();
    gui_description_text->hide();
  }
  else {
    gui_description_label->show();
    gui_description_text->show();
    gui_description_label->setGeometry(10,ypos,120,20);
    gui_description_text->setGeometry(135,ypos,size().width()-225,20);
    ypos+=20;
  }

  if(gui_channelurl_text->text().isEmpty()) {
    gui_channelurl_label->hide();
    gui_channelurl_text->hide();
  }
  else {
    gui_channelurl_label->show();
    gui_channelurl_text->show();
    gui_channelurl_label->setGeometry(10,ypos,120,20);
    gui_channelurl_text->setGeometry(135,ypos,size().width()-225,20);
    ypos+=20;
  }

  if(gui_genre_text->text().isEmpty()) {
    gui_genre_label->hide();
    gui_genre_text->hide();
  }
  else {
    gui_genre_label->show();
    gui_genre_text->show();
    gui_genre_label->setGeometry(10,ypos,120,20);
    gui_genre_text->setGeometry(135,ypos,right,20);
    ypos+=20;
  }

  gui_stats_button->setGeometry(10,size().height()-40,110,35);

  gui_logo_label->
    setGeometry(size().width()-edge-MAX_AUDIO_CHANNELS*10-15,size().height()-edge-10,edge,edge);

  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    gui_meters[i]->setGeometry(size().width()-10*(MAX_AUDIO_CHANNELS-i)-10,
			       30,10,size().height()-40);
  }
}


void MainWidget::ProcessStats(const QStringList &stats)
{
  QString category;
  QString param;
  QString value;

  for(int i=0;i<stats.size();i++) {
    if(!stats[i].isEmpty()) {
      QStringList f0=stats[i].split(": ");
      QStringList f1=f0[0].split("|",QString::KeepEmptyParts);
      category=f1[0];
      if(f1.size()==2) {
	param=f1[1];
      }
      f0.erase(f0.begin());
      value=f0.join(": ");

      UpdateStat(category,param,value);
    }
  }
}


void MainWidget::ProcessMeterUpdates(const QString &values)
{
  int level;
  bool ok=false;

  level=values.left(4).toInt(&ok,16);
  if(ok) {
    gui_meters[0]->setPeakBar(-level);
  }
  level=values.right(4).toInt(&ok,16);
  if(ok) {
    gui_meters[1]->setPeakBar(-level);
  }
}


void MainWidget::UpdateStat(const QString &category,const QString &param,
			    const QString &value)
{
  QString misc;

  gui_stats_dialog->update(category,param,value);

  if(category=="Metadata") {
    if(param=="StreamTitle") {
      if(value.isEmpty()) {
	gui_title_text->setText(tr("The GlassPlayer"));
      }
      else {
	gui_title_text->setText(value);
      }
    }
    if(param=="Name") {
      gui_name_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="Description") {
      gui_description_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="ChannelUrl") {
      gui_channelurl_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="Genre") {
      gui_genre_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="StreamUrl") {
      GetLogo(value);
    }
  }
}


void MainWidget::GetLogo(const QString &url)
{
  QStringList args;

  if(url.isEmpty()) {
    return;
  }
  args.push_back("--user-agent");
  args.push_back(GLASSPLAYERGUI_USER_AGENT);
  args.push_back(url);

  if(gui_logo_process!=NULL) {
    delete gui_logo_process;
  }
  gui_logo_process=new QProcess(this);
  gui_logo_process->setReadChannel(QProcess::StandardOutput);
  connect(gui_logo_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(logoProcessErrorData(QProcess::ProcessError)));
  connect(gui_logo_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(logoProcessFinishedData(int,QProcess::ExitStatus)));
  gui_logo_process->start("curl",args);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
