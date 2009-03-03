## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

lib_LTLIBRARIES = libmusic.la libmusic-c.la

libmusic_la_SOURCES = \
	runtime.cc music/runtime.hh \
	setup.cc music/setup.hh \
	error.cc music/error.hh music/debug.hh \
	linear_index.cc music/linear_index.hh \
	index_map.cc music/index_map.hh \
	music/data_map.hh \
	array_data.cc music/array_data.hh \
	configuration.cc music/configuration.hh \
	application_map.cc music/application_map.hh \
	ioutils.cc music/ioutils.hh \
	connectivity.cc music/connectivity.hh \
	spatial.cc music/spatial.hh \
	temporal.cc music/temporal.hh \
	parse.cc music/parse.hh \
	port.cc music/port.hh \
	sampler.cc music/sampler.hh \
	event_router.cc music/event_router.hh \
	distributor.cc music/distributor.hh \
	collector.cc music/collector.hh \
	clock.cc music/clock.hh \
	subconnector.cc music/subconnector.hh \
	connector.cc music/connector.hh \
	permutation_index.cc music/permutation_index.hh \
	index_map_factory.cc music/index_map_factory.hh \
	synchronizer.cc music/synchronizer.hh \
	BIFO.cc music/BIFO.hh \
	FIBO.cc music/FIBO.hh music/message.hh \
	music/interval.hh music/interval_tree.hh \
	music/communication.hh

libmusic_la_HEADERS = music.hh
libmusic_la_CXXFLAGS = @MPI_CXXFLAGS@
libmusic_la_LDFLAGS = -version-info 0:0 -export-dynamic
libmusic_ladir = $(includedir)

libmusic_c_la_SOURCES = \
	music-c.cc music-c-c.c music-c.h

libmusic_c_la_HEADERS = music-c.h
libmusic_c_la_CXXFLAGS = -DBUILDING_MUSIC_LIBRARY @MPI_CXXFLAGS@
libmusic_c_la_CFLAGS = -DBUILDING_MUSIC_LIBRARY @MPI_CFLAGS@
libmusic_c_la_LDFLAGS = -version-info 0:0 -export-dynamic
libmusic_c_ladir = $(includedir)

musicincludedir = $(includedir)/music
musicinclude_HEADERS = music/runtime.hh music/setup.hh \
		       music/interval.hh music/interval_tree.hh \
		       music/index_map.hh music/data_map.hh \
		       music/linear_index.hh music/array_data.hh \
		       music/configuration.hh music/connectivity.hh \
		       music/application_map.hh music/ioutils.hh \
		       music/spatial.hh music/temporal.hh music/error.hh \
		       music/debug.hh music/port.hh music/clock.hh \
		       music/connector.hh music/subconnector.hh \
		       music/permutation_index.hh music/synchronizer.hh \
		       music/index_map_factory.hh \
		       music/sampler.hh music/BIFO.hh \
		       music/FIBO.hh music/event_router.hh \
		       music/collector.hh music/distributor.hh \
		       music/cont_data.hh music/event.hh music/message.hh \
		       music/message.hh music/music-config.hh \
		       music/communication.hh

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
