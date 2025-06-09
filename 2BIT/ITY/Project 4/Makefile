PROJ=proj4

$(PROJ).pdf: $(PROJ).tex
	pdflatex $(PROJ).tex
	biber $(PROJ)
	pdflatex $(PROJ).tex
	pdflatex $(PROJ).tex

clean:
	rm -f $(PROJ).aux $(PROJ).dvi $(PROJ).log $(PROJ).ps $(PROJ).out $(PROJ).bcf $(PROJ).run.xml $(PROJ).bbl $(PROJ).blg

clean-all: clean
	rm -f $(PROJ).pdf