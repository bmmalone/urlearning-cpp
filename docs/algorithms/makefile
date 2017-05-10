###
#   Thanks to Niklas Jahnsson for putting this together.
#
#   Replace: %s:bnsl_algorithms:<filename>:g
###

bnsl_algorithms.pdf: bnsl_algorithms.tex
	pdflatex bnsl_algorithms

toc:
	pdflatex bnsl_algorithms
	pdflatex bnsl_algorithms

bib:
	pdflatex bnsl_algorithms
	bibtex bnsl_algorithms
	pdflatex bnsl_algorithms
	pdflatex bnsl_algorithms

clean:
	rm -f *.aux *.log *~ *.bbl *.blg figures/*converted-to.pdf
