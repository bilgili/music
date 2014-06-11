## Process this file with Automake to create Makefile.in

ACLOCAL = $(top_srcdir)/aclocal.sh

install-data-hook:
	@INSTALL_PROGRAM@ events.test chain.test run_test \
			divergence.test fork.test loop.test \
			cloop.test const.test contclock.test \
			launchtest.test messages.test \
			multicomm.test multiport.test \
			  $(DESTDIR)@TESTSUITE_BASEDIR@
