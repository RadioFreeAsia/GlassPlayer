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
#include <QStringList>

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
  // Logo
  //
  gui_logo_label=new QLabel(this);
  gui_logo_label->setAlignment(Qt::AlignCenter);
  gui_logo_label->setScaledContents(true);

  if(!gui_url.isEmpty()) {
    processStart(gui_url);
  }

  setMinimumSize(sizeHint());
  setMaximumHeight(sizeHint().height());
}


QSize MainWidget::sizeHint() const
{
  return QSize(500,175);
}


void MainWidget::processStart(const QString &url)
{
  QStringList args;

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
  QByteArray data;
  int ptr=-1;

  while(gui_player_process->bytesAvailable()>0) {
    data=gui_player_process->read(1024);
    QString line(data);
    if((ptr=line.indexOf("\n\n"))>=0) {
      ProcessStats(gui_stats_buffer+line.left(ptr+1));
      gui_stats_buffer=line.right(line.length()-ptr-2);
    }
    else {
      gui_stats_buffer+=line;
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
    QMessageBox::critical(this,"GlassPlayer",tr("Logo download crashed!"));
    exit(256);
  }
  else {
    if(exit_code!=0) {
      QMessageBox::critical(this,"GlassPlayer",
		  tr("Logo download process exited with non-zero exit code")+
			    QString().sprintf(" [%d]!",exit_code));
      exit(256);
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
    ypos+=22;
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
    ypos+=22;
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
    ypos+=22;
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
    ypos+=22;
  }

  gui_logo_label->
    setGeometry(size().width()-edge-10,size().height()-edge-10,edge,edge);
}


void MainWidget::ProcessStats(const QString &str)
{
  //    printf("STATS: %s\n",(const char *)str.toUtf8());

  QString category;
  QString param;
  QString value;

  QStringList lines=str.split("\n",QString::KeepEmptyParts);
  for(int i=0;i<lines.size();i++) {
    if(!lines[i].isEmpty()) {
      QStringList f0=lines[i].split(": ");
      QStringList f1=f0[0].split("|",QString::KeepEmptyParts);
      category=f1[0];
      if(f1.size()==2) {
	param=f1[1];
      }
      f0.erase(f0.begin());
      value=f0.join(": ");

      UpdateStat(category.toLower(),param.toLower(),value);
    }
  }
}


void MainWidget::UpdateStat(const QString &category,const QString &param,
			    const QString &value)
{
  QString misc;

  if(category=="metadata") {
    /*
    if(!value.isEmpty()) {
      printf("%s: %s\n",(const char *)param.toUtf8(),
	     (const char *)value.toUtf8());
    }
    */
    if(param=="streamtitle") {
      if(value.isEmpty()) {
	gui_title_text->setText(tr("The GlassPlayer"));
      }
      else {
	gui_title_text->setText(value);
      }
    }
    if(param=="name") {
      gui_name_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="description") {
      gui_description_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="channelurl") {
      gui_channelurl_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="genre") {
      gui_genre_text->setText(value);
      resizeEvent(NULL);
    }
    if(param=="streamurl") {
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
