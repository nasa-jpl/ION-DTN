PODM1 = pod2man -s 1 -c "DTKA executables"
PODM3 = pod2man -s 3 -c "DTKA overview"
PODM5 = pod2man -s 5 -c "DTKA configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/knadmin.1 \
	./man/man1/knclock.1 \
	./man/man1/knmgr.1 \
	./man/man1/kaboot.1 \
	./man/man1/kaadmin.1 \
	./man/man1/karecv.1 \
	./man/man1/kacompile.1 \
	./man/man1/kapublish.1 \
	./man/man5/kauthrc.5 \
	./man/man5/knoderc.5 \
	./man/man3/dtka.3

HTMLFILES = \
	./html/man1/knadmin.html \
	./html/man1/knclock.html \
	./html/man1/knmgr.html \
	./html/man1/kaboot.html \
	./html/man1/kaadmin.html \
	./html/man1/karecv.html \
	./html/man1/kacompile.html \
	./html/man1/kapublish.html \
	./html/man5/kauthrc.html \
	./html/man5/knoderc.html \
	./html/man3/dtka.html

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
