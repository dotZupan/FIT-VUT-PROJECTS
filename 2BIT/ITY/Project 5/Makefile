# názov výsledného PDF
TARGET = proj5

# hlavný .tex súbor (bez prípony)
MAIN = proj5

# predvolené pravidlo
all: $(TARGET).pdf

$(TARGET).pdf: $(MAIN).tex
	pdflatex $(MAIN).tex
	pdflatex $(MAIN).tex

clean:
	rm -f *.aux *.log *.nav *.out *.snm *.toc *.vrb *.lof *.lot *.gz *.bbl *.blg *.bcf *.run.xml 
