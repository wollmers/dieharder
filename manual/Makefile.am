SOURCE = dieharder.tex
VERSION = 0.1
RELEASE = 1

EXTRA_DIST = dieharder.tex html.sty \
	     OPL.tex crown_quarto.tex rgbmacros.tex example_ascii.input

# The list of includes are things required for the build to work.  Examples
# might be included .tex or .sty files, embedded .eps files, and so forth.
# FIGS = $(shell ls *.fig 2>&1 | grep -v "No such file or directory" | sed -e "/\/bin\/ls:/d")
# FEPS = $(FIGS:.fig=.eps)
# INCLUDE = $(FIGS)

# Source(s) and production objects.
SOURCES = $(SOURCE)
LOGS = $(SOURCE:.tex=.log)
TOC = $(SOURCE:.tex=.toc)
AUX = $(SOURCE:.tex=.aux)
ERR = $(SOURCE:.tex=.err)
DVI = $(SOURCE:.tex=.dvi)
PS = $(SOURCE:.tex=.ps)
PDF = $(SOURCE:.tex=.pdf)
A4PS = dieharder_a4.ps
A4PDF = dieharder_a4.pdf
HTMLDIR = $(SOURCE:.tex=)

# Build targets (from rules)
.PHONY: link-sources
if HAVE_INSRCDIR
all:	$(PDF)
link-sources:
else
all:	link-sources $(PDF)
link-sources:
	-@for s in $(EXTRA_DIST); \
        do \
	  if [ ! -e $$s ] && [ -e $(srcdir)/$$s ]; then \
	    ln -s $(srcdir)/$$s; \
	  fi \
	done
endif
ps-local:  $(DVI) $(PS)
pdf-local: $(DVI) $(PDF)
a4ps:	$(DVI) $(A4PS)
a4pdf:	$(DVI) $(A4PS) $(A4PDF)
tar:	$(TAR)
tgz:	$(TGZ)


# Clean up all the junk build files and output formats to save space.
clean-local:
	rm -f $(DVI) $(AUX) $(LOGS) $(TOC) $(ERR) $(PS) $(PDF) dieharder.out
if !HAVE_INSRCDIR
	rm -f $(EXTRA_DIST) $(EXTRA_LOCAL)
endif

DISTCLEANFILES = macros.tex

#
#                RULES FOR BUILDING STANDARD TARGETS
#
# Rule for making dvi out of tex.  Do it twice to build table of contents
# and resolve forward references.
.tex.dvi: $(SOURCES) link-sources
	latex $< $@
	latex $< $@

# Rule for making postscript image
.dvi.ps:
	dvips -Pamz -Pcmz -o $@ $<

# Rule for making pdf image
.ps.pdf:
	ps2pdf $<

# Rule for making .fig->.eps
.fig.eps:
	fig2dev -L eps $< > $@

# Rule for making postscript image
dieharder_a4.ps: dieharder.dvi
	dvips -Pamz -Pcmz -t A4 -o $@ $<

# Rule for making pdf image
dieharder_a4.pdf: dieharder_a4.ps
	ps2pdf $<
