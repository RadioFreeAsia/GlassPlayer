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
#include <QStringList>

#include "cmdswitch.h"
#include "connector.h"
#include "glassplayergui.h"

//#include "../../icons/glassplayer-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent)
{
  gui_process=NULL;

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

  if(!gui_url.isEmpty()) {
    processStart(gui_url);
  }

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
  if(gui_process!=NULL) {
    delete gui_process;
  }
  gui_process=new QProcess(this);
  gui_process->setReadChannel(QProcess::StandardOutput);
  connect(gui_process,SIGNAL(readyRead()),this,SLOT(processReadyReadData()));
  connect(gui_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(processErrorData(QProcess::ProcessError)));
  connect(gui_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(processFinishedData(int,QProcess::ExitStatus)));
  gui_process->
    start("glassplayer",args,QIODevice::Unbuffered|QIODevice::ReadOnly);
}


void MainWidget::processReadyReadData()
{
  QByteArray data;
  int ptr=-1;

  while(gui_process->bytesAvailable()>0) {
    data=gui_process->read(1024);
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


void MainWidget::closeEvent(QCloseEvent *e)
{
  if(gui_process!=NULL) {
    if(gui_process->state()!=QProcess::NotRunning) {
      gui_process->terminate();
      gui_process->waitForFinished(5000);
      if(gui_process->state()!=QProcess::NotRunning) {
	gui_process->kill();
      }
    }
  }

  e->accept();
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
}


void MainWidget::ProcessStats(const QString &str)
{
  printf("STATS: %s\n",(const char *)str.toUtf8());
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
