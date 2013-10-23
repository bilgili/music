## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = eventlogger
noinst_PROGRAMS = clocksource contsink constsource eventdelay contdelay \
		  messagesource waveproducer waveconsumer testallgather

EXTRA_DIST = chain.music cloop.music const.music contclock.music	\
	     events.music messages.music fork.music loop.music		\
	     wavetest.music viewevents.music demo.music demolarge.music	\
             neuronGrid.data neuronGridLARGE.data			\
	     spikes0.dat spikes1.dat README

waveproducer_SOURCES = waveproducer.cc
waveproducer_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
waveproducer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

waveconsumer_SOURCES = waveconsumer.cc
waveconsumer_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
waveconsumer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventlogger_SOURCES = eventlogger.cc
eventlogger_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
eventlogger_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

clocksource_SOURCES = clocksource.cc
clocksource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
clocksource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

contsink_SOURCES = contsink.cc
contsink_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
contsink_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

constsource_SOURCES = constsource.cc
constsource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
constsource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

eventdelay_SOURCES = eventdelay.cc
eventdelay_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
eventdelay_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

contdelay_SOURCES = contdelay.cc
contdelay_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
contdelay_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

messagesource_SOURCES = messagesource.cc
messagesource_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
messagesource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

testallgather_SOURCES = testallgather.cc
testallgather_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@
testallgather_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
