PODM1 = pod2man -s 1 -c "TC executables"
PODM3 = pod2man -s 3 -c "TC overview"
PODM5 = pod2man -s 5 -c "TC configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/dtka.1 \
	./man/man1/dtkaadmin.1 \
	./man/man1/tccadmin.1 \
	./man/man1/tcc.1 \
	./man/man1/tcaboot.1 \
	./man/man1/tcaadmin.1 \
	./man/man1/tcarecv.1 \
	./man/man1/tcacompile.1 \
	./man/man1/tcapublish.1 \
	./man/man3/tc.3 \
	./man/man5/dtkarc.5 \
	./man/man5/tccrc.5 \
	./man/man5/tcarc.5

HTMLFILES = \
	./html/man1/dtka.html \
	./html/man1/dtkaadmin.html \
	./html/man1/tccadmin.html \
	./html/man1/tcc.html \
	./html/man1/tcaboot.html \
	./html/man1/tcaadmin.html \
	./html/man1/tcarecv.html \
	./html/man1/tcacompile.html \
	./html/man1/tcapublish.html \
	./html/man3/tc.html \
	./html/man5/dtkarc.html \
	./html/man5/tccrc.html \
	./html/man5/tcarc.html

ALL = $(MANFILES) $(HTMLFILES)

all:		$(ALL)

clean:
		rm -f man/man1/*.1
		rm -f man/man3/*.3
		rm -f man/man5/*.5
		rm -f html/man1/*.html
		rm -f html/man3/*.html
		rm -f html/man5/*.html
		rm -f *.tmp

install:
		cp man/man1/*.1 $(ROOT)/man/man1
		cp man/man3/*.3 $(ROOT)/man/man3
		cp man/man5/*.5 $(ROOT)/man/man5

#	-	-	Man files	-	-	-	-	-

./man/man1/%.1:	pod1/%.pod
		$(PODM1) $< $@

./man/man3/%.3:	pod3/%.pod
		$(PODM3) $< $@

./man/man5/%.5:	pod5/%.pod
		$(PODM5) $< $@

./html/man1/%.html:	pod1/%.pod
		$(PODH) --infile=$< --outfile=$@

./html/man3/%.html:	pod3/%.pod
		$(PODH) --infile=$< --outfile=$@

./html/man5/%.html:	pod5/%.pod
		$(PODH) --infile=$< --outfile=$@
