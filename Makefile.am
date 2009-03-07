SUBDIRS = mpidep src test rudeconfig utils doc

debian-source: dist
	mv music-@PACKAGE_VERSION@.tar.gz ../incf-music-@PACKAGE_VERSION@.orig.tar.gz
	( cd ..; tar zxf incf-music-@PACKAGE_VERSION@.orig.tar.gz )
	cp -pr debian ../music-@PACKAGE_VERSION@/debian

dist-hook:
	rm $(distdir)/TODO
