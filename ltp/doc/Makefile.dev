PODM1 = pod2man -s 1 -c "LTP executables"
PODM3 = pod2man -s 3 -c "LTP library functions"
PODM5 = pod2man -s 5 -c "LTP configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/ltpadmin.1 \
	./man/man1/ltpsecadmin.1 \
	./man/man1/ltpclock.1 \
	./man/man1/ltpcounter.1 \
	./man/man1/ltpdriver.1 \
	./man/man1/ltpmeter.1 \
	./man/man1/sdatest.1 \
	./man/man1/udplsi.1 \
	./man/man1/udplso.1 \
	./man/man1/dccplsi.1 \
	./man/man1/dccplso.1 \
	./man/man5/ltprc.5 \
	./man/man5/ltpsecrc.5 \
	./man/man3/ltp.3 \
	./man/man3/sda.3

HTMLFILES = \
	./html/man1/ltpadmin.html \
	./html/man1/ltpsecadmin.html \
	./html/man1/ltpclock.html \
	./html/man1/ltpcounter.html \
	./html/man1/ltpdriver.html \
	./html/man1/ltpmeter.html \
	./html/man1/sdatest.html \
	./html/man1/udplsi.html \
	./html/man1/udplso.html \
	./html/man1/dccplsi.html \
	./html/man1/dccplso.html \
	./html/man5/ltprc.html \
	./html/man5/ltpsecrc.html \
	./html/man3/ltp.html \
	./html/man3/sda.html

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
