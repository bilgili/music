## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = music eventsource eventlogger
#	eventsink eventselect eventgenerator eventcounter \
#        eventlogger eventdelay multiport \
#	clocksource contsink constsource contdelay \
#	messagesource waveproducer waveconsumer \
#	@OPTIONAL_UTILS@

EXTRA_DIST = chain.music cloop.music const.music contclock.music	\
	     events.music messages.music fork.music loop.music		\
	     wavetest.music viewevents.music demo.music demolarge.music	\
	     events.py \
             neuronGrid.data neuronGridLARGE.data			\
	     spikes0.dat spikes1.dat README

EXTRA_PROGRAMS = viewevents

MUSIC_INCLUDE = -I$(top_srcdir)/src -I$(top_builddir)/src

 music_SOURCES = music.cc application_mapper.cc application_mapper.hh
 music_CXXFLAGS = $(MUSIC_INCLUDE) -I$(top_srcdir) @MPI_CXXFLAGS@
 music_LDADD = $(top_builddir)/src/libmusic.la $(top_builddir)/mpidep/libmpidep.la $(top_builddir)/rudeconfig/librudeconfig.la @MPI_LDFLAGS@

 eventsource_SOURCES = eventsource.cc datafile.h datafile.cc
 eventsource_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
 eventsource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# eventsink_SOURCES = eventsink.cc
# eventsink_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# eventsink_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# eventselect_SOURCES = eventselect.cc
# eventselect_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# eventselect_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# eventgenerator_SOURCES = eventgenerator.cc
# eventgenerator_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# eventgenerator_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# eventcounter_SOURCES = eventcounter.cc
# eventcounter_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# eventcounter_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

 eventlogger_SOURCES = eventlogger.cc
 eventlogger_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
 eventlogger_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# eventdelay_SOURCES = eventdelay.cc
# eventdelay_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# eventdelay_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# multiport_SOURCES = multiport.cc
# multiport_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# multiport_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# clocksource_SOURCES = clocksource.cc
# clocksource_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# clocksource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# contsink_SOURCES = contsink.cc
# contsink_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# contsink_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# constsource_SOURCES = constsource.cc
# constsource_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# constsource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# contdelay_SOURCES = contdelay.cc
# contdelay_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# contdelay_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# messagesource_SOURCES = messagesource.cc
# messagesource_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# messagesource_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# waveproducer_SOURCES = waveproducer.cc
# waveproducer_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# waveproducer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@
 
# waveconsumer_SOURCES = waveconsumer.cc
# waveconsumer_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# waveconsumer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

# viewevents_SOURCES = viewevents.cpp VisualiseNeurons.cpp VisualiseNeurons.h
# viewevents_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
# viewevents_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@  -lglut -lGL -lGLU 


MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
