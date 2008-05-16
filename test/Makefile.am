## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = application1 waveproducer waveconsumer launchtest eventlogger

application1_SOURCES = application1.cc
application1_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
application1_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

waveproducer_SOURCES = waveproducer.cc
waveproducer_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
waveproducer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

waveconsumer_SOURCES = waveconsumer.cc
waveconsumer_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
waveconsumer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventlogger_SOURCES = eventlogger.cc
eventlogger_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
eventlogger_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

launchtest_SOURCES = launchtest.cc
launchtest_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
launchtest_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
