PODM1 = pod2man -s 1 -c "DGR executables"
PODM3 = pod2man -s 3 -c "DGR library functions"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/dgr2file.1 \
	./man/man1/file2dgr.1 \
	./man/man3/dgr.3

HTMLFILES = \
	./html/man1/dgr2file.html \
	./html/man1/file2dgr.html \
	./html/man3/dgr.html

ALL = $(MANFILES) $(HTMLFILES)

all:		$(ALL)

clean:
		rm -f man/man1/*.1
		rm -f man/man3/*.3
		rm -f html/man1/*.html
		rm -f html/man3/*.html
		rm -f *.tmp

install:
		cp man/man1/*.1 $(ROOT)/man/man1
		cp man/man3/*.3 $(ROOT)/man/man3

#	-	-	Man files	-	-	-	-	-

./man/man1/%.1:	pod1/%.pod
		$(PODM1) $< $@

./man/man3/%.3:	pod3/%.pod
		$(PODM3) $< $@

./html/man1/%.html:	pod1/%.pod
		$(PODH) --infile=$< --outfile=$@

./html/man3/%.html:	pod3/%.pod
		$(PODH) --infile=$< --outfile=$@
