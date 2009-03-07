## Process this file with Automake to create Makefile.in

dist_man1_MANS = music.1

EXTRA_DIST = music-manual.pdf

CLEANFILES = music-manual.aux \
	     music-manual.bbl \
	     music-manual.blg \
	     music-manual.idx \
	     music-manual.ilg \
	     music-manual.ind \
	     music-manual.lof \
	     music-manual.log \
	     music-manual.pdf \
	     music-manual.toc

music-manual.pdf: music-manual.bbl music-manual.ind
	pdflatex music-manual
	pdflatex music-manual

music-manual.bbl: music-manual.aux
	bibtex music-manual

music-manual.ind: music-manual.idx
	makeindex music-manual

music-manual.aux:
	pdflatex music-manual
