// glassplayergui.cpp
//
// glassplayergui(1) Audio Receiver front end
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

#include <QApplication>
#include <QCloseEvent>
#include <QJsonObject>
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

  CmdSwitch *cmd=new CmdSwitch("glassplayergui",GLASSPLAYERGUI_USAGE);
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
  // Metadata Title
  //
  gui_title_text=new QLabel(tr("The GlassPlayer"),this);
  gui_title_text->setFont(title_font);

  //
  // Metadata Fields
  //
  for(int i=0;i<GLASSPLAYERGUI_METADATA_FIELD_QUAN;i++) {
    gui_metadata_labels[i]=new QLabel(tr("Stream Name")+":",this);
    gui_metadata_labels[i]->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    gui_metadata_labels[i]->setFont(bold_font);

    gui_metadata_texts[i]=new QLabel(this);
    gui_metadata_texts[i]->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  }

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

  gui_json_parser=new JsonParser(this);
  connect(gui_json_parser,SIGNAL(newDocument(const QJsonDocument &)),
	  this,SLOT(newJsonDocumentData(const QJsonDocument &)));

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

  args.push_back("--json");
  args.push_back("--meter-data");
  args.push_back("--metadata-out");
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
  gui_json_parser->addData(gui_player_process->readAllStandardOutput());
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


void MainWidget::newJsonDocumentData(const QJsonDocument &doc)
{
  if(!doc.isNull()) {
    if(doc.isObject()) {
      QJsonObject obj=doc.object();
      if(obj.keys().at(0)=="Meter") {
	ProcessMeterUpdates(obj.value("Meter").toObject().value("Update").
			    toString().split(" ").at(1));
      }
      else {
	if(obj.keys().at(0)=="Metadata") {
	  ProcessMetadataUpdates(obj.value("Metadata").toObject());
	}

	//
	// Stats Dialog
	//
	QStringList keys=obj.value(obj.keys().at(0)).toObject().keys();
	  for(int i=0;i<keys.size();i++) {
	    QJsonValue v=
	      obj.value(obj.keys().at(0)).toObject().value(keys.at(i));
	    if(v.isString()) {
	      gui_stats_dialog->update(obj.keys().at(0),keys.at(i),
				       v.toString());
	    }
	    else {
	      gui_stats_dialog->update(obj.keys().at(0),keys.at(i),
				       QString().sprintf("%d",v.toInt()));
	    }
	  }
      }
    }
  }
}


void MainWidget::logoProcessFinishedData(int exit_code,
					 QProcess::ExitStatus status)
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

  for(int i=0;i<GLASSPLAYERGUI_METADATA_FIELD_QUAN;i++) {
    if(gui_metadata_texts[i]->text().isEmpty()) {
      gui_metadata_labels[i]->hide();
      gui_metadata_texts[i]->hide();
    }
    else {
      gui_metadata_labels[i]->show();
      gui_metadata_texts[i]->show();
      gui_metadata_labels[i]->setGeometry(10,ypos,120,20);
      gui_metadata_texts[i]->setGeometry(135,ypos,size().width()-225,20);
      ypos+=20;
    }
  }

  gui_stats_button->setGeometry(10,size().height()-40,110,35);

  gui_logo_label->
    setGeometry(size().width()-edge-MAX_AUDIO_CHANNELS*10-15,
		size().height()-edge-10,edge,edge);

  for(int i=0;i<MAX_AUDIO_CHANNELS;i++) {
    gui_meters[i]->setGeometry(size().width()-10*(MAX_AUDIO_CHANNELS-i)-10,30,
			       10,size().height()-40);
  }
}


void MainWidget::ProcessMetadataUpdates(const QJsonObject &obj)
{
  QStringList keys=obj.keys();
  int next_field=0;

  //
  // HLS-Style Fields
  //
  if(obj.value("TRSN")!=QJsonValue::Undefined) {
    gui_title_text->setText(obj.value("TRSN").toString());
  }
  if((obj.value("TRSO")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    QString text=gui_title_text->text();
    if(!text.isEmpty()) {
      text+=" - ";
    }
    gui_title_text->setText(text+obj.value("TRSO").toString());
  }
  if((obj.value("TIT2")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    gui_metadata_labels[next_field]->setText(tr("Title")+":");
    gui_metadata_texts[next_field]->setText(obj.value("TIT2").toString());
    next_field++;
  }
  if((obj.value("TPE1")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    gui_metadata_labels[next_field]->setText(tr("Artist")+":");
    gui_metadata_texts[next_field]->setText(obj.value("TPE1").toString());
    next_field++;
  }
  if((obj.value("TALB")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    if(obj.value("TALB")==QJsonValue::String) {
      gui_metadata_labels[next_field]->setText(tr("Album")+":");
      gui_metadata_texts[next_field]->setText(obj.value("TALB").toString());
    }
    else {
      gui_metadata_labels[next_field]->setText(tr("Year")+":");
      gui_metadata_texts[next_field]->
	setText(QString().sprintf("%d",obj.value("TALB").toInt()));
    }
    next_field++;
  }

  //
  // X-Cast Style Fields
  //
  if(obj.value("icy-name")!=QJsonValue::Undefined) {
    gui_title_text->setText(obj.value("icy-name").toString());
  }

  if((obj.value("icy-description")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    QString text=gui_title_text->text();
    if(!text.isEmpty()) {
      text+=" - ";
    }
    gui_title_text->setText(text+obj.value("icy-description").toString());
  }

  if((obj.value("icy-genre")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    gui_metadata_labels[next_field]->setText(tr("Genre")+":");
    gui_metadata_texts[next_field]->setText(obj.value("icy-genre").toString());
    next_field++;
  }

  if((obj.value("StreamTitle")!=QJsonValue::Undefined)&&
     (next_field<GLASSPLAYERGUI_METADATA_FIELD_QUAN)) {
    gui_metadata_labels[next_field]->setText(tr("Now Playing")+":");
    gui_metadata_texts[next_field]->setText(obj.value("StreamTitle").toString());
    next_field++;
  }

  if(obj.value("StreamUrl")!=QJsonValue::Undefined) {
    GetLogo(obj.value("StreamUrl").toString());
  }

  resizeEvent(NULL);
  /*
  for(int i=0;i<keys.size();i++) {
    printf("%s: %s\n",(const char *)keys.at(i).toUtf8(),
	   (const char *)obj.value(keys.at(i)).toString().toUtf8());
  }
  */
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
