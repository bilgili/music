SUBDIRS = mpidep src test rudeconfig utils doc

debian-source: dist
	mv music-@PACKAGE_VERSION@.tar.gz ../incf-music_@PACKAGE_VERSION@.orig.tar.gz
	@test -e ../music-@PACKAGE_VERSION@ && echo "*** Remove directory ../music-@PACKAGE_VERSION@" && exit 1
	( cd ..; tar zxf incf-music_@PACKAGE_VERSION@.orig.tar.gz )
	mkdir ../music-@PACKAGE_VERSION@/debian
	cp -p debian/* ../music-@PACKAGE_VERSION@/debian

dist-hook:
	rm $(distdir)/TODO
