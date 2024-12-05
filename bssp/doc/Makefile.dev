PODM1 = pod2man -s 1 -c "BSSP executables"
PODM3 = pod2man -s 3 -c "BSSP library functions"
PODM5 = pod2man -s 5 -c "BSSP configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/bsspadmin.1 \
	./man/man1/bsspclock.1 \
	./man/man1/udpbsi.1 \
	./man/man1/udpbso.1 \
	./man/man1/tcpbsi.1 \
	./man/man1/tcpbso.1 \
	./man/man5/bssprc.5 \
	./man/man3/bssp.3

HTMLFILES = \
	./html/man1/bsspadmin.html \
	./html/man1/bsspclock.html \
	./html/man1/udpbsi.html \
	./html/man1/udpbso.html \
	./html/man1/tcpbsi.html \
	./html/man1/tcpbso.html \
	./html/man5/bssprc.html \
	./html/man3/bssp.html

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
