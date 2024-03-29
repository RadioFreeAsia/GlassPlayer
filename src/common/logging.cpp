// logging.cpp
//
// Logging routines for glassplayer(1).
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#include "logging.h"

int global_log_to=LOG_TO_STDERR;
bool global_log_verbose=false;

void Log(int prio,const QString &msg)
{
  switch(global_log_to) {
  case LOG_TO_SYSLOG:
#ifndef WIN32
    syslog(prio,"%s",msg.toUtf8().constData());
#endif  // WIN32
    break;

  case LOG_TO_STDOUT:
    printf("ER %d %s\n",prio,msg.toUtf8().constData());
#ifndef WIN32
    syslog(prio,"%s",msg.toUtf8().constData());
#endif  // WIN32
    break;

  default:
    if(global_log_verbose||(prio<LOG_DEBUG)) {
      fprintf(stderr,"%s\n",msg.toUtf8().constData());
    }
    break;
  }
}
