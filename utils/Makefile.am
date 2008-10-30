## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = music eventsource

music_SOURCES = music.cc application_mapper.cc application_mapper.hh
music_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
music_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

eventsource_SOURCES = eventsource.cc datafile.h datafile.cc
eventsource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventsource_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
