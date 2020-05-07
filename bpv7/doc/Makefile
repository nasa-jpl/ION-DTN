PODM1 = pod2man -s 1 -c "BP executables"
PODM3 = pod2man -s 3 -c "BP library functions"
PODM5 = pod2man -s 5 -c "BP configuration files"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/bibeadmin.1 \
	./man/man1/bpadmin.1 \
	./man/man1/bpsecadmin.1 \
	./man/man1/bpchat.1 \
	./man/man1/bpclock.1 \
	./man/man1/bptransit.1 \
	./man/man1/bpcounter.1 \
	./man/man1/bpdriver.1 \
	./man/man1/bpecho.1 \
	./man/man1/bpsendfile.1 \
	./man/man1/bprecvfile.1 \
	./man/man1/bpsink.1 \
	./man/man1/bpsource.1 \
	./man/man1/bpstats.1 \
	./man/man1/bpstats2.1 \
	./man/man1/bpnmtest.1 \
	./man/man1/bplist.1 \
	./man/man1/bpcancel.1 \
	./man/man1/bptrace.1 \
	./man/man1/bping.1 \
	./man/man1/brsccla.1 \
	./man/man1/brsscla.1 \
	./man/man1/dgrcli.1 \
	./man/man1/dgrclo.1 \
	./man/man1/dtn2adminep.1 \
	./man/man1/dtn2admin.1 \
	./man/man1/dtn2fw.1 \
	./man/man1/ipnadminep.1 \
	./man/man1/ipnadmin.1 \
	./man/man1/ipnfw.1 \
	./man/man1/imcadmin.1 \
	./man/man1/imcfw.1 \
	./man/man1/ipnd.1 \
	./man/man1/lgagent.1 \
	./man/man1/lgsend.1 \
	./man/man1/hmackeys.1 \
	./man/man1/bpclm.1 \
	./man/man1/ltpcli.1 \
	./man/man1/ltpclo.1 \
	./man/man1/stcpcli.1 \
	./man/man1/stcpclo.1 \
	./man/man1/tcpcli.1 \
	./man/man1/tcpclo.1 \
	./man/man1/udpcli.1 \
	./man/man1/udpclo.1 \
	./man/man1/bibeclo.1 \
	./man/man1/dccpcli.1 \
	./man/man1/dccpclo.1 \
	./man/man1/cgrfetch.1 \
	./man/man5/biberc.5 \
	./man/man5/bprc.5 \
	./man/man5/bpsecrc.5 \
	./man/man5/ipnrc.5 \
	./man/man5/imcrc.5 \
	./man/man5/dtn2rc.5 \
	./man/man5/lgfile.5 \
	./man/man3/bpextensions.3 \
	./man/man3/bp.3

HTMLFILES = \
	./html/man1/bibeadmin.html \
	./html/man1/bpadmin.html \
	./html/man1/bpsecadmin.html \
	./html/man1/bpchat.html \
	./html/man1/bpclock.html \
	./html/man1/bptransit.html \
	./html/man1/bpcounter.html \
	./html/man1/bpdriver.html \
	./html/man1/bpecho.html \
	./html/man1/bpsendfile.html \
	./html/man1/bprecvfile.html \
	./html/man1/bpsink.html \
	./html/man1/bpsource.html \
	./html/man1/bpstats.html \
	./html/man1/bpstats2.html \
	./html/man1/bpnmtest.html \
	./html/man1/bplist.html \
	./html/man1/bpcancel.html \
	./html/man1/bptrace.html \
	./html/man1/bping.html \
	./html/man1/brsccla.html \
	./html/man1/brsscla.html \
	./html/man1/dgrcli.html \
	./html/man1/dgrclo.html \
	./html/man1/dtn2adminep.html \
	./html/man1/dtn2admin.html \
	./html/man1/dtn2fw.html \
	./html/man1/ipnadminep.html \
	./html/man1/ipnadmin.html \
	./html/man1/ipnfw.html \
	./html/man1/imcadmin.html \
	./html/man1/imcfw.html \
	./html/man1/ipnd.html \
	./html/man1/lgagent.html \
	./html/man1/lgsend.html \
	./html/man1/hmackeys.html \
	./html/man1/bpclm.html \
	./html/man1/ltpcli.html \
	./html/man1/ltpclo.html \
	./html/man1/stcpcli.html \
	./html/man1/stcpclo.html \
	./html/man1/tcpcli.html \
	./html/man1/tcpclo.html \
	./html/man1/udpcli.html \
	./html/man1/udpclo.html \
	./html/man1/bibeclo.html \
	./html/man1/dccpcli.html \
	./html/man1/dccpclo.html \
	./html/man1/cgrfetch.html \
	./html/man5/biberc.html \
	./html/man5/bprc.html \
	./html/man5/bpsecrc.html \
	./html/man5/ipnrc.html \
	./html/man5/imcrc.html \
	./html/man5/dtn2rc.html \
	./html/man5/lgfile.html \
	./html/man3/bpextensions.html \
	./html/man3/bp.html

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
