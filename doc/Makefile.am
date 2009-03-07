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

figures = figures/multisim.pdf \
	  figures/datamapping.pdf \
	  figures/ticklogic.pdf \
	  figures/ticklogic2.pdf \
	  figures/timeline.pdf \
	  figures/remapping.pdf \
	  figures/remapping2.pdf

music-manual.pdf: music-manual.bbl music-manual.ind $(figures)
	pdflatex music-manual
	pdflatex music-manual

music-manual.bbl: music-manual.aux
	bibtex music-manual

music-manual.ind: music-manual.idx
	makeindex music-manual

music-manual.aux: $(figures)
	pdflatex music-manual

$(figures): figures/%.pdf : figures/%.asy
	asy -f pdf -o $@ $<
