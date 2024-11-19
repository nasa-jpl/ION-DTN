PODM1 = pod2man -s 1 -c "CFDP executables"
PODM3 = pod2man -s 3 -c "CFDP library functions"
PODM5 = pod2man -s 5 -c "CFDP configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/cfdpadmin.1 \
	./man/man1/cfdpclock.1 \
	./man/man1/cfdptest.1 \
	./man/man1/bputa.1 \
	./man/man1/bpcp.1 \
	./man/man1/bpcpd.1 \
	./man/man5/cfdprc.5 \
	./man/man3/cfdp.3

HTMLFILES = \
	./html/man1/cfdpadmin.html \
	./html/man1/cfdpclock.html \
	./html/man1/cfdptest.html \
	./html/man1/bputa.html \
	./html/man1/bpcp.html \
	./html/man1/bpcpd.html \
	./html/man5/cfdprc.html \
	./html/man3/cfdp.html

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
