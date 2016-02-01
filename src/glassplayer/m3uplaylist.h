// m3uplaylist.h
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

#ifndef M3UPLAYLIST_H
#define M3UPLAYLIST_H

#include <vector>

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QUrl>

class M3uPlaylist
{
 public:
  M3uPlaylist();
  bool isExtended() const;
  int version() const;
  int targetDuration() const;
  bool isEnded() const;
  bool segmentsAreIndependent() const;
  int mediaSequence() const;
  unsigned segmentQuantity() const;
  QDateTime segmentDateTime(unsigned n) const;
  double segmentDuration(unsigned n) const;
  QString segmentTitle(unsigned n) const;
  QUrl segmentUrl(unsigned n) const;
  bool parse(const QByteArray &data);
  QString dump() const;
  void clear();
  bool operator!=(const M3uPlaylist &plist);

 private:
  bool m3u_extended;
  int m3u_version;
  int m3u_target_duration;
  bool m3u_ended;
  bool m3u_independent;
  int m3u_media_sequence;
  double m3u_current_segment_duration;
  std::vector<double> m3u_segment_durations;
  QString m3u_current_segment_title;
  std::vector<QString> m3u_segment_titles;
  QDateTime m3u_current_segment_datetime;
  std::vector<QDateTime> m3u_segment_datetimes;
  std::vector<QUrl> m3u_segment_urls;
};


#endif  // M3UPLAYLIST_H
