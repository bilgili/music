SUBDIRS 	= unittests/catch sanitytests

#runtestdir = $(bindir)
#dist_runtest_DATA = music_tests.sh

EXTRA_DIST	= $(SUBDIRS)

install-data-hook:
	cd $(srcdir);\
	@INSTALL_PROGRAM@ $(srcdir)/music_tests.sh $(DESTDIR)$(bindir)
