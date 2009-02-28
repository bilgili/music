## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = music eventsource eventsink eventselect eventgenerator \
	       eventcounter @OPTIONAL_UTILS@
EXTRA_PROGRAMS = viewevents clocksource contsink

music_SOURCES = music.cc application_mapper.cc application_mapper.hh
music_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
music_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/mpidep/libmpidep.a $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

eventsource_SOURCES = eventsource.cc datafile.h datafile.cc
eventsource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventsource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventsink_SOURCES = eventsink.cc
eventsink_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventsink_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventselect_SOURCES = eventselect.cc
eventselect_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventselect_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventgenerator_SOURCES = eventgenerator.cc
eventgenerator_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventgenerator_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventcounter_SOURCES = eventcounter.cc
eventcounter_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventcounter_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

viewevents_SOURCES = viewevents.cpp VisualiseNeurons.cpp VisualiseNeurons.h
viewevents_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
viewevents_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@  -lglut -lGL -lGLU 

clocksource_SOURCES = clocksource.cc
clocksource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
clocksource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

contsink_SOURCES = contsink.cc
contsink_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
contsink_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@


MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
