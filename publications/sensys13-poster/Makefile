NAME      = main
PRODUCT   = $(NAME).pdf
TEXSOURCE = $(NAME).tex introduction.tex background.tex
BBL       = $(NAME).bbl


BIBINPUTS = references.bib	

all: $(PRODUCT)

$(NAME).pdf: $(TEXSOURCE) $(BBL)
$(NAME).dvi: $(TEXSOURCE) $(BBL) 
$(NAME).bbl: $(TEXSOURCE) $(BIBINPUTS)

clean:
	$(RM) $(NAME).aux $(NAME).dvi \
	$(NAME).log $(NAME).blg $(NAME).bbl $(NAME).out \
	$(NAME).toc $(NAME).lof $(NAME).lot $(NAME).brf *.aux

# configuration issues
.SUFFIXES: .tex .pdf .bbl

PDFLATEX  =	pdflatex
BIBTEX    =	bibtex
XELATEX   =	xelatex 
LATEX     =	latex
BIBLATEX  =	$(PDFLATEX)
BIBTEX    =	bibtex -min-crossref=1000
RM        =	rm -f
MV        =	mv

.tex.pdf:
	$(PDFLATEX) $(LATEXFLAGS) $<
	@while egrep -q 'LaTeX Warning:.*Rerun|Rerun to get citations correct' $*.log; do \
		echo $(PDFLATEX) $<; \
		$(PDFLATEX) $(LATEXFLAGS) $< || exit $$?; \
	done

.tex.bbl: 
	$(BIBLATEX) $(LATEXFLAGS) $<
	$(BIBTEX) $*
	$(RM) $*.aux $*.dvi $*.pdf

