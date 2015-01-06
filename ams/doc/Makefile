PODM1 = pod2man -s 1 -c "AMS executables"
PODM3 = pod2man -s 3 -c "AMS library functions"
PODM5 = pod2man -s 5 -c "AMS configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/amsbenchr.1 \
	./man/man1/amsbenchs.1 \
	./man/man1/amsd.1 \
	./man/man1/amshello.1 \
	./man/man1/amslog.1 \
	./man/man1/amslogprt.1 \
	./man/man1/amspub.1 \
	./man/man1/amsshell.1 \
	./man/man1/amsstop.1 \
	./man/man1/amsmib.1 \
	./man/man1/amssub.1 \
	./man/man1/ramsgate.1 \
	./man/man3/ams.3 \
	./man/man5/petition_log.5 \
	./man/man5/amsrc.5 \
	./man/man5/amsxml.5

HTMLFILES = \
	./html/man1/amsbenchr.html \
	./html/man1/amsbenchs.html \
	./html/man1/amsd.html \
	./html/man1/amshello.html \
	./html/man1/amslog.html \
	./html/man1/amslogprt.html \
	./html/man1/amspub.html \
	./html/man1/amsshell.html \
	./html/man1/amsstop.html \
	./html/man1/amsmib.html \
	./html/man1/amssub.html \
	./html/man1/ramsgate.html \
	./html/man3/ams.html \
	./html/man5/petition_log.html \
	./html/man5/amsrc.html \
	./html/man5/amsxml.html

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
