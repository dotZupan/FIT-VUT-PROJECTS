PROJ=proj2

$(PROJ).pdf: $(PROJ).tex
	pdflatex $(PROJ).tex
	pdflatex $(PROJ).tex

clean:
	rm -f $(PROJ).aux $(PROJ).log 

clean-all: clean
	rm -f $(PROJ).pdf