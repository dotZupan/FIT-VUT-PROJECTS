SOURCE=proj3

$(SOURCE).pdf: $(SOURCE).tex
	latex $(SOURCE).tex
	latex $(SOURCE).tex
	dvips $(SOURCE).dvi
	ps2pdf -sPAPERSIZE=a4 $(SOURCE).ps

clean:
	rm -f $(SOURCE).aux $(SOURCE).dvi $(SOURCE).log $(SOURCE).ps $(SOURCE).out

clean-all: clean
	rm -f $(SOURCE).pdf