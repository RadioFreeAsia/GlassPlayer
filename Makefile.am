## automake.am
##
## Top level automake.am
##
## Use automake to process this into a Makefile.in
##
##   (C) Copyright 2014,2016 Fred Gleason <fredg@paravelsystems.com>
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

SUBDIRS = src

rpm:	dist
	cp glassplayer-$(VERSION).tar.gz $(HOME)/rpmbuild/SOURCES/
	$(RPMBUILD) -ba --target $(ARCH)-$(VENDOR)-linux glassplayer.spec
	mv $(HOME)/rpmbuild/RPMS/$(ARCH)/glassplayer-*.rpm $(top_srcdir)/
	mv $(HOME)/rpmbuild/SRPMS/glassplayer-*.src.rpm $(top_srcdir)/
	rm $(HOME)/rpmbuild/SOURCES/glassplayer-$(VERSION).tar.gz
	rm -rf $(HOME)/rpmbuild/BUILD/glassplayer-$(VERSION)

EXTRA_DIST = glassplayer.spec.in\
             autogen.sh\
             get_distro.sh\
             get_target.sh\
             PACKAGE_VERSION

CLEANFILES = *~\
             *.deb\
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
