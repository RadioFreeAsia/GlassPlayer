## automake.am
##
## Top level automake.am
##
## Use automake to process this into a Makefile.in
##
##   (C) Copyright 2014-2020 Fred Gleason <fredg@paravelsystems.com>
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

if DOCBOOK_AM
  DOCBOOK_AM_OPT = docs
endif

SUBDIRS = debian $(DOCBOOK_AM_OPT) src

rpm:	dist
	cp glassplayer-$(VERSION).tar.gz $(HOME)/rpmbuild/SOURCES/
	$(RPMBUILD) -ba --target $(ARCH)-$(VENDOR)-linux glassplayer.spec
	mv $(HOME)/rpmbuild/RPMS/$(ARCH)/glassplayer-*.rpm $(top_srcdir)/
	mv $(HOME)/rpmbuild/SRPMS/glassplayer-*.src.rpm $(top_srcdir)/
	rm $(HOME)/rpmbuild/SOURCES/glassplayer-$(VERSION).tar.gz
	rm -rf $(HOME)/rpmbuild/BUILD/glassplayer-$(VERSION)

install-win32:	
	cp src/glassplayer/glassplayer.exe .
	cp src/glassplayergui/glassplayergui.exe .
	cp $(MINGW32_SYS_ROOT)/bin/QtCore4.dll .
	cp $(MINGW32_SYS_ROOT)/bin/QtGui4.dll .
	cp $(MINGW32_SYS_ROOT)/bin/QtNetwork4.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libgcc_s_sjlj-1.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libpng16-16.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libsamplerate-0.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libsndfile-1.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libstdc++-6.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libltdl-7.dll .
	cp $(MINGW32_SYS_ROOT)/bin/libwinpthread-1.dll .
	cp $(MINGW32_SYS_ROOT)/bin/zlib1.dll .

install-win32-ogg:	
		cp $(MINGW32_SYS_ROOT)/bin/libvorbis-0.dll .
		cp $(MINGW32_SYS_ROOT)/bin/libvorbisenc-2.dll .
		cp $(MINGW32_SYS_ROOT)/bin/libvorbisfile-3.dll .
		cp $(MINGW32_SYS_ROOT)/bin/libogg-0.dll .
		cp $(MINGW32_SYS_ROOT)/bin/libopus-0.dll .

EXTRA_DIST = CODINGSTYLE\
             autogen.sh\
             get_distro.sh\
             get_target.sh\
             glassplayer.spec.in\
             PACKAGE_VERSION

CLEANFILES = *~\
             *.deb\
             *.dll\
             *.exe\
             *.mo\
             *.rpm\
             *.tar.gz\
             *.tgz\
             *.zip

DISTCLEANFILES = glassplayer.spec\
                 config.guess\
                 config.sub\
                 ltmain.sh

MAINTAINERCLEANFILES = *~\
                       *.tar.gz\
                       aclocal.m4\
                       compile\
                       config.guess\
                       config.status\
                       config.sub\
                       configure\
                       depcomp\
                       install-sh\
                       libtool\
                       ltmain.sh\
                       Makefile.in\
                       missing\
                       mkinstalldirs
