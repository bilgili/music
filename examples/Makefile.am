## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

examplesdir = $(libdir)/@PACKAGE@-@PACKAGE_VERSION@
examples_PROGRAMS = waveproducer waveconsumer 	 

MUSIC_INCLUDE = -I$(top_srcdir)/src -I$(top_builddir)/src

waveproducer_SOURCES = waveproducer.cc
waveproducer_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
waveproducer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

waveconsumer_SOURCES = waveconsumer.cc
waveconsumer_CXXFLAGS = $(MUSIC_INCLUDE) @MPI_CXXFLAGS@
waveconsumer_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

EXTRA_DIST = messages.music wavetest.music \
		viewevents.music demo.music demolarge.music \
		neuronGrid.data neuronGridLARGE.data \
		spikes0.dat spikes1.dat README

install-data-hook:
	mkdir -p $(DESTDIR)@PKGDATADIR@/examples
	@INSTALL_PROGRAM@  messages.music wavetest.music \
			viewevents.music demo.music demolarge.music \
			neuronGrid.data neuronGridLARGE.data \
			spikes0.dat spikes1.dat README \
			$(DESTDIR)@PKGDATADIR@/examples

