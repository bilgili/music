SUBDIRS = mpidep src test rudeconfig utils doc

debian-source: dist
	mv music-@PACKAGE_VERSION@.tar.gz ../incf-music-@PACKAGE_VERSION@.orig.tar.gz

dist-hook:
	rm $(distdir)/TODO
