## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

noinst_LIBRARIES = libmpidep.a

libmpidep_a_SOURCES = mpidep.cc mpidep.hh
libmpidep_a_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir) @MPI_CXXFLAGS@


MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
