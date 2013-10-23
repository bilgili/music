edit = sed -e 's|@libdir[@]|$(libdir)|g'

pkgpython_PYTHON = __init__.py predict_rank.py config.py

predict_rank.py: Makefile predict_rank.py.in
	rm -f $@ $@.tmp
	$(edit) $${srcdir}$@.in >$@.tmp
	mv $@.tmp $@
