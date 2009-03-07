SUBDIRS = mpidep src test rudeconfig utils doc

debian-source: dist
	mv @PACKAGE_NAME@-@PACKAGE_VERSION@.tar.gz ../@PACKAGE_NAME@_@PACKAGE_VERSION@.orig.tar.gz
	@test -e ../@PACKAGE_NAME@-@PACKAGE_VERSION@ && echo "*** Remove directory ../@PACKAGE_NAME@-@PACKAGE_VERSION@" && exit 1
	( cd ..; tar zxf @PACKAGE_NAME@_@PACKAGE_VERSION@.orig.tar.gz )
	mkdir ../@PACKAGE_NAME@-@PACKAGE_VERSION@/debian
	cp -p debian/* ../@PACKAGE_NAME@-@PACKAGE_VERSION@/debian

dist-hook:
	rm $(distdir)/TODO
