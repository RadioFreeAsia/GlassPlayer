// m3uplaylist.cpp
//
// Abstract an M3U playlist
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

#include <stdio.h>

#include <QStringList>

#include "connector.h"
#include "logging.h"
#include "m3uplaylist.h"

M3uPlaylist::M3uPlaylist()
{
  clear();
}


bool M3uPlaylist::isExtended() const
{
  return m3u_extended;
}


int M3uPlaylist::version() const
{
  return m3u_version;
}


int M3uPlaylist::targetDuration() const
{
  return m3u_target_duration;
}


bool M3uPlaylist::isEnded() const
{
  return m3u_ended;
}


bool M3uPlaylist::segmentsAreIndependent() const
{
  return m3u_independent;
}


int M3uPlaylist::mediaSequence() const
{
  return m3u_media_sequence;
}


unsigned M3uPlaylist::segmentQuantity() const
{
  return m3u_segment_durations.size();
}


QDateTime M3uPlaylist::segmentDateTime(unsigned n) const
{
  return m3u_segment_datetimes[n];
}


double M3uPlaylist::segmentDuration(unsigned n) const
{
  return m3u_segment_durations[n];
}


QString M3uPlaylist::segmentTitle(unsigned n) const
{
  return m3u_segment_titles[n];
}


QUrl M3uPlaylist::segmentUrl(unsigned n) const
{
  return m3u_segment_urls[n];
}


bool M3uPlaylist::parse(const QByteArray &data)
{
  bool ok=false;
  QStringList f0=QString(data).split("\n");
  for(int i=0;i<f0.size();i++) {
    f0[i]=f0[i].trimmed();
  }

  clear();

  for(int i=0;i<f0.size();i++) {
    if(f0[i].left(1)=="#") {  // Tag
      if(f0[i]=="#EXTM3U") {
	m3u_extended=true;
      }
      if(m3u_extended) {
	QStringList f1=f0[i].split(":");

	if((f1[0]=="#EXT-X-VERSION")&&(f1.size()==2)) {
	  if(m3u_version!=-1) {
	    Log(LOG_WARNING,"hls: multiple EXT-X-VERSION tags not allowed");
	    return false;   // As per Pantos 4.3.1.2
	  }
	  m3u_version=f1[1].toInt(&ok);
	  if(!ok) {
	    Log(LOG_WARNING,"hls: invalid EXT-X-VERSION tag");
	    return false;
	  }
	}

	if((f1[0]=="#EXT-X-TARGETDURATION")&&(f1.size()==2)) {
	  m3u_target_duration=f1[1].toInt(&ok);
	  if(!ok) {
	    Log(LOG_WARNING,"hls: invalid EXT-X-TARGETDURATION tag");
	    return false;
	  }
	}

	if((f1[0]=="#EXT-X-MEDIA-SEQUENCE")&&(f1.size()==2)) {
	  m3u_media_sequence=f1[1].toInt(&ok);
	  if(!ok) {
	    Log(LOG_WARNING,"hls: invalid EXT-X-MEDIA-SEQUENCE tag");
	    return false;
	  }
	}

	if(f1[0]=="#EXT-X-ENDLIST") {
	  m3u_ended=true;
	}

	if(f1[0]=="#EXT-X-INDEPENDENT-SEGMENTS") {
	  m3u_independent=true;
	}

	if((f1[0]=="#EXTINF")&&(f1.size()==2)) {
	  QStringList f2=f1[1].split(",");
	  m3u_current_segment_duration=f2[0].toDouble(&ok);
	  if(!ok) {
	    Log(LOG_WARNING,"hls: invalid EXTINF tag");
	    return false;
	  }
	  if(f2.size()>1) {
	    f2.erase(f2.begin());
	    m3u_current_segment_title=f2.join(",");
	  }
	}

	if((f1[0]=="#EXT-X-PROGRAM-DATE-TIME")&&(f1.size()>1)) {
	  f1.erase(f1.begin());
	  m3u_current_segment_datetime=Connector::xmlTimestamp(f1.join(":"));
	}
      }
    }
    else {
      if(!f0[i].isEmpty()) {
	m3u_segment_urls.push_back(QUrl(f0[i]));
	if(!m3u_segment_urls.back().isValid()) {
	  Log(LOG_WARNING,"hls: invalid URL");
	  return false;
	}
	m3u_segment_durations.push_back(m3u_current_segment_duration);
	m3u_current_segment_duration=0.0;
	m3u_segment_titles.push_back(m3u_current_segment_title);
	m3u_current_segment_title="";
	m3u_segment_datetimes.push_back(m3u_current_segment_datetime);
	m3u_current_segment_datetime=QDateTime();
      }
    }
  }
  return true;
}


QString M3uPlaylist::dump() const
{
  QString ret="";

  if(m3u_extended) {
    ret+="#EXTM3U\r\n";
  }
  ret+=QString().sprintf("#EXT-X-TARGETDURATION:%d\r\n",m3u_target_duration);
  if(m3u_version>=0) {
    ret+=QString().sprintf("#EXT-X-VERSION:%d\r\n",m3u_version);
  }
  if(m3u_media_sequence>=0) {
    ret+=QString().sprintf("#EXT-X-MEDIA-SEQUENCE:%d\r\n",m3u_media_sequence);
  }
  if(m3u_ended) {
    ret+="#EXT-X-ENDLIST\r\n" ;
  }
  if(m3u_independent) {
    ret+="#EXT-X-INDEPENDENT-SEGMENTS\r\n";
  }
  
  for(unsigned i=0;i<m3u_segment_titles.size();i++) {
    if(m3u_segment_datetimes[i].isValid()) {
      ret+="#EXT-X-PROGRAM-DATE-TIME:"+
	m3u_segment_datetimes[i].toString("yyyy-MM-dd")+"T"+
	m3u_segment_datetimes[i].toString("hh:mm:ss:zzz")+
	Connector::timezoneOffsetString()+"\r\n";
    }
    ret+=QString().sprintf("#EXTINF:%7.5lf,",m3u_segment_durations[i])+
      m3u_segment_titles[i]+"\r\n";
    ret+=m3u_segment_urls[i].toString()+"\r\n";
  }

  return ret;
}


void M3uPlaylist::clear()
{
  m3u_extended=false;
  m3u_version=-1;
  m3u_target_duration=10;
  m3u_ended=false;
  m3u_independent=false;
  m3u_media_sequence=-1;
  m3u_current_segment_duration=0.0;
  m3u_segment_durations.clear();
  m3u_current_segment_title="";
  m3u_segment_titles.clear();
  m3u_current_segment_datetime=QDateTime();
  m3u_segment_datetimes.clear();
  m3u_segment_urls.clear();
}


bool M3uPlaylist::operator!=(const M3uPlaylist &plist)
{
  return (m3u_extended!=plist.m3u_extended)||
    (m3u_version!=plist.m3u_version)||
    (m3u_target_duration!=plist.m3u_target_duration)||
    (m3u_ended!=plist.m3u_ended)||
    (m3u_independent!=plist.m3u_independent)||
    (m3u_media_sequence!=plist.m3u_media_sequence)||
    (m3u_segment_durations!=plist.m3u_segment_durations)||
    (m3u_segment_titles!=plist.m3u_segment_titles)||
    (m3u_segment_datetimes!=plist.m3u_segment_datetimes)||
    (m3u_segment_urls!=plist.m3u_segment_urls);
}
