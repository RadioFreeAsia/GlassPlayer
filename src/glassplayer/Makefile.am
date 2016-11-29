## automake.am
##
## Makefile for the glassplayer(1) Audio Encoder.
##
## (C) Copyright 2014-2015 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing @QT4_CFLAGS@ @SNDFILE_CFLAGS@ @ALSA_CFLAGS@
MOC = @QT4_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


bin_PROGRAMS = glassplayer

dist_glassplayer_SOURCES = audiodevicefactory.cpp audiodevicefactory.h\
                           codec_fdk.cpp codec_fdk.h\
                           codec_mpeg1.cpp codec_mpeg1.h\
                           codec_null.cpp codec_null.h\
                           codec_ogg.cpp codec_ogg.h\
                           codec_pass.cpp codec_pass.h\
                           codecfactory.cpp codecfactory.h\
                           conn_file.cpp conn_file.h\
                           conn_hls.cpp conn_hls.h\
                           conn_xcast.cpp conn_xcast.h\
                           connectorfactory.cpp connectorfactory.h\
                           dev_alsa.cpp dev_alsa.h\
                           dev_file.cpp dev_file.h\
                           dev_jack.cpp dev_jack.h\
                           dev_stdout.cpp dev_stdout.h\
                           glassplayer.cpp glassplayer.h\
                           m3uplaylist.cpp m3uplaylist.h\
                           meteraverage.cpp meteraverage.h\
                           serverid.cpp serverid.h

nodist_glassplayer_SOURCES = audiodevice.cpp audiodevice.h\
                             cmdswitch.cpp cmdswitch.h\
                             codec.cpp codec.h\
                             connector.cpp connector.h\
                             glasslimits.h\
                             logging.cpp logging.h\
                             metaevent.cpp metaevent.h\
                             moc_audiodevice.cpp\
                             moc_codec.cpp\
                             moc_codec_fdk.cpp\
                             moc_codec_mpeg1.cpp\
                             moc_codec_null.cpp\
                             moc_codec_ogg.cpp\
                             moc_codec_pass.cpp\
                             moc_conn_file.cpp\
                             moc_conn_hls.cpp\
                             moc_conn_xcast.cpp\
                             moc_connector.cpp\
                             moc_dev_alsa.cpp\
                             moc_dev_file.cpp\
                             moc_dev_jack.cpp\
                             moc_dev_stdout.cpp\
                             moc_glassplayer.cpp\
                             moc_serverid.cpp\
                             ringbuffer.cpp ringbuffer.h

glassplayer_LDADD = @SIRLIBS@ @LIBJACK@ @SNDFILE_LIBS@ @ALSA_LIBS@ @ASIHPI_LIBS@ -lpthread -logg -lvorbis
glassplayer_LDFLAGS = @SIRFLAGS@

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
