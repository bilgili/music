## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = music eventsource @OPTIONAL_UTILS@
EXTRA_PROGRAMS = viewevents

music_SOURCES = music.cc application_mapper.cc application_mapper.hh
music_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
music_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

eventsource_SOURCES = eventsource.cc datafile.h datafile.cc
eventsource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventsource_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

viewevents_SOURCES = viewevents.cpp VisualiseNeurons.cpp VisualiseNeurons.h
viewevents_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
viewevents_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@  -lglut -lGL -lGLU 


MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
