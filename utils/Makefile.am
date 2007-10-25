## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = music

music_SOURCES = music.cc parse.cc parse.hh
music_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
music_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
