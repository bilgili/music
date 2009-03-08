SUBDIRS = mpidep src test rudeconfig utils doc

debdir=../@PACKAGE_NAME@-@PACKAGE_VERSION@

debian-source: dist
	mv @PACKAGE_NAME@-@PACKAGE_VERSION@.tar.gz ../@PACKAGE_NAME@_@PACKAGE_VERSION@.orig.tar.gz
	@test ! -e $(debdir) || ( echo "*** Remove directory ../@PACKAGE_NAME@-@PACKAGE_VERSION@" && exit 1 )
	( cd ..; tar zxf @PACKAGE_NAME@_@PACKAGE_VERSION@.orig.tar.gz )
	mkdir $(debdir)/debian
	cp -p debian/* $(debdir)/debian
#	cp -p doc/music-manual.tex doc/music-rfc.bib $(debdir)/doc
#	mkdir $(debdir)/doc/figures
#	cp -p doc/figures/*.asy $(debdir)/doc/figures

dist-hook:
	rm $(distdir)/TODO
