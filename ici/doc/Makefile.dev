PODM1 = pod2man -s 1 -c "ICI executables"
PODM3 = pod2man -s 3 -c "ICI library functions"
PODM5 = pod2man -s 5 -c "ICI configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/ionadmin.1 \
	./man/man1/ionsecadmin.1 \
	./man/man1/rfxclock.1 \
	./man/man1/psmwatch.1 \
	./man/man1/sdrwatch.1 \
	./man/man1/sdrmend.1 \
	./man/man1/ionunlock.1 \
	./man/man1/ionxnowner.1 \
	./man/man1/ionlog.1 \
	./man/man1/file2sdr.1 \
	./man/man1/file2sm.1 \
	./man/man1/psmshell.1 \
	./man/man1/sdr2file.1 \
	./man/man1/sm2file.1 \
	./man/man1/smlistsh.1 \
	./man/man1/smrbtsh.1 \
	./man/man1/owltsim.1 \
	./man/man1/owlttb.1 \
	./man/man5/ionconfig.5 \
	./man/man5/ionrc.5 \
	./man/man5/ionsecrc.5 \
	./man/man3/platform.3 \
	./man/man3/memmgr.3 \
	./man/man3/ion.3 \
	./man/man3/llcv.3 \
	./man/man3/lyst.3 \
	./man/man3/ioncbor.3 \
	./man/man3/ioncrc.3 \
	./man/man3/psm.3 \
	./man/man3/zco.3 \
	./man/man3/smlist.3 \
	./man/man3/smrbt.3 \
	./man/man3/sdrlist.3 \
	./man/man3/sdrstring.3 \
	./man/man3/sdrtable.3 \
	./man/man3/sdrhash.3 \
	./man/man3/sdr.3

HTMLFILES = \
	./html/man1/ionadmin.html \
	./html/man1/ionsecadmin.html \
	./html/man1/rfxclock.html \
	./html/man1/psmwatch.html \
	./html/man1/sdrwatch.html \
	./html/man1/sdrmend.html \
	./html/man1/ionunlock.html \
	./html/man1/ionxnowner.html \
	./html/man1/ionlog.html \
	./html/man1/file2sdr.html \
	./html/man1/file2sm.html \
	./html/man1/psmshell.html \
	./html/man1/sdr2file.html \
	./html/man1/sm2file.html \
	./html/man1/smlistsh.html \
	./html/man1/smrbtsh.html \
	./html/man1/owltsim.html \
	./html/man1/owlttb.html \
	./html/man5/ionconfig.html \
	./html/man5/ionrc.html \
	./html/man5/ionsecrc.html \
	./html/man3/platform.html \
	./html/man3/memmgr.html \
	./html/man3/ion.html \
	./html/man3/llcv.html \
	./html/man3/lyst.html \
	./html/man3/ioncbor.html \
	./html/man3/ioncrc.html \
	./html/man3/psm.html \
	./html/man3/zco.html \
	./html/man3/smlist.html \
	./html/man3/smrbt.html \
	./html/man3/sdrlist.html \
	./html/man3/sdrstring.html \
	./html/man3/sdrtable.html \
	./html/man3/sdrhash.html \
	./html/man3/sdr.html

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
