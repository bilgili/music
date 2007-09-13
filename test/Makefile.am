## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

bin_PROGRAMS = application1

application1_SOURCES = application1.cc
application1_CXXFLAGS = -I$(top_srcdir)/src @MPI_CXXFLAGS@
application1_LDADD = $(top_builddir)/src/libmusic.la @MPI_LDFLAGS@

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
