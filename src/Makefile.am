## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

lib_LTLIBRARIES = libmusic.la

libmusic_la_SOURCES = \
	runtime.cc music/runtime.hh \
	setup.cc music/setup.hh
libmusic_la_HEADERS = music.hh
libmusic_la_CXXFLAGS = @MPI_CXXFLAGS@
libmusic_la_LDFLAGS = -version-info 0:0 -export-dynamic
libmusic_ladir = $(includedir)

musicincludedir = $(includedir)/music
musicinclude_HEADERS = music/runtime.hh music/setup.hh

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
