## automake.am
##
## Makefile for the glassplayergui(1) Audio Player front-end.
##
## (C) Copyright 2016 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License version 2 as
##   published by the Free Software Foundation.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing @QT4GUI_CFLAGS@ @SAMPLERATE_CFLAGS@
MOC = @QT4_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


bin_PROGRAMS = glassplayergui

dist_glassplayergui_SOURCES = glassplayergui.cpp glassplayergui.h\
                              playmeter.cpp playmeter.h\
                              segmeter.cpp segmeter.h\
                              statsdialog.cpp statsdialog.h\
                              statspanel.cpp statspanel.h

nodist_glassplayergui_SOURCES = audiodevice.cpp audiodevice.h\
                                cmdswitch.cpp cmdswitch.h\
                                codec.cpp codec.h\
                                connector.cpp connector.h\
                                glasslimits.h\
                                logging.cpp logging.h\
                                metaevent.cpp metaevent.h\
                                moc_audiodevice.cpp\
                                ringbuffer.cpp ringbuffer.h\
                                moc_codec.cpp\
                                moc_connector.cpp\
                                moc_glassplayergui.cpp\
                                moc_playmeter.cpp\
                                moc_segmeter.cpp\
                                moc_statsdialog.cpp\
                                moc_statspanel.cpp

glassplayergui_LDADD = @QT4GUI_LIBS@ @SAMPLERATE_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

DISTCLEANFILES = audiodevice.cpp audiodevice.h\
                 cmdswitch.cpp cmdswitch.h\
                 codec.cpp codec.h\
                 connector.cpp connector.h\
                 glasslimits.h\
                 logging.cpp logging.h\
                 metaevent.cpp metaevent.h\
                 ringbuffer.cpp ringbuffer.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
