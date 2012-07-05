PODM1 = pod2man -s 1 -c "DTPC executables"
PODM3 = pod2man -s 3 -c "DTPC library functions"
PODM5 = pod2man -s 5 -c "DTPC files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/dtpcd.1 \
	./man/man1/dtpcclock.1 \
	./man/man1/dtpcadmin.1 \
	./man/man1/dtpcsend.1 \
	./man/man1/dtpcreceive.1 \
	./man/man3/dtcp.3 \
	./man/man5/dtcprc.5

HTMLFILES = \
	./html/man1/dtpcd.html \
	./html/man1/dtpcclock.html \
	./man/man1/dtpcadmin.1 \
	./man/man1/dtpcsend.1 \
	./man/man1/dtpcreceive.1 \
	./html/man3/dtcp.html \
	./html/man5/dtcprc.html

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
